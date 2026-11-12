#include <QDateTime>
#include <QDebug>

#include "Utils.h"
#include "ChatService.h"
#include "ChatSessionDao.h"
#include "ChatMessageDao.h"
#include "CharacterService.h"
#include "ApiConfigService.h"
#include "CompletionPresetService.h"
#include "PromptBuilder.h"
#include "WorldInfoService.h"
#include "WorldInfoMatcher.h"
#include "ModelFactory.h"
#include "SettingsService.h"
#include "DatabaseManager.h"
#include "AppException.h"
#include "CompletionPresetService.h"


ChatService& ChatService::instance()
{
    static ChatService instance;
    return instance;
}

ChatService::ChatService(QObject* parent)
    : QObject(parent)
    , m_sessionDao(new ChatSessionDao(this))
    , m_messageDao(new ChatMessageDao(this))
    , m_characterDao(new CharacterDao(this))
{
}

ChatService::~ChatService()
{
    cleanupCurrentClient();
    // cancelGeneration();
}

void ChatService::restoreLastSession()
{
    QString targetCharId;
    QString lastCharId = SettingsService::instance().getString(GlobalConstants::LAST_ACTIVE_CHARACTER_ID);

    // 无历史记录，打开默认角色
    if (lastCharId.isEmpty()) {
        qDebug()<<"[ChatService::restoreLastSession]无历史记录";
        targetCharId=CharacterService::instance().getDefaultCharacterId();
    }else{
        // 校验角色是否存在
        CharacterEntity character = CharacterService::instance().getCharacterById(lastCharId);
        if (character.id.isEmpty()) {
            // 角色已删除，清空无效历史记录
            SettingsService::instance().setValue(GlobalConstants::LAST_ACTIVE_CHARACTER_ID, "");
            Utils::writeLog("INFO", "删除最后一个活动角色，跳过会话还原", "ChatService");
            return;
        }
        targetCharId=lastCharId;
    }

    // 打开该角色会话
    openCharacterSession(targetCharId);
}

QString ChatService::openCharacterSession(const QString &charId)
{
    // 合法性校验
    if (charId.isEmpty())
    {
        Utils::writeLog("WARN", "ChatService", "打开会话：空charId");
        return "";
    }
    // 生成中自动取消当前对话，防止状态冲突
    if (m_isGenerating)
        cancelGeneration();

// 获取该角色全部会话列表
    QList<ChatSessionEntity> allSessions = getSessionListByCharacter(charId);

//无任何会话：新建会话并绑定为默认
    if (allSessions.isEmpty())
    {
        QString newSid = createSession(charId);
        if (!newSid.isEmpty())
        {
            switchSession(newSid);
        }
        return newSid;
    }

//有会话读取上次打开的会话ID
    QString lastSid = getCharacterLastSessionId(charId);
    bool exists = m_sessionDao->exists(lastSid);

    QString targetSessionId;

    if (!lastSid.isEmpty() && exists)
    {
        // 上次会话有效，直接打开
        targetSessionId = lastSid;
    }
    else
    {
        // 上次会话不存在，取最新创建的会话
        targetSessionId = allSessions.first().id;
    }
    switchSession(targetSessionId);
    return targetSessionId;
}

void ChatService::saveCharacterLastSession(const QString &charId, const QString &sessionId)
{
    //更新最后会话
    CharacterEntity entity=m_characterDao->selectById(charId);
    entity.lastSessionId=sessionId;
    m_characterDao->update(entity);
    //更新最后角色
    SettingsService::instance().setValue(GlobalConstants::LAST_ACTIVE_CHARACTER_ID, charId);
}

// ======================================
// 会话管理
// ======================================
QString ChatService::createSession(const QString& characterId)
{
    if (characterId.isEmpty()) return {};

    ChatSessionEntity session;
    session.id = Utils::generateUuid();
    session.characterId = characterId;
    session.title = QStringLiteral("新对话");
    session.createdAt = QDateTime::currentDateTime();
    session.lastModified = QDateTime::currentDateTime();

    if (!m_sessionDao->insert(session)) {
        Utils::writeLog("ERROR", "ChatService", "创建聊天会话失败");
        return {};
    }



    // 自动插入角色开场白（默认第 1 条；metadata 记住索引，供会话内切换）
    const QStringList greetings = greetingTextsFor(characterId);
    if (!greetings.isEmpty()) {
        ChatMessageEntity firstMsg;
        firstMsg.id = Utils::generateUuid();
        firstMsg.sessionId = session.id;
        firstMsg.type = GlobalConstants::MessageType::Assistant;
        firstMsg.status = GlobalConstants::MessageGenerationStatus::Normal;
        firstMsg.content = greetings.first();
        firstMsg.senderName = "第一条消息";
        firstMsg.timestamp = QDateTime::currentDateTime();
        firstMsg.metadata.insert(QStringLiteral("greetingIndex"), 0);
        ChatMessageDao msgDao;
        msgDao.insert(firstMsg);
    }

    emit sessionListChanged(characterId);
    return session.id;
}

void ChatService::switchSession(const QString& sessionId)
{
    if (sessionId.isEmpty() || sessionId == m_currentSessionId) {
        return;
    }

    // 生成中切换会话，自动取消当前生成，避免数据串扰
    if (m_isGenerating) {
        cancelGeneration();
    }

    ChatSessionEntity session = m_sessionDao->selectById(sessionId);
    if (session.id.isEmpty()) {
        Utils::writeLog("WARN", "ChatService", "切换到不存在的会话： " + sessionId);
        return;
    }

    m_currentSessionId = sessionId;
    m_currentCharacterId = session.characterId;

    // 刷新开场白状态：新会话直接读 metadata，旧会话按内容反推并补写
    refreshGreetingState();

    // 绑定会话对应预设（预留：可从会话settings中读取预设ID）()
    // CompletionPresetEntity defaultPreset = CompletionPresetService::instance().getDefaultPreset();
    // m_currentPreset = defaultPreset;

    emit sessionSwitched(m_currentCharacterId, m_currentSessionId);
    //持久化保存最近会话
    saveCharacterLastSession(m_currentCharacterId, m_currentSessionId);
}

void ChatService::deleteSession(const QString& sessionId)
{
    if (sessionId.isEmpty()) return;

    bool isCurrent = (sessionId == m_currentSessionId);
    ChatSessionEntity session = m_sessionDao->selectById(sessionId);

    // 当前会话且正在生成，先终止
    if (isCurrent && m_isGenerating) {
        cancelGeneration();
    }

    if (!m_sessionDao->remove(sessionId)) {
        Utils::writeLog("ERROR", "ChatService", "删除会话失败: " + sessionId);
        return;
    }

    emit sessionDeleted(sessionId);
    emit messageCleared(sessionId);

    if (!session.characterId.isEmpty()) {
        emit sessionListChanged(session.characterId);
    }

    // 删除当前会话则先把状态清空，再落到别的会话上
    if (isCurrent) {
        m_currentSessionId.clear();
        m_currentCharacterId.clear();

        // 落到同角色"最近修改"的剩余会话；一条不剩就新建一个（会带开场白），
        // 否则界面会停在"有消息列表但没有会话"的悬空状态
        const QList<ChatSessionEntity> remaining = getSessionListByCharacter(session.characterId);
        QString nextSessionId;
        QDateTime newest;
        for (const ChatSessionEntity& candidate : remaining) {
            if (nextSessionId.isEmpty() || candidate.lastModified > newest) {
                nextSessionId = candidate.id;
                newest = candidate.lastModified;
            }
        }

        if (!nextSessionId.isEmpty()) {
            switchSession(nextSessionId);
        } else if (!session.characterId.isEmpty()) {
            const QString newSessionId = createSession(session.characterId);
            if (!newSessionId.isEmpty()) {
                switchSession(newSessionId);
            }
        }
    }
}

bool ChatService::renameSession(const QString& sessionId, const QString& title)
{
    if (sessionId.isEmpty()) return false;

    const QString trimmed = title.trimmed();
    if (trimmed.isEmpty()) return false;

    ChatSessionEntity session = m_sessionDao->selectById(sessionId);
    if (session.id.isEmpty()) return false;

    // 标题会进抽屉行和页面副标题，限长避免把布局挤爆。
    // 注意：不改 lastModified —— 改名不该让会话在列表里跳到最前。
    session.title = trimmed.left(40);
    if (!m_sessionDao->update(session)) {
        Utils::writeLog("ERROR", "ChatService", "重命名会话失败: " + sessionId);
        return false;
    }

    emit sessionListChanged(session.characterId);
    return true;
}

void ChatService::deleteAllSession()
{
    if(m_currentCharacterId.isEmpty()){
        Utils::writeLog("ERROR", "ChatService", "删除全部会话失败,当前角色为空");
        return;
    }

    if (!m_sessionDao->removeByCharacterId(m_currentCharacterId)) {
        Utils::writeLog("ERROR", "ChatService", "删除全部会话失败");
        return;
    }
    operationTip("删除成功");
    QString targetSessionId=createSession(m_currentCharacterId);
    switchSession(targetSessionId);
}

QList<ChatSessionEntity> ChatService::getSessionListByCharacter(const QString& characterId)
{
    return m_sessionDao->selectByCharacterId(characterId);
}

ChatSessionEntity ChatService::currentSession() const
{
    if (m_currentSessionId.isEmpty()) return {};
    return m_sessionDao->selectById(m_currentSessionId);
}

QString ChatService::currentSessionId() const
{
    return m_currentSessionId;
}

QString ChatService::currentCharacterId() const
{
    return m_currentCharacterId;
}

QList<ChatSessionEntity> ChatService::getSessionListByCurrentCharacter()
{
    if(m_currentCharacterId.isEmpty()){
        Utils::writeLog("ERROR", "ChatService", "获取角色会话失败,当前角色为空");
    }
    return m_sessionDao->selectByCharacterId(m_currentCharacterId);
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

    updateSessionLastModified(msg.sessionId);
    emit messageUpdated(messageId, msg.content);
}

void ChatService::deleteMessage(const QString& messageId)
{
    if (messageId.isEmpty()) return;

    ChatMessageEntity msg = m_messageDao->selectById(messageId);
    if (msg.id.isEmpty()) return;

    if (!m_messageDao->remove(messageId)) {
        Utils::writeLog("ERROR", "ChatService", "删除邮件失败: " + messageId);
        return;
    }

    // 删掉的如果正好是开场白，状态要跟着失效
    if (messageId == m_greetingMessageId) {
        refreshGreetingState();
    }

    updateSessionLastModified(msg.sessionId);
    emit messageDeleted(messageId);
}

// ======================================
// 开场白切换
// ======================================
QStringList ChatService::greetingTextsFor(const QString& characterId) const
{
    QStringList texts;
    if (characterId.isEmpty()) return texts;

    const CharacterEntity character =
        CharacterService::instance().getCharacterById(characterId);

    // 首条消息就是第 1 条开场白；备选依次排后。空串没有切换价值，直接过滤。
    if (!character.firstMessage.trimmed().isEmpty())
        texts.append(character.firstMessage);
    for (const QString& greeting : character.alternateGreetings) {
        if (!greeting.trimmed().isEmpty())
            texts.append(greeting);
    }
    return texts;
}

void ChatService::refreshGreetingState()
{
    m_greetingMessageId.clear();
    m_greetingIndex = -1;
    m_greetingCount = 0;

    if (!m_currentSessionId.isEmpty()) {
        const ChatSessionEntity session = m_sessionDao->selectById(m_currentSessionId);
        const QStringList texts = greetingTextsFor(session.characterId);
        m_greetingCount = texts.size();

        ChatMessageEntity first = m_messageDao->selectFirstBySessionId(m_currentSessionId);
        if (!first.id.isEmpty() && !texts.isEmpty()
            && first.type == GlobalConstants::MessageType::Assistant) {

            int index = first.metadata.value(QStringLiteral("greetingIndex"), -1).toInt();

            // 旧会话没有 metadata：按内容反推一条，命中就补写回去，
            // 这样升级前就存在的对话也能直接用上切换
            if (index < 0) {
                index = texts.indexOf(first.content);
                if (index >= 0) {
                    first.metadata.insert(QStringLiteral("greetingIndex"), index);
                    m_messageDao->update(first);
                }
            }

            if (index >= 0 && index < texts.size()) {
                m_greetingMessageId = first.id;
                m_greetingIndex = index;
            }
        }
    }

    emit greetingStateChanged();
}

void ChatService::applyGreetingIndex(const QString& messageId, int index)
{
    ChatMessageEntity message = m_messageDao->selectById(messageId);
    if (message.id.isEmpty()) return;

    const ChatSessionEntity session = m_sessionDao->selectById(message.sessionId);
    const QStringList texts = greetingTextsFor(session.characterId);
    if (index < 0 || index >= texts.size()) return;

    message.content = texts.at(index);
    message.metadata.insert(QStringLiteral("greetingIndex"), index);
    // 切换开场白不是"用户编辑内容"，不要动 isEdited

    if (!m_messageDao->update(message)) {
        Utils::writeLog("ERROR", "ChatService", "切换开场白失败: " + messageId);
        return;
    }

    m_greetingIndex = index;
    emit messageUpdated(message.id, message.content);
    emit greetingStateChanged();
    updateSessionLastModified(message.sessionId);
}

void ChatService::switchGreeting(int delta)
{
    if (m_greetingMessageId.isEmpty() || m_greetingCount <= 1 || delta == 0) return;

    // 循环取模；负数也要落到合法区间
    const int next = ((m_greetingIndex + delta) % m_greetingCount + m_greetingCount)
                     % m_greetingCount;
    if (next == m_greetingIndex) return;

    applyGreetingIndex(m_greetingMessageId, next);
}

// ======================================
// AI生成核心接口
// ======================================
void ChatService::sendMessage(const QString& userInput)
{
    if (userInput.isEmpty() || m_isGenerating) return;
    if (m_currentSessionId.isEmpty()) {
        Utils::writeLog("警告", "ChatService", "发送没有活跃的会话的消息");
        return;
    }

    DatabaseManager& db = DatabaseManager::instance();
    bool transactionOk = db.beginTransaction();
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
        updateSessionLastModified(m_currentSessionId);
        // 提交事务
        if (transactionOk) {
            if (!db.commitTransaction()) {
                throw QStringLiteral("事务提交失败");
            }
        }
        // 事务成功后再通知UI更新，保证数据库与UI一致()
        emit messageAppended(m_messageDao->selectById(userMsgId));
        emit messageAppended(m_messageDao->selectById(assistantMsgId));
    }
    catch (const QString& err) {
        if (transactionOk) {
            db.rollbackTransaction();
        }
        Utils::writeLog("ERROR","ChatService","发送消息失败: " + err);
        return;
    }

// 构建提示词上下文
    QList<ChatMessage> messages = buildPromptContext(userInput);

// 启动生成
    m_generatingMessageId = assistantMsgId;
    m_fullResponse.clear();
    m_isGenerating = true;
    emit generationStarted(assistantMsgId);

    startGeneration(messages);
}

void ChatService::regenerateLastMessage()
{
    if (m_isGenerating || m_currentSessionId.isEmpty()) return;

    // 定位最后一条助理消息
    ChatMessageEntity lastAssistantMsg;
    QList<ChatMessageEntity> allMsgs = m_messageDao->selectLatestBySessionId(m_currentSessionId, 1000);
    for (auto it = allMsgs.rbegin(); it != allMsgs.rend(); ++it) {
        if (it->type == GlobalConstants::MessageType::Assistant
            && it->status == GlobalConstants::MessageGenerationStatus::Normal) {
            lastAssistantMsg = *it;
            break;
        }
    }

    if (lastAssistantMsg.id.isEmpty()) {
        Utils::writeLog("WARN", "ChatService", "没有要重新生成的消息");
        return;
    }

    DatabaseManager& db = DatabaseManager::instance();
    bool transactionOk = db.beginTransaction();
    QString newAssistantMsgId;

    try {
        // 删除原助理消息
        if (!m_messageDao->remove(lastAssistantMsg.id)) {
            throw QStringLiteral("删除原消息失败");
        }

        // 插入新的生成中占位消息
        if (!insertPlaceholderAssistantMessage(newAssistantMsgId)) {
            throw QStringLiteral("创建新消息失败");
        }

        updateSessionLastModified(m_currentSessionId);

        if (transactionOk) {
            if (!db.commitTransaction()) {
                throw QStringLiteral("事务提交失败");
            }
        }

        // 事务成功后同步UI
        emit messageDeleted(lastAssistantMsg.id);
        emit messageAppended(m_messageDao->selectById(newAssistantMsgId));
    }
    catch (const QString& err) {
        if (transactionOk) {
            db.rollbackTransaction();
        }
        Utils::writeLog("ERROR", "ChatService", "重新生成失败: " + err);
        return;
    }

    // 重新构建上下文（无新用户输入）
    QList<ChatMessage> messages = buildPromptContext();

    m_generatingMessageId = newAssistantMsgId;
    m_fullResponse.clear();
    m_isGenerating = true;
    emit generationStarted(newAssistantMsgId);

    startGeneration(messages);
}

void ChatService::continueGeneration()
{
    if (m_isGenerating || m_currentSessionId.isEmpty()) return;

    QString assistantMsgId;
    if (!insertPlaceholderAssistantMessage(assistantMsgId)) {
        Utils::writeLog("ERROR", "ChatService", "创建继续消息失败");
        return;
    }

    emit messageAppended(m_messageDao->selectById(assistantMsgId));

    QList<ChatMessage> messages = buildPromptContext();

    m_generatingMessageId = assistantMsgId;
    m_fullResponse.clear();
    m_isGenerating = true;
    emit generationStarted(assistantMsgId);

    startGeneration(messages);
}

void ChatService::cancelGeneration()
{
    if (!m_isGenerating || m_generatingMessageId.isEmpty()) return;

    cleanupCurrentClient();
    m_isGenerating = false;

    // 标记消息为已取消
    m_messageDao->updateStatus(m_generatingMessageId,
        GlobalConstants::MessageGenerationStatus::Canceled);

    emit messageStatusChanged(m_generatingMessageId,
        static_cast<int>(GlobalConstants::MessageGenerationStatus::Canceled));
    emit generationCanceled(m_generatingMessageId);

    m_generatingMessageId.clear();
    updateSessionLastModified(m_currentSessionId);
}

bool ChatService::isGenerating() const
{
    return m_isGenerating;
}

// ======================================
// 私有槽：网络层信号响应
// ======================================
void ChatService::onStreamChunkReceived(const QString& chunk)
{
    if (m_generatingMessageId.isEmpty()) return;

    m_fullResponse += chunk;

    // 增量落库 + 通知UI
    // m_messageDao->appendContent(m_generatingMessageId, chunk);
    emit generationChunk(m_generatingMessageId, chunk);
}

void ChatService::onCompletionFinished(const QString& fullText)
{
    qDebug()<<"完整响应:\n"<<"fullText:"<<fullText<<"\n"<<"m_fullResponse:"<<m_fullResponse;
    if (m_generatingMessageId.isEmpty()) return;

    m_isGenerating = false;
    cleanupCurrentClient();

    // 标记消息为正常完成
    m_messageDao->updateStatus(m_generatingMessageId,
        GlobalConstants::MessageGenerationStatus::Normal);
    //回复内容写入数据库
    bool ok=m_messageDao->updateContent(m_generatingMessageId,m_fullResponse);
    if(!ok){
        Utils::writeLog("ERROR","ChatService",QString("写入失败:%1").arg(m_generatingMessageId));
    }

    //更新时间戳
    updateSessionLastModified(m_currentSessionId);
    //更新标题
    updateSessionTitle();


//发送信号
    emit messageStatusChanged(m_generatingMessageId,
        static_cast<int>(GlobalConstants::MessageGenerationStatus::Normal));
    emit generationFinished(m_generatingMessageId, m_fullResponse);
    m_generatingMessageId.clear();
}

void ChatService::onRequestFailed(const QString& error)
{
    if (m_generatingMessageId.isEmpty()) return;

    m_isGenerating = false;
    cleanupCurrentClient();

    // 标记消息为失败
    m_messageDao->updateStatus(m_generatingMessageId,
        GlobalConstants::MessageGenerationStatus::Failed);

    emit messageStatusChanged(m_generatingMessageId,
        static_cast<int>(GlobalConstants::MessageGenerationStatus::Failed));
    emit generationFailed(m_generatingMessageId,
        static_cast<int>(AppException::ErrorCode::NetworkError), error);

    updateSessionLastModified(m_currentSessionId);
    m_generatingMessageId.clear();
}

void ChatService::onRequestTimeout()
{
    onRequestFailed(QStringLiteral("请求超时，请检查网络或API配置"));
}


// ======================================
// 启动生成请求
// ======================================
void ChatService::startGeneration(const QList<ChatMessage>& messages)
{
    // 获取当前激活的API配置
    ApiConfigEntity apiConfig = ApiConfigService::instance().getActiveConfig();
    if (apiConfig.id.isEmpty()) {
        onRequestFailed(QStringLiteral("请先配置并激活API连接"));
        return;
    }

    // 创建对应类型的API客户端
    cleanupCurrentClient();
    m_currentClient = ModelFactory::instance().createClient(apiConfig.modelType);
    if (!m_currentClient) {
        onRequestFailed(QStringLiteral("不支持的模型类型"));
        return;
    }

    // 绑定网络层信号
    connect(m_currentClient, &BaseApiClient::streamChunkReceived,
            this, &ChatService::onStreamChunkReceived);
    connect(m_currentClient, &BaseApiClient::completionFinished,
            this, &ChatService::onCompletionFinished);
    connect(m_currentClient, &BaseApiClient::requestFailed,
            this, &ChatService::onRequestFailed);

    // 加载配置与生成参数
    m_currentClient->loadConfig(apiConfig);
    m_currentClient->loadPreset(CompletionPresetService::instance().getActivePreset());

    // 发起请求
    m_currentClient->sendChatCompletion(messages);
}

// ======================================
// 构建提示词上下文
// ======================================
QList<ChatMessage> ChatService::buildPromptContext(const QString& userInput)
{
    //角色卡
    CharacterEntity character = CharacterService::instance().getCharacterById(m_currentCharacterId);
    if (character.id.isEmpty()) return {};

    //历史消息
    QList<ChatMessageEntity> allMsgs = m_messageDao->selectLatestBySessionId(m_currentSessionId, 1000);
    // 提取有效历史消息（仅正常完成、未隐藏）(可优化)
    QList<ChatMessageEntity> validHistory;
    for (const ChatMessageEntity& msg : allMsgs) {
        if (msg.status == GlobalConstants::MessageGenerationStatus::Normal && !msg.isHidden) {
            validHistory.append(msg);
        }
    }

    CompletionPresetEntity preset = CompletionPresetService::instance().getActivePreset();

    // 世界书匹配
    QList<WorldInfoEntryEntity> wibr = WorldInfoService::instance().getEntryForEnabledBooks();
    WorldInfoMatcher& matcher = WorldInfoMatcher::instance();
    WorldInfoResult wir = matcher.matchAndResolve(allMsgs,wibr,userInput,character);

    // 长期记忆注入（预留扩展）
    QString memorySection;


// ===== 调用提示词构建器生成最终格式 =====

    //填充优先级从高到低大致为：
    //系统核心提示 → 角色描述 / 性格 / 场景 → 世界信息 → 对话示例 → 最新的聊天历史。
    //当 token 预算不足时，会从优先级最低的内容（最久远的聊天记录）开始裁剪,
    //直到总 token 符合预算。如果连必填的核心提示都超出预算，会抛出 TokenBudgetExceededError 异常。
    return PromptBuilder::instance().buildChatMessages(
        character,
        validHistory,
        userInput,
        preset,
        wir,
        memorySection);
}

// ======================================
// 消息插入工具
// ======================================
bool ChatService::insertUserMessage(const QString& content, QString& outMessageId)
{
    ChatMessageEntity msg;
    msg.id = Utils::generateUuid();
    msg.sessionId = m_currentSessionId;
    msg.type = GlobalConstants::MessageType::User;
    msg.status = GlobalConstants::MessageGenerationStatus::Normal;
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
    CharacterEntity character = CharacterService::instance().getCharacterById(m_currentCharacterId);

    ChatMessageEntity msg;
    msg.id = Utils::generateUuid();
    msg.sessionId = m_currentSessionId;
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

void ChatService::finalizeMessage(const QString &messageId, GlobalConstants::MessageGenerationStatus status)
{
    
}

// ======================================
// 内部辅助：资源与状态清理
// ======================================
void ChatService::cleanupCurrentClient()
{
    if (m_currentClient) {
        m_currentClient->cancelAllRequests();
        disconnect(m_currentClient, nullptr, this, nullptr);
        m_currentClient->deleteLater();
        m_currentClient = nullptr;
    }
}

void ChatService::updateSessionLastModified(const QString& sessionId)
{
    m_sessionDao->updateLastModified(sessionId, QDateTime::currentDateTime());
}

void ChatService::updateSessionTitle()
{
    ChatSessionEntity entity=m_sessionDao->selectById(m_currentSessionId);
    qDebug()<<entity.title;
    if(entity.title=="新对话"){
        entity.title=m_fullResponse.left(10)+"...";
    }
    if(!m_sessionDao->update(entity))
    {
        Utils::writeLog("ERROR","ChatService",QString("标题更新失败:%1").arg(m_generatingMessageId));
    }
}

QString ChatService::getCharacterLastSessionId(const QString &charId)
{
    CharacterEntity entity= m_characterDao->selectById(charId);
    return entity.lastSessionId;
}

