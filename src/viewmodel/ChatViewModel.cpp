#include "ChatViewModel.h"
#include "ChatService.h"
#include "ChatMessageItemModel.h"
#include "CharacterService.h"
#include "CompletionPresetService.h"
#include "SettingsService.h"
#include "Utils.h"
#include "GlobalConstants.h"

#include <QDir>

ChatViewModel::ChatViewModel(QObject* parent)
    : QObject(parent)
    , m_messageModel(new ChatMessageListModel(this))
    , m_sessionModel(new ChatSessionListModel(this))
    , m_chatService(&ChatService::instance())
    , m_characterService(&CharacterService::instance())
    , m_completionPresetService(&CompletionPresetService::instance())
{
    // 会话状态信号连接
    connect(m_chatService, &ChatService::sessionSwitched,
            this, &ChatViewModel::onSessionSwitched);

    // 会话列表变更（新建/删除/重命名）后刷新抽屉。
    // 原先这里没连，列表只在"切换会话"时顺手刷新，所以单条删除/改名后界面不会变。
    connect(m_chatService, &ChatService::sessionListChanged,
            this, [this](const QString&) {
                refreshSessionList();

                // 改名的可能正是当前会话，页面副标题要跟着变
                const ChatSessionEntity current = m_chatService->currentSession();
                if (!current.id.isEmpty() && current.title != m_currentSessionTitle) {
                    m_currentSessionTitle = current.title;
                    emit currentSessionTitleChanged();
                }
            });
    connect(m_chatService, &ChatService::sessionDeleted,
            this, [this](const QString& sessionId) {
                if (sessionId == m_chatService->currentSessionId()) {
                    m_messageModel->clear();
                    m_currentCharName.clear();
                    m_currentCharAvatarPath.clear();
                    m_currentSessionTitle.clear();
                    m_currentSessionId.clear();
                    m_currentPresetName.clear();
                    m_oldestMessageTime = QDateTime();
                    m_oldestMessageId.clear();
                    m_hasMoreHistory = false;
                    emit currentCharNameChanged();
                    emit currentSessionTitleChanged();
                    emit currentSessionIdChanged();
                    emit currentPresetChanged();
                    emit hasMoreHistoryChanged();
                }
            });

    // 消息变更信号连接
    connect(m_chatService, &ChatService::messageAppended,
            this, &ChatViewModel::onMessageAppended);
    connect(m_chatService, &ChatService::messageUpdated,
            this, &ChatViewModel::onMessageUpdated);
    connect(m_chatService, &ChatService::messageStatusChanged,
            this, &ChatViewModel::onMessageStatusChanged);
    connect(m_chatService, &ChatService::messageDeleted,
            this, &ChatViewModel::onMessageDeleted);

    // 开场白状态直接透传（承载消息、序号、总数都由 Service 维护）
    connect(m_chatService, &ChatService::greetingStateChanged,
            this, &ChatViewModel::greetingStateChanged);

    // 消息条数：页面上"是否只有开场白这一条"要读它。
    // 不直接读 ListView.count——QML 同文件内对后声明的 id 做绑定引用不会生效，
    // 而切换条又必须排在 ListView 前面（否则不在顶部）。
    connect(m_messageModel, &QAbstractItemModel::rowsInserted,
            this, [this] { emit messageCountChanged(); });
    connect(m_messageModel, &QAbstractItemModel::rowsRemoved,
            this, [this] { emit messageCountChanged(); });
    connect(m_messageModel, &QAbstractItemModel::modelReset,
            this, [this] { emit messageCountChanged(); });

    // 生成生命周期信号连接
    connect(m_chatService, &ChatService::generationStarted,
            this, &ChatViewModel::onGenerationStarted);
    connect(m_chatService, &ChatService::generationChunk,
            this, &ChatViewModel::onGenerationChunk);
    connect(m_chatService, &ChatService::generationFinished,
            this, &ChatViewModel::onGenerationFinished);
    connect(m_chatService, &ChatService::generationFailed,
            this, &ChatViewModel::onGenerationFailed);
    connect(m_chatService, &ChatService::generationCanceled,
            this, &ChatViewModel::onGenerationCanceled);

    connect(m_chatService, &ChatService::operationTip,
            this, &ChatViewModel::operationTip);
    connect(m_chatService, &ChatService::operationError,
            this, &ChatViewModel::operationError);

    connect(&SettingsService::instance(), &SettingsService::settingChanged,
            this, [this](const QString& key, const QVariant&) {
                if (key == GlobalConstants::SEND_WITH_ENTER)
                    emit sendWithEnterChanged();
            });


    //加载最后会话角色
    m_chatService->restoreLastSession();
    m_currentSessionId = m_chatService->currentSessionId();

    //加载角色会话列表
    refreshSessionList();
}

ChatViewModel::~ChatViewModel() = default;

// ==================== 属性读取 ====================
bool ChatViewModel::isGenerating() const
{
    return m_isGenerating;
}

QString ChatViewModel::currentCharName() const
{
    return m_currentCharName;
}

QString ChatViewModel::currentSessionTitle() const
{
    return m_currentSessionTitle;
}

QString ChatViewModel::currentSessionId() const
{
    return m_currentSessionId;
}

bool ChatViewModel::hasMoreHistory() const
{
    return m_hasMoreHistory;
}

bool ChatViewModel::sendWithEnter() const
{
    return SettingsService::instance().getBool(GlobalConstants::SEND_WITH_ENTER);
}

ChatMessageListModel* ChatViewModel::messageModel() const
{
    return m_messageModel;
}

QString ChatViewModel::currentPresetName() const
{
    return m_completionPresetService->getActivePreset().name;
    // return m_currentPresetName;
}

// ======================================
// UI 操作接口（全部转发至 ChatService）
// ======================================
void ChatViewModel::sendMessage(const QString& content)
{
    QString text = content.trimmed();
    m_chatService->sendMessage(text);
}

void ChatViewModel::regenerateLastMessage()
{
    if (m_isGenerating) return;
    m_chatService->regenerateLastMessage();

    // m_chatService->regenerateLastMessage(m_currentCharId, m_currentSessionId);
}

void ChatViewModel::continueGeneration()
{
    if (m_isGenerating) return;
    m_chatService->continueGeneration();
}

void ChatViewModel::cancelGeneration()
{
    m_chatService->cancelGeneration();
}

void ChatViewModel::editMessage(const QString& messageId, const QString& newContent)
{
    m_chatService->editMessage(messageId, newContent);
}

void ChatViewModel::deleteMessage(const QString& messageId)
{
    m_chatService->deleteMessage(messageId);
}

void ChatViewModel::createNewSession()
{
    QString charId = m_chatService->currentCharacterId();
    if (charId.isEmpty()) return;
    QString sessionId = m_chatService->createSession(charId);
    m_chatService->switchSession(sessionId);
}

void ChatViewModel::switchSession(const QString& sessionId)
{
    //转发
    m_chatService->switchSession(sessionId);
}

void ChatViewModel::deleteAllSession()
{
    m_chatService->deleteAllSession();
}

void ChatViewModel::deleteSession(const QString& sessionId)
{
    m_chatService->deleteSession(sessionId);
}

void ChatViewModel::renameSession(const QString& sessionId, const QString& title)
{
    m_chatService->renameSession(sessionId, title);
}

void ChatViewModel::loadMoreHistory()
{
    const QString sessionId = m_chatService->currentSessionId();
    if (sessionId.isEmpty() || !m_hasMoreHistory || !m_oldestMessageTime.isValid())
        return;

    const QList<ChatMessageEntity> olderMsgs = m_chatService->loadMessagesBefore(
        sessionId, m_oldestMessageTime, m_oldestMessageId, 50);
    if (olderMsgs.isEmpty()) {
        m_hasMoreHistory = false;
        emit hasMoreHistoryChanged();
        return;
    }

    // 按时间正序插入列表头部
    for (int i = olderMsgs.size() - 1; i >= 0; --i) {
        ChatMessageItemModel* item = createItemFromEntity(olderMsgs[i]);
        m_messageModel->insertMessage(0, item);
    }

    m_oldestMessageTime = olderMsgs.first().timestamp;
    m_oldestMessageId = olderMsgs.first().id;
    m_hasMoreHistory = olderMsgs.size() == 50;
    emit hasMoreHistoryChanged();
}

// ==================== Service 信号响应槽 ====================
void ChatViewModel::onSessionSwitched(const QString& characterId, const QString& sessionId)
{
    //刷新会话列表
    refreshSessionList();

    if (m_currentSessionId != sessionId) {
        m_currentSessionId = sessionId;
        emit currentSessionIdChanged();
    }
    // 同步角色数据
    CharacterEntity character = m_characterService->getCharacterById(characterId);
    m_currentCharName = character.name;
    m_currentCharAvatarPath = character.avatarPath;
    emit currentCharNameChanged();
    emit currentCharAvatarPathChanged();

    // 同步会话标题
    ChatSessionEntity session = m_chatService->currentSession();
    m_currentSessionTitle = session.title;
    emit currentSessionTitleChanged();

    // 重置消息列表与分页
    m_messageModel->clear();
    loadInitialMessages();

    emit currentPresetChanged();
}

void ChatViewModel::onMessageAppended(const ChatMessageEntity& message)
{
    ChatMessageItemModel* item = createItemFromEntity(message);
    m_messageModel->appendMessage(item);
}

void ChatViewModel::onMessageUpdated(const QString& messageId, const QString& content)
{
    m_messageModel->updateMessageContent(messageId, content);
}

void ChatViewModel::onMessageStatusChanged(const QString& messageId, int status)
{
    m_messageModel->updateMessageStatus(messageId, status);
}

void ChatViewModel::onMessageDeleted(const QString& messageId)
{
    m_messageModel->removeMessage(messageId);
}

void ChatViewModel::onGenerationStarted(const QString& messageId)
{
    Q_UNUSED(messageId)
    m_isGenerating = true;
    emit isGeneratingChanged();
}

void ChatViewModel::onGenerationChunk(const QString& messageId, const QString& chunk)
{
    // qDebug()<<"追加流块:"<<chunk;

    m_messageModel->appendMessageContent(messageId, chunk);
}

void ChatViewModel::onGenerationFinished(const QString& messageId, const QString& fullText)
{
    Q_UNUSED(fullText)
    m_isGenerating = false;
    emit isGeneratingChanged();
    m_messageModel->updateMessageStatus(messageId,
        static_cast<int>(GlobalConstants::MessageGenerationStatus::Normal));

    // 同步会话标题
    ChatSessionEntity session = m_chatService->currentSession();
    m_currentSessionTitle = session.title;
    emit currentSessionTitleChanged();
}

void ChatViewModel::onGenerationFailed(const QString& messageId, int errorCode, const QString& errorMsg)
{
    Q_UNUSED(errorCode)
    m_isGenerating = false;
    emit isGeneratingChanged();
    emit generationError(errorMsg);
    m_messageModel->updateMessageStatus(messageId,
        static_cast<int>(GlobalConstants::MessageGenerationStatus::Failed));
}

void ChatViewModel::onGenerationCanceled(const QString& messageId)
{
    m_isGenerating = false;
    emit isGeneratingChanged();
    m_messageModel->updateMessageStatus(messageId,
        static_cast<int>(GlobalConstants::MessageGenerationStatus::Canceled));
}

// ======================================
// 内部辅助方法
// ======================================
void ChatViewModel::loadInitialMessages()
{
    const QString sessionId = m_chatService->currentSessionId();
    if (sessionId.isEmpty()) return;

    const QList<ChatMessageEntity> messages =
        m_chatService->loadLatestMessages(sessionId, 50);

    m_oldestMessageTime = messages.isEmpty()
        ? QDateTime() : messages.first().timestamp;
    m_oldestMessageId = messages.isEmpty() ? QString() : messages.first().id;
    m_hasMoreHistory = messages.size() == 50;
    emit hasMoreHistoryChanged();

    QList<ChatMessageItemModel*> items;
    items.reserve(messages.size());
    for (const ChatMessageEntity& msg : messages) {
        items.append(createItemFromEntity(msg));
    }
    m_messageModel->resetData(items);
}

void ChatViewModel::refreshSessionList()
{
    // qDebug()<<"刷新会话列表";
    QList<ChatSessionEntity> entities = m_chatService->getSessionListByCurrentCharacter();
    m_sessionModel->loadList(entities);
}

ChatMessageItemModel* ChatViewModel::createItemFromEntity(const ChatMessageEntity& entity)
{
    auto* item = new ChatMessageItemModel(m_messageModel);
    item->setId(entity.id);
    item->setMessageType(entity.type);
    item->setStatus(entity.status);
    item->setContent(entity.content);
    item->setSenderName(entity.senderName);
    item->setTimeText(Utils::formatTime(entity.timestamp));
    item->setIsEdited(entity.isEdited);
    item->setIsHidden(entity.isHidden);
    return item;
}

QString ChatViewModel::currentCharAvatarPath()
{
    return Utils::avatarPathToUrl(m_currentCharAvatarPath);
}

ChatSessionListModel* ChatViewModel::sessionModel() const
{
    return m_sessionModel;
}

// ==================== 开场白 ====================
QString ChatViewModel::greetingMessageId() const
{
    return m_chatService->greetingMessageId();
}

int ChatViewModel::greetingIndex() const
{
    return m_chatService->greetingIndex();
}

int ChatViewModel::greetingCount() const
{
    return m_chatService->greetingCount();
}

int ChatViewModel::messageCount() const
{
    return m_messageModel->rowCount();
}

void ChatViewModel::switchGreeting(int delta)
{
    m_chatService->switchGreeting(delta);
}


