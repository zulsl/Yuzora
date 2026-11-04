#include "ChatViewModel.h"
#include "ChatService.h"
#include "ChatMessageItemModel.h"
#include "CharacterService.h"
#include "Utils.h"
#include "GlobalConstants.h"

#include <QDir>

ChatViewModel::ChatViewModel(QObject* parent)
    : QObject(parent)
    , m_messageModel(new ChatMessageListModel(this))
    , m_sessionModel(new ChatSessionListModel(this))
{
    ChatService& chatService = ChatService::instance();

    // 会话状态信号连接
    connect(&chatService, &ChatService::sessionSwitched,
            this, &ChatViewModel::onSessionSwitched);
    connect(&chatService, &ChatService::sessionDeleted,
            this, [this](const QString& sessionId) {
                if (sessionId == ChatService::instance().currentSessionId()) {
                    m_messageModel->clear();
                    m_currentCharName.clear();
                    m_currentCharAvatarPath.clear();
                    m_currentSessionTitle.clear();
                    m_currentPresetName.clear();
                    m_historyLoadOffset = 0;
                    emit currentCharNameChanged();
                    emit currentSessionTitleChanged();
                    emit currentPresetChanged();
                }
            });

    // 消息变更信号连接
    connect(&chatService, &ChatService::messageAppended,
            this, &ChatViewModel::onMessageAppended);
    connect(&chatService, &ChatService::messageUpdated,
            this, &ChatViewModel::onMessageUpdated);
    connect(&chatService, &ChatService::messageStatusChanged,
            this, &ChatViewModel::onMessageStatusChanged);
    connect(&chatService, &ChatService::messageDeleted,
            this, &ChatViewModel::onMessageDeleted);

    // 生成生命周期信号连接
    connect(&chatService, &ChatService::generationStarted,
            this, &ChatViewModel::onGenerationStarted);
    connect(&chatService, &ChatService::generationChunk,
            this, &ChatViewModel::onGenerationChunk);
    connect(&chatService, &ChatService::generationFinished,
            this, &ChatViewModel::onGenerationFinished);
    connect(&chatService, &ChatService::generationFailed,
            this, &ChatViewModel::onGenerationFailed);
    connect(&chatService, &ChatService::generationCanceled,
            this, &ChatViewModel::onGenerationCanceled);

    connect(&chatService, &ChatService::operationTip,
            this, &ChatViewModel::operationTip);
    connect(&chatService, &ChatService::operationError,
            this, &ChatViewModel::operationError);


    //加载最后会话角色
    chatService.restoreLastSession();

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

ChatMessageListModel* ChatViewModel::messageModel() const
{
    return m_messageModel;
}

#include "CompletionPresetService.h"
QString ChatViewModel::currentPresetName() const
{
    return CompletionPresetService::instance().getActivePreset().name;
    // return m_currentPresetName;
}

// ======================================
// UI 操作接口（全部转发至 ChatService）
// ======================================
void ChatViewModel::sendMessage(const QString& content)
{
    QString text = content.trimmed();
    ChatService::instance().sendMessage(text);
}

void ChatViewModel::regenerateLastMessage()
{
    if (m_isGenerating) return;
    ChatService::instance().regenerateLastMessage();

    // ChatService::instance().regenerateLastMessage(m_currentCharId, m_currentSessionId);
}

void ChatViewModel::continueGeneration()
{
    if (m_isGenerating) return;
    ChatService::instance().continueGeneration();
}

void ChatViewModel::cancelGeneration()
{
    ChatService::instance().cancelGeneration();
}

void ChatViewModel::editMessage(const QString& messageId, const QString& newContent)
{
    ChatService::instance().editMessage(messageId, newContent);
}

void ChatViewModel::deleteMessage(const QString& messageId)
{
    ChatService::instance().deleteMessage(messageId);
}

void ChatViewModel::switchCharacter(const QString& charId)
{
    //(待优化)
    if (charId.isEmpty()) return;
    // 切换角色时自动创建新会话
    QString sessionId = ChatService::instance().createSession(charId);
    if (!sessionId.isEmpty()) {
        ChatService::instance().switchSession(sessionId);
    }
}

void ChatViewModel::createNewSession()
{
    QString charId = ChatService::instance().currentCharacterId();
    if (charId.isEmpty()) return;
    QString sessionId = ChatService::instance().createSession(charId);
    ChatService::instance().switchSession(sessionId);
}

void ChatViewModel::switchSession(const QString& sessionId)
{
    //转发
    ChatService::instance().switchSession(sessionId);
}

void ChatViewModel::deleteAllSession()
{
    ChatService::instance().deleteAllSession();
}

void ChatViewModel::loadMoreHistory()
{
    ChatService& service = ChatService::instance();
    QString sessionId = service.currentSessionId();
    if (sessionId.isEmpty()) return;

    QList<ChatMessageEntity> olderMsgs = service.loadMessagePage(
        sessionId, m_historyLoadOffset, 50);
    if (olderMsgs.isEmpty()) return;

    // 按时间正序插入列表头部
    for (int i = olderMsgs.size() - 1; i >= 0; --i) {
        ChatMessageItemModel* item = createItemFromEntity(olderMsgs[i]);
        m_messageModel->insertMessage(0, item);
    }
    m_historyLoadOffset += olderMsgs.size();
}

// ==================== Service 信号响应槽 ====================
void ChatViewModel::onSessionSwitched(const QString& characterId, const QString& sessionId)
{
    //刷新会话列表
    refreshSessionList();

    Q_UNUSED(sessionId)
    // 同步角色数据
    CharacterEntity character = CharacterService::instance().getCharacterById(characterId);
    m_currentCharName = character.name;
    m_currentCharAvatarPath = character.avatarPath;
    emit currentCharNameChanged();
    emit currentCharAvatarPathChanged();

    // 同步会话标题
    ChatSessionEntity session = ChatService::instance().currentSession();
    m_currentSessionTitle = session.title;
    emit currentSessionTitleChanged();

    // 重置消息列表与分页
    m_messageModel->clear();
    m_historyLoadOffset = 0;
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
    ChatSessionEntity session = ChatService::instance().currentSession();
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
    ChatService& service = ChatService::instance();
    QString sessionId = service.currentSessionId();
    if (sessionId.isEmpty()) return;

    QList<ChatMessageEntity> messages = service.loadMessagePage(sessionId, 0, 50);
    m_historyLoadOffset = messages.size();

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
    QList<ChatSessionEntity> entities = ChatService::instance().getSessionListByCurrentCharacter();
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


