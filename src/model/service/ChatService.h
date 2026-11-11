#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <QObject>
#include <QJsonArray>
#include "ChatSessionDao.h"
#include "ChatMessageDao.h"
#include "CharacterDao.h"
#include "ChatMessageEntity.h"
#include "CompletionPresetEntity.h"
#include "CharacterEntity.h"
#include "BaseApiClient.h"

class ChatService : public QObject {
    Q_OBJECT
public:
    static ChatService& instance();
    ~ChatService() override;

    ChatService(const ChatService&) = delete;
    ChatService& operator=(const ChatService&) = delete;
    ChatService(ChatService&&) = delete;
    ChatService& operator=(ChatService&&) = delete;



// ========== 启动设置 ==========
    void restoreLastSession();// 恢复上次激活的角色与会话（应用启动时调用）
// ========== 会话管理 ==========
    QString createSession(const QString& characterId);//新建会话
    void switchSession(const QString& sessionId);//切换会话
    void deleteSession(const QString& sessionId);//删除会话
    void deleteAllSession();
    ChatSessionEntity currentSession() const;//当前会话
    QString currentSessionId() const;//当前会话id
    QString currentCharacterId() const;//当前角色id
    QList<ChatSessionEntity> getSessionListByCurrentCharacter();//获取当前角色会话列表
    QList<ChatSessionEntity> getSessionListByCharacter(
        const QString& characterId);//获取角色会话列表,保留函数


    QString openCharacterSession(const QString& charId);//打开角色会话
    //保存角色当前会话为最近会话（切换会话时自动调用）
    void saveCharacterLastSession(const QString& charId, const QString& sessionId);
// ========== 消息管理 ==========
    QList<ChatMessageEntity> loadMessagePage(
        const QString& sessionId, int offset = 0, int limit = 50);//加载聊天页面
    QList<ChatMessageEntity> loadLatestMessages(const QString& sessionId, int limit = 50);
    QList<ChatMessageEntity> loadMessagesBefore(const QString& sessionId,
        const QDateTime& beforeTimestamp, const QString& beforeId, int limit = 50);
    void editMessage(const QString& messageId, const QString& newContent);//编辑消息
    void deleteMessage(const QString& messageId);//删除消息

    // ========== AI生成核心 ==========
    void sendMessage(const QString& userInput);//发送消息
    void regenerateLastMessage();//重新生成上一条消息
    void continueGeneration();//继续生成
    void cancelGeneration();//取消生成
    bool isGenerating() const;


signals:
    // 会话状态变更
    void sessionSwitched(const QString& characterId, const QString& sessionId);//会话已切换
    void sessionListChanged(const QString& characterId);//会话列表变更
    void sessionDeleted(const QString& sessionId);

    // 消息变更
    void messageAppended(const ChatMessageEntity& message);
    void messageUpdated(const QString& messageId, const QString& content);
    void messageStatusChanged(const QString& messageId, int status);//生成状态变更
    void messageDeleted(const QString& messageId);
    void messageCleared(const QString& sessionId);

    // 生成生命周期
    void generationStarted(const QString& messageId);//生成已开始
    void generationChunk(const QString& messageId, const QString& chunk);//收到流块
    void generationFinished(const QString& messageId, const QString& fullText);//生成完成
    void generationFailed(const QString& messageId, int errorCode, const QString& errorMsg);//生成失败
    void generationCanceled(const QString& messageId);

    void operationTip(const QString& tipMsg);//提示信息
    void operationError(const QString& errorMsg);//操作错误

private slots:
    void onStreamChunkReceived(const QString& chunk);//收到流式文本块
    void onCompletionFinished(const QString& fullText);//请求完成
    void onRequestFailed(const QString& error);
    void onRequestTimeout();

private:
    explicit ChatService(QObject* parent = nullptr);

    // 内部业务流程
    void startGeneration(const QList<ChatMessage>& messages);//开始生成
    QList<ChatMessage> buildPromptContext(const QString& userInput = QString());
    bool insertUserMessage(const QString& content, QString& outMessageId);//追加聊天记录
    bool insertPlaceholderAssistantMessage(QString& outMessageId);//追加助手占位符
    void finalizeMessage(const QString& messageId,
        GlobalConstants::MessageGenerationStatus status);//()

    // 工具方法
    void cleanupCurrentClient();
    void updateSessionLastModified(const QString& sessionId);//更新会话时间戳
    void updateSessionTitle();//更新会话标题
    QString getCharacterLastSessionId(const QString& charId);//读取角色上次会话ID

// DAO依赖
    CharacterDao* m_characterDao = nullptr;
    ChatSessionDao* m_sessionDao = nullptr;
    ChatMessageDao* m_messageDao = nullptr;

    // 运行时状态（唯一可信源）
    QString m_currentCharacterId;//当前角色id
    QString m_currentSessionId;//当前会话id
    QString m_generatingMessageId;//回复消息id
    QString m_fullResponse;//完整回复内容()
    bool m_isGenerating = false;//正在生成

    // 配置与网络
    BaseApiClient* m_currentClient = nullptr;
};

#endif // CHATSERVICE_H