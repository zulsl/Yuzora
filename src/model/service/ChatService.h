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
    // 重命名会话：trim 后为空直接拒绝；超长按 40 字符截断
    bool renameSession(const QString& sessionId, const QString& title);
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

    // ========== 开场白切换 ==========
    // 开场白集合 = [角色首条消息] + 备选开场白（各自过滤掉空串）。
    // 会话的第一条 assistant 消息用 metadata.greetingIndex 记住自己用的是第几条，
    // 切换就是替换这条消息的内容，不新增消息。
    QString greetingMessageId() const { return m_greetingMessageId; }
    int greetingIndex() const { return m_greetingIndex; }
    int greetingCount() const { return m_greetingCount; }
    void switchGreeting(int delta);//delta = +1 / -1，循环取模

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

    // 开场白状态变更（切换会话、新建会话、切换开场白后都会发）
    void greetingStateChanged();

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

    // 开场白：拼集合、刷新当前会话的开场白状态、把第 index 条写进某条消息
    QStringList greetingTextsFor(const QString& characterId) const;
    void refreshGreetingState();
    void applyGreetingIndex(const QString& messageId, int index);

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

    // 开场白状态（-1 / 空串 = 当前会话没有可切换的开场白）
    QString m_greetingMessageId;
    int m_greetingIndex = -1;
    int m_greetingCount = 0;

    // 配置与网络
    BaseApiClient* m_currentClient = nullptr;
};

#endif // CHATSERVICE_H