#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <QObject>
#include "ChatSessionEntity.h"
#include "ChatMessageDao.h"
#include "CharacterDao.h"
#include "ChatMessageEntity.h"
#include "ChatMessage.h"
#include "TransactionScope.h"

class CharacterService;
class SettingsService;
class ChatCompletionRunner;
class GreetingService;
class SessionService;

// ChatService 的全部协作者。生产由 ChatServiceAssembly.cpp 组装，
// 单测按需替换（假补全器、假事务、内存库 DAO）。
// 约定：这里传进来的一律只借用，不接管所有权。
struct ChatServiceDependencies {
    SessionService* sessions = nullptr;            // 会话表 + 当前会话指针
    ChatMessageDao* messageDao = nullptr;
    CharacterDao* characterDao = nullptr;
    CharacterService* characterService = nullptr;
    SettingsService* settingsService = nullptr;
    ChatCompletionRunner* completion = nullptr;    // 补全请求的出口（网络/配置）
    TransactionScope* transaction = nullptr;       // 事务边界
    GreetingService* greeting = nullptr;           // 开场白状态
};

class ChatService : public QObject {
    Q_OBJECT
public:
    static ChatService& instance();
    ~ChatService() override;

    // 测试/组合根用：协作者全部注入（见 ChatServiceDependencies）。
    explicit ChatService(const ChatServiceDependencies& deps, QObject* parent = nullptr);

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
// ========== 消息管理 ==========
    QList<ChatMessageEntity> loadMessagePage(
        const QString& sessionId, int offset = 0, int limit = 50);//加载聊天页面
    QList<ChatMessageEntity> loadLatestMessages(const QString& sessionId, int limit = 50);
    QList<ChatMessageEntity> loadMessagesBefore(const QString& sessionId,
        const QDateTime& beforeTimestamp, const QString& beforeId, int limit = 50);
    void editMessage(const QString& messageId, const QString& newContent);//编辑消息
    void deleteMessage(const QString& messageId);//删除消息

    // ========== 开场白切换 ==========
    // 状态由 GreetingService 维护，这里只做转发（QML 依赖这三个属性不变）。
    // 开场白集合 = [角色首条消息] + 备选开场白；切换替换承载消息的内容，不新增消息。
    QString greetingMessageId() const;
    int greetingIndex() const;
    int greetingCount() const;
    void switchGreeting(int delta);//delta = +1 / -1，循环取模

    // ========== AI生成核心 ==========
    void sendMessage(const QString& userInput);//发送消息
    void regenerateLastMessage();//重新生成最后一条消息（保留给旧调用方）
    void regenerateMessage(const QString& messageId);//重新生成指定消息（必须是会话最后一条助手回复）
    void continueGeneration();//继续生成
    void cancelGeneration();//取消生成
    bool isGenerating() const;


signals:
    // 会话状态变更
    void sessionSwitched(const QString& characterId, const QString& sessionId);//会话已切换
    void sessionListChanged(const QString& characterId);//会话列表变更
    void sessionDeleted(const QString& sessionId);
    // 当前角色被删除，内存里的会话指针已清空（会话与消息由外键级联删除）。
    // 与 sessionDeleted 分开：这里没有一个"仍然存在的会话"可以指代。
    void characterClosed(const QString& characterId);

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
    // 失败的技术细节（分类/地址/HTTP 状态/原始错误），先于 generationFailed 发出
    void generationErrorDetail(const QString& detail);
    void generationCanceled(const QString& messageId);

    void operationTip(const QString& tipMsg);//提示信息
    void operationError(const QString& errorMsg);//操作错误

private slots:
    // 角色被删除（CharacterService::characterDeleted）：删的若是当前角色，
    // 清空内存里的当前会话指针，否则聊天页会停在一个已经不存在的角色上。
    void onCharacterDeleted(const QString& characterId);

    // 补全器（生成请求）的三个回调，语义与旧版网络槽一一对应
    void onCompletionChunk(const QString& chunk);//收到流式文本块
    void onCompletionFinished(const QString& fullText);//请求完成
    void onCompletionFailed(const QString& error);//请求失败

private:
    // 仅 instance() 使用：在 ChatServiceComposition.cpp 里组装生产依赖
    explicit ChatService(QObject* parent = nullptr);

    // 内部业务流程
    bool insertUserMessage(const QString& content, QString& outMessageId);//追加聊天记录
    bool insertPlaceholderAssistantMessage(QString& outMessageId);//追加助手占位符

    // 旧版本按回复前 10 字符生成的标题可能留下 <content>\n... 这类残片；
    // 打开角色时按首条用户消息修一次，只匹配明确的旧格式，不碰用户手写标题。
    void repairLegacySessionTitles(const QString& characterId);

    // 构建提示词 → 启动补全。
    // 提示词构建失败（如固定提示词超出上下文预算，对应 ST 的 TokenBudgetExceededError）
    // 时把占位消息标记为失败并发 generationFailed，不再把空消息丢给补全器。
    bool startGeneration(const QString& userInput, const QString& generationType,
                         const QString& assistantMsgId);
    void failGeneration(const QString& assistantMsgId, const QString& error);

// DAO依赖
    CharacterDao* m_characterDao = nullptr;
    ChatMessageDao* m_messageDao = nullptr;

    // 会话服务（借用，不持有）：会话表 + 当前会话指针
    SessionService* m_sessions = nullptr;

    // 业务服务依赖（借用，不持有）
    CharacterService* m_characterService = nullptr;
    SettingsService* m_settingsService = nullptr;

    // 补全出口 / 事务边界 / 开场白（借用，不持有）
    ChatCompletionRunner* m_completion = nullptr;
    TransactionScope* m_transaction = nullptr;
    GreetingService* m_greeting = nullptr;

    // 运行时状态（唯一可信源）
    QString m_generatingMessageId;//回复消息id
    QString m_fullResponse;//完整回复内容()
    QString m_lastErrorDetail;//最近一次失败的技术细节（failedDetail 先到，generationFailed 时消费）
    bool m_isGenerating = false;//正在生成
};

#endif // CHATSERVICE_H