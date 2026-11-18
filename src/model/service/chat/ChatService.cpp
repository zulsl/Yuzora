#include <QDateTime>
#include <QDebug>

#include "Utils.h"
#include "ChatService.h"
#include "ChatMessageDao.h"
#include "CharacterService.h"
#include "SettingsService.h"
#include "ExceptionHandler.h"
#include "ChatPromptContext.h"
#include "AppException.h"
#include "ChatCompletionRunner.h"
#include "SessionService.h"
#include "GreetingService.h"


namespace {
// 旧版自动标题 = 回复前 10 字符 + "..."。回复以 <content> 开头时，
// 落库的是 "<content>\n..."，界面把它当富文本后就只剩 "..."。
// 只认这几个明确特征，避免把用户手写的 "<3" 之类标题也改掉。
bool isLegacyAutoTitleArtifact(const QString& title)
{
    const QString trimmed = title.trimmed();
    // 旧格式一定以 "..." 结尾；再看是否只剩省略号或带 <content 包装
    return trimmed.endsWith(QStringLiteral("..."))
        && (trimmed == QStringLiteral("...")
            || trimmed.startsWith(QStringLiteral("<content")));
}
}


ChatService::ChatService(const ChatServiceDependencies& deps, QObject* parent)
    : QObject(parent)
    , m_characterDao(deps.characterDao)
    , m_sessions(deps.sessions)
    , m_messageDao(deps.messageDao)
    , m_characterService(deps.characterService)
    , m_settingsService(deps.settingsService)
    , m_completion(deps.completion)
    , m_transaction(deps.transaction)
    , m_greeting(deps.greeting)
{
    // 补全器的三种回调是生成链路的唯一入口（生产实现见 ApiCompletionRunner）
    if (m_completion) {
        connect(m_completion, &ChatCompletionRunner::chunkReceived,
                this, &ChatService::onCompletionChunk);
        connect(m_completion, &ChatCompletionRunner::finished,
                this, &ChatService::onCompletionFinished);
        connect(m_completion, &ChatCompletionRunner::failed,
                this, &ChatService::onCompletionFailed);
        // 细节先于 failed 到达，先存起来，等 onCompletionFailed 一起交给 ViewModel
        connect(m_completion, &ChatCompletionRunner::failedDetail,
                this, [this](const QString& detail) { m_lastErrorDetail = detail; });
    }

    // 开场白状态透传给 QML（三个属性 + 一个 NOTIFY 信号对外保持不变）
    if (m_greeting) {
        connect(m_greeting, &GreetingService::stateChanged,
                this, &ChatService::greetingStateChanged);
    }
}

ChatService::~ChatService()
{
    if (m_completion) m_completion->cancel();
    // cancelGeneration();
}

void ChatService::restoreLastSession()
{
    QString targetCharId = m_sessions->lastActiveCharacterId();

    // 无历史记录，打开默认角色
    if (targetCharId.isEmpty()) {
        qDebug()<<"[ChatService::restoreLastSession]无历史记录";
        targetCharId = m_characterService->getDefaultCharacterId();
    } else if (m_characterService->getCharacterById(targetCharId).id.isEmpty()) {
        // 角色已删除，清空无效历史记录
        m_sessions->clearLastActiveCharacter();
        Utils::writeLog("INFO", "删除最后一个活动角色，跳过会话还原", "ChatService");
        return;
    }

    // 打开该角色会话
    openCharacterSession(targetCharId);
}

QString ChatService::openCharacterSession(const QString &charId)
{
    // 合法性校验
    if (charId.isEmpty()) {
        Utils::writeLog("WARN", "ChatService", "打开会话：空charId");
        return {};
    }
    // 生成中自动取消当前对话，防止状态冲突
    if (m_isGenerating)
        cancelGeneration();

    const QList<ChatSessionEntity> sessions = m_sessions->listForCharacter(charId);

    // 无任何会话：新建会话（带开场白）并绑定为默认
    if (sessions.isEmpty()) {
        const QString newSid = createSession(charId);
        if (!newSid.isEmpty())
            switchSession(newSid);
        return newSid;
    }

    // 进入角色时顺手修掉旧版留下的 "<content>\n..." 标题
    repairLegacySessionTitles(charId);

    // 有会话：优先上次打开的，失效了就取最新创建的那条
    const QString lastSid = m_sessions->lastSessionIdForCharacter(charId);
    const QString targetSessionId = (!lastSid.isEmpty() && m_sessions->exists(lastSid))
                                    ? lastSid
                                    : sessions.first().id;
    switchSession(targetSessionId);
    return targetSessionId;
}

// ======================================
// 会话管理
// ======================================
QString ChatService::createSession(const QString& characterId)
{
    const QString sessionId = m_sessions->create(characterId);
    if (sessionId.isEmpty()) return {};

    // 自动插入角色开场白（默认第 1 条）——插入细节归 GreetingService
    if (m_greeting && !m_greeting->insertGreetingMessage(sessionId, characterId)) {
        Utils::writeLog("ERROR", "ChatService", "插入开场白消息失败: " + sessionId);
    }

    emit sessionListChanged(characterId);
    return sessionId;
}

void ChatService::repairLegacySessionTitles(const QString& characterId)
{
    if (characterId.isEmpty()) return;

    bool changed = false;
    const QList<ChatSessionEntity> sessions = m_sessions->listForCharacter(characterId);
    for (const ChatSessionEntity& session : sessions) {
        if (!isLegacyAutoTitleArtifact(session.title)) continue;

        const ChatMessageEntity firstUser = m_messageDao->selectFirstUserBySessionId(session.id);
        if (firstUser.id.isEmpty()) continue;

        if (m_sessions->autoTitleFromUserMessage(session.id, firstUser.content, true))
            changed = true;
    }

    if (changed)
        emit sessionListChanged(characterId);
}

void ChatService::switchSession(const QString& sessionId)
{
    if (sessionId.isEmpty() || sessionId == m_sessions->currentSessionId()) {
        return;
    }

    // 生成中切换会话，自动取消当前生成，避免数据串扰
    if (m_isGenerating) {
        cancelGeneration();
    }

    const ChatSessionEntity session = m_sessions->switchTo(sessionId);
    if (session.id.isEmpty()) {
        Utils::writeLog("WARN", "ChatService", "切换到不存在的会话： " + sessionId);
        return;
    }

    // 刷新开场白状态：新会话直接读 metadata，旧会话按内容反推并补写
    if (m_greeting)
        m_greeting->refresh(session.id, session.characterId);

    emit sessionSwitched(session.characterId, session.id);
}

void ChatService::deleteSession(const QString& sessionId)
{
    if (sessionId.isEmpty()) return;

    const ChatSessionEntity session = m_sessions->entity(sessionId);
    const bool isCurrent = (sessionId == m_sessions->currentSessionId());

    // 当前会话且正在生成，先终止
    if (isCurrent && m_isGenerating) {
        cancelGeneration();
    }

    if (!m_sessions->remove(sessionId)) return;   // 失败日志在 SessionService 里

    emit sessionDeleted(sessionId);
    emit messageCleared(sessionId);

    if (!session.characterId.isEmpty()) {
        emit sessionListChanged(session.characterId);
    }

    // 删除当前会话则先把状态清空，再落到别的会话上
    if (!isCurrent) return;
    m_sessions->clearCurrent();

    // 落到同角色"最近修改"的剩余会话；一条不剩就新建一个（会带开场白），
    // 否则界面会停在"有消息列表但没有会话"的悬空状态
    const QString nextSessionId = m_sessions->mostRecentSessionId(session.characterId);
    if (!nextSessionId.isEmpty()) {
        switchSession(nextSessionId);
    } else if (!session.characterId.isEmpty()) {
        const QString newSessionId = createSession(session.characterId);
        if (!newSessionId.isEmpty()) {
            switchSession(newSessionId);
        }
    }
}

bool ChatService::renameSession(const QString& sessionId, const QString& title)
{
    if (!m_sessions->rename(sessionId, title)) return false;

    emit sessionListChanged(m_sessions->entity(sessionId).characterId);
    return true;
}

void ChatService::deleteAllSession()
{
    const QString characterId = m_sessions->currentCharacterId();
    if (characterId.isEmpty()) {
        Utils::writeLog("ERROR", "ChatService", "删除全部会话失败,当前角色为空");
        return;
    }

    if (!m_sessions->removeAllForCharacter(characterId)) return;

    operationTip("删除成功");
    const QString targetSessionId = createSession(characterId);
    switchSession(targetSessionId);
}

QList<ChatSessionEntity> ChatService::getSessionListByCharacter(const QString& characterId)
{
    return m_sessions->listForCharacter(characterId);
}

ChatSessionEntity ChatService::currentSession() const
{
    return m_sessions->current();
}

QString ChatService::currentSessionId() const
{
    return m_sessions->currentSessionId();
}

QString ChatService::currentCharacterId() const
{
    return m_sessions->currentCharacterId();
}

QList<ChatSessionEntity> ChatService::getSessionListByCurrentCharacter()
{
    return m_sessions->listForCurrentCharacter();
}

// ======================================
// 消息管理
// ======================================
QList<ChatMessageEntity> ChatService::loadMessagePage(const QString& sessionId, int offset, int limit)
{
    Q_UNUSED(offset)
    if (sessionId.isEmpty()) return {};
    return m_messageDao->selectLatestBySessionId(sessionId, limit);
}

QList<ChatMessageEntity> ChatService::loadLatestMessages(const QString& sessionId, int limit)
{
    if (sessionId.isEmpty()) return {};
    return m_messageDao->selectLatestBySessionId(sessionId, limit);
}

QList<ChatMessageEntity> ChatService::loadMessagesBefore(
    const QString& sessionId, const QDateTime& beforeTimestamp,
    const QString& beforeId, int limit)
{
    if (sessionId.isEmpty()) return {};
    return m_messageDao->selectBefore(sessionId, beforeTimestamp, beforeId, limit);
}

void ChatService::editMessage(const QString& messageId, const QString& newContent)
{
    if (messageId.isEmpty()) return;

    ChatMessageEntity msg = m_messageDao->selectById(messageId);
    if (msg.id.isEmpty()) return;

    msg.content = newContent.trimmed();
    msg.isEdited = true;

    if (!m_messageDao->update(msg)) {
        Utils::writeLog("ERROR", "ChatService", "编辑消息失败: " + messageId);
        return;
    }

    m_sessions->touch(msg.sessionId);
    emit messageUpdated(messageId, msg.content);
}

void ChatService::deleteMessage(const QString& messageId)
{
    if (messageId.isEmpty()) return;

    // 生成中不允许删除任何消息：UI 会置灰，这里兜底，避免删掉正在被引用的历史
    if (m_isGenerating) {
        emit operationError(QStringLiteral("正在生成回复，请先停止再删除"));
        return;
    }

    ChatMessageEntity msg = m_messageDao->selectById(messageId);
    if (msg.id.isEmpty()) return;

    if (!m_messageDao->remove(messageId)) {
        Utils::writeLog("ERROR", "ChatService", "删除消息失败: " + messageId);
        emit operationError(QStringLiteral("删除消息失败，请查看日志"));
        return;
    }

    // 删掉的如果正好是开场白，状态要跟着失效
    if (m_greeting && messageId == m_greeting->messageId()) {
        m_greeting->refresh(m_sessions->currentSessionId(), m_sessions->currentCharacterId());
    }

    m_sessions->touch(msg.sessionId);
    emit messageDeleted(messageId);
    emit operationTip(QStringLiteral("已删除消息"));
}

// ======================================
// 开场白切换
// ======================================
QString ChatService::greetingMessageId() const
{
    return m_greeting ? m_greeting->messageId() : QString();
}

int ChatService::greetingIndex() const
{
    return m_greeting ? m_greeting->index() : -1;
}

int ChatService::greetingCount() const
{
    return m_greeting ? m_greeting->count() : 0;
}

void ChatService::switchGreeting(int delta)
{
    if (!m_greeting) return;

    // 切换发生在承载消息上：GreetingService 改内容 + 发 stateChanged，
    // 这里补上"消息内容变了"和会话时间戳两件事
    const GreetingSwitchResult result = m_greeting->switchBy(m_sessions->currentCharacterId(), delta);
    if (result.messageId.isEmpty()) return;

    emit messageUpdated(result.messageId, result.content);
    m_sessions->touch(m_sessions->currentSessionId());
}


// 构建提示词上下文 → 启动补全。
// ST 在固定提示词装不进上下文预算时会中止本次生成（openai.js TokenBudgetExceededError）；
// 这里把 AppException 转成 generationFailed，避免把空/超长提示词直接发给模型。
bool ChatService::startGeneration(const QString& userInput, const QString& generationType,
                                  const QString& assistantMsgId)
{
    QList<ChatMessage> messages;
    try {
        messages = buildChatPromptContext(
            m_sessions->currentCharacterId(), m_sessions->currentSessionId(),
            userInput, m_messageDao, m_characterService, generationType);
    }
    catch (const AppException& ex) {
        Utils::writeLog("ERROR", "ChatService",
                        QStringLiteral("构建提示词失败: %1").arg(ex.getMessage()));
        failGeneration(assistantMsgId, ex.getMessage());
        return false;
    }
    catch (const QString& err) {
        Utils::writeLog("ERROR", "ChatService",
                        QStringLiteral("构建提示词失败: %1").arg(err));
        failGeneration(assistantMsgId, err);
        return false;
    }

    m_generatingMessageId = assistantMsgId;
    m_fullResponse.clear();
    m_lastErrorDetail.clear();
    m_isGenerating = true;
    emit generationStarted(assistantMsgId);

    if (m_completion) m_completion->start(messages);
    return true;
}

void ChatService::failGeneration(const QString& assistantMsgId, const QString& error)
{
    m_lastErrorDetail.clear();
    if (assistantMsgId.isEmpty()) {
        emit operationError(error);
        return;
    }

    m_messageDao->updateStatus(assistantMsgId,
        GlobalConstants::MessageGenerationStatus::Failed);

    emit messageStatusChanged(assistantMsgId,
        static_cast<int>(GlobalConstants::MessageGenerationStatus::Failed));
    emit generationFailed(assistantMsgId,
        static_cast<int>(AppException::ErrorCode::ModelError), error);

    m_sessions->touch(m_sessions->currentSessionId());
}

// ======================================
// AI生成核心接口
// ======================================
void ChatService::sendMessage(const QString& userInput)
{
    if (userInput.isEmpty() || m_isGenerating) return;
    if (m_sessions->currentSessionId().isEmpty()) {
        Utils::writeLog("ERROR", "ChatService", "发送没有活跃的会话的消息");
        // emit ExceptionHandler::instance().errorOccurred("[ChatService]发送没有活跃的会话的消息");
        return;
    }

    const bool transactionOk = (m_transaction && m_transaction->begin());
    QString userMsgId;//消息uuid
    QString assistantMsgId;//回复uuid

//持久化到聊天记录
    try {
        // 插入用户聊天记录（事务内仅落库，不发信号）
        if (!insertUserMessage(userInput, userMsgId)) {
            throw QStringLiteral("插入用户消息失败");
        }
        // 插入生成中占位助理消息
        if (!insertPlaceholderAssistantMessage(assistantMsgId)) {
            throw QStringLiteral("创建助理消息失败");
        }
        // 更新会话最后修改时间
        m_sessions->touch(m_sessions->currentSessionId());
        // 提交事务
        if (transactionOk) {
            if (!m_transaction->commit()) {
                throw QStringLiteral("事务提交失败");
            }
        }
        // 事务成功后再通知UI更新，保证数据库与UI一致()
        emit messageAppended(m_messageDao->selectById(userMsgId));
        emit messageAppended(m_messageDao->selectById(assistantMsgId));

        // 标题只用用户自己说的话：模型回复可能以 <content> 等包装标签开头，
        // 截前几个字符会把标签/空白带进标题（界面上就只剩"..."）。
        if (m_sessions->autoTitleFromUserMessage(m_sessions->currentSessionId(), userInput))
            emit sessionListChanged(m_sessions->currentCharacterId());
    }
    catch (const QString& err) {
        if (transactionOk) {
            m_transaction->rollback();
        }
        Utils::writeLog("ERROR","ChatService","发送消息失败: " + err);
        return;
    }

// 构建提示词上下文并启动生成
    startGeneration(userInput, QStringLiteral("normal"), assistantMsgId);
}

void ChatService::regenerateLastMessage()
{
    if (m_isGenerating) return;

    const QString sessionId = m_sessions->currentSessionId();
    if (sessionId.isEmpty()) return;

    const ChatMessageEntity last = m_messageDao->selectLastBySessionId(sessionId);
    if (last.id.isEmpty() || last.type != GlobalConstants::MessageType::Assistant) {
        emit operationError(QStringLiteral("最后一条消息不是助手回复"));
        return;
    }

    regenerateMessage(last.id);
}

// 重新生成一条助手回复：只允许会话最后一条、非开场白。
// 失败/取消的占位也是助手消息，所以"失败后重试"和"换一条回复"走同一条路；
// 以前按"最后一条 Normal 助手消息"找，会把更早的正常回复误删，这里用会话最后一条兜住。
void ChatService::regenerateMessage(const QString& messageId)
{
    if (m_isGenerating) {
        emit operationError(QStringLiteral("正在生成回复，请先停止再重新生成"));
        return;
    }

    const QString sessionId = m_sessions->currentSessionId();
    if (sessionId.isEmpty() || messageId.isEmpty())
        return;

    const ChatMessageEntity message = m_messageDao->selectById(messageId);
    if (message.id.isEmpty() || message.sessionId != sessionId) {
        emit operationError(QStringLiteral("找不到要重新生成的消息"));
        return;
    }
    if (message.type != GlobalConstants::MessageType::Assistant) {
        emit operationError(QStringLiteral("只能重新生成助手回复"));
        return;
    }
    if (m_greeting && messageId == m_greeting->messageId()) {
        emit operationError(QStringLiteral("开场白不能重新生成"));
        return;
    }
    if (m_messageDao->selectLastBySessionId(sessionId).id != messageId) {
        emit operationError(QStringLiteral("只能重新生成会话的最后一条回复"));
        return;
    }

    const bool transactionOk = (m_transaction && m_transaction->begin());
    QString newAssistantMsgId;

    try {
        // 删除旧回复（位置就是最后一条），再插新的生成中占位
        if (!m_messageDao->remove(messageId)) {
            throw QStringLiteral("删除原回复失败");
        }
        if (!insertPlaceholderAssistantMessage(newAssistantMsgId)) {
            throw QStringLiteral("创建新回复失败");
        }

        m_sessions->touch(sessionId);

        if (transactionOk) {
            if (!m_transaction->commit()) {
                throw QStringLiteral("事务提交失败");
            }
        }

        // 事务成功后再同步 UI
        emit messageDeleted(messageId);
        emit messageAppended(m_messageDao->selectById(newAssistantMsgId));
    }
    catch (const QString& err) {
        if (transactionOk) {
            m_transaction->rollback();
        }
        Utils::writeLog("ERROR", "ChatService", "重新生成失败: " + err);
        emit operationError(QStringLiteral("重新生成失败：") + err);
        return;
    }

    // 重新构建上下文（无新用户输入）；新占位是 Generating，不会进上下文
    startGeneration(QString(), QStringLiteral("regenerate"), newAssistantMsgId);
}

void ChatService::continueGeneration()
{
    if (m_isGenerating || m_sessions->currentSessionId().isEmpty()) return;

    QString assistantMsgId;
    if (!insertPlaceholderAssistantMessage(assistantMsgId)) {
        Utils::writeLog("ERROR", "ChatService", "创建继续消息失败");
        return;
    }

    emit messageAppended(m_messageDao->selectById(assistantMsgId));

    startGeneration(QString(), QStringLiteral("continue"), assistantMsgId);
}

void ChatService::cancelGeneration()
{
    if (!m_isGenerating || m_generatingMessageId.isEmpty()) return;

    if (m_completion) m_completion->cancel();
    m_isGenerating = false;
    m_lastErrorDetail.clear();

    // 标记消息为已取消
    m_messageDao->updateStatus(m_generatingMessageId,
        GlobalConstants::MessageGenerationStatus::Canceled);

    emit messageStatusChanged(m_generatingMessageId,
        static_cast<int>(GlobalConstants::MessageGenerationStatus::Canceled));
    emit generationCanceled(m_generatingMessageId);

    m_generatingMessageId.clear();
    m_sessions->touch(m_sessions->currentSessionId());
}

bool ChatService::isGenerating() const
{
    return m_isGenerating;
}

// ======================================
// 私有槽：网关回调（流式增量 / 完成 / 失败）
// ======================================
void ChatService::onCompletionChunk(const QString& chunk)
{
    if (m_generatingMessageId.isEmpty()) return;

    m_fullResponse += chunk;

    // 增量只发信号，不写库（避免每个 token 一次 UPDATE）
    emit generationChunk(m_generatingMessageId, chunk);
}

void ChatService::onCompletionFinished(const QString& fullText)
{
    if (m_generatingMessageId.isEmpty()) return;

    m_isGenerating = false;

    // 非流式响应不会经过 chunk()，这里用网关给的完整文本兜底
    if (m_fullResponse.isEmpty())
        m_fullResponse = fullText;

    // 空回复不能当成功：存一条空的 Normal 消息会留在历史里污染后续请求的上下文
    if (m_fullResponse.isEmpty()) {
        onCompletionFailed(QStringLiteral("接口返回了空内容（没有解析到回复文本）"));
        return;
    }

    // 标记消息为正常完成
    m_messageDao->updateStatus(m_generatingMessageId,
        GlobalConstants::MessageGenerationStatus::Normal);
    //回复内容写入数据库
    bool ok=m_messageDao->updateContent(m_generatingMessageId,m_fullResponse);
    if(!ok){
        Utils::writeLog("ERROR","ChatService",QString("写入失败:%1").arg(m_generatingMessageId));
    }

    //更新时间戳
    m_sessions->touch(m_sessions->currentSessionId());
    // 标题在 sendMessage 时已按首条用户消息生成；不要用回复文本覆盖。


//发送信号
    emit messageStatusChanged(m_generatingMessageId,
        static_cast<int>(GlobalConstants::MessageGenerationStatus::Normal));
    emit generationFinished(m_generatingMessageId, m_fullResponse);
    m_generatingMessageId.clear();
}

void ChatService::onCompletionFailed(const QString& error)
{
    if (m_generatingMessageId.isEmpty()) return;

    m_isGenerating = false;

    // 标记消息为失败
    m_messageDao->updateStatus(m_generatingMessageId,
        GlobalConstants::MessageGenerationStatus::Failed);

    emit messageStatusChanged(m_generatingMessageId,
        static_cast<int>(GlobalConstants::MessageGenerationStatus::Failed));

    // 细节先发：ViewModel 收到 generationFailed 时已经能拿到技术细节
    const QString detail = m_lastErrorDetail;
    m_lastErrorDetail.clear();
    if (!detail.isEmpty())
        emit generationErrorDetail(detail);

    emit generationFailed(m_generatingMessageId,
        static_cast<int>(AppException::ErrorCode::NetworkError), error);

    m_sessions->touch(m_sessions->currentSessionId());
    m_generatingMessageId.clear();
}

// ======================================
// 消息插入工具
// ======================================
bool ChatService::insertUserMessage(const QString& content, QString& outMessageId)
{
    ChatMessageEntity msg;
    msg.id = Utils::generateUuid();
    msg.sessionId = m_sessions->currentSessionId();
    msg.type = GlobalConstants::MessageType::User;
    msg.status = GlobalConstants::MessageGenerationStatus::Normal;
    // 落库保留原始文本：{{user}} / {{char}} 只在显示层和提示词装配时替换。
    // 如果在这里替换，改名后旧消息不会更新，编辑时也会丢掉原始宏。
    msg.content = content;
    msg.senderName = QStringLiteral("User");
    msg.timestamp = QDateTime::currentDateTime();
    msg.isEdited = false;
    msg.isHidden = false;

    bool ok = m_messageDao->insert(msg);
    if (ok) {
        outMessageId = msg.id;
    }
    return ok;
}

bool ChatService::insertPlaceholderAssistantMessage(QString& outMessageId)
{
    CharacterEntity character = m_characterService->getCharacterById(m_sessions->currentCharacterId());

    ChatMessageEntity msg;
    msg.id = Utils::generateUuid();
    msg.sessionId = m_sessions->currentSessionId();
    msg.type = GlobalConstants::MessageType::Assistant;
    msg.status = GlobalConstants::MessageGenerationStatus::Generating;
    msg.content = QString();
    msg.senderName = character.name;
    msg.timestamp = QDateTime::currentDateTime();
    msg.isEdited = false;
    msg.isHidden = false;

    bool ok = m_messageDao->insert(msg);
    if (ok) {
        outMessageId = msg.id;
    }
    return ok;
}
