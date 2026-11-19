#ifndef CHATVIEWMODEL_H
#define CHATVIEWMODEL_H

#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>
#include "ChatMessageEntity.h"
#include "ChatMessageListModel.h"
#include "ChatSessionListModel.h"

class QJSEngine;
class ChatService;
class CharacterService;
class CompletionPresetService;

/**
 * @brief 聊天视图模型
 * 职责：持有当前会话/角色的展示状态、消息与会话两个列表模型，把 UI 操作转发给 ChatService。
 * 本类以 QML 单例形式暴露给 QML（类型名 ChatViewModel）。
 */
class ChatViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // QML 单例工厂（供 QML 使用）：没有 create() 时 qmltyperegistrar 会把单例标记为
    // isCreatable=false，QML 侧解析不到该名字。
    static ChatViewModel* create(QQmlEngine*, QJSEngine*)
    {
        static ChatViewModel instance;
        // 生命周期归 C++：静态实例必须由我们销毁，否则退出时被销毁两次导致卡死
        QQmlEngine::setObjectOwnership(&instance, QQmlEngine::CppOwnership);
        return &instance;
    }
    //正在生成
    Q_PROPERTY(bool isGenerating READ isGenerating NOTIFY isGeneratingChanged)
    //当前角色名称
    Q_PROPERTY(QString currentCharName READ currentCharName NOTIFY currentCharNameChanged)
    //当前角色头像路径
    Q_PROPERTY(QString currentCharAvatarPath READ currentCharAvatarPath NOTIFY currentCharAvatarPathChanged FINAL)
    //当前会话标题
    Q_PROPERTY(QString currentSessionTitle READ currentSessionTitle NOTIFY currentSessionTitleChanged)
    //当前会话 id（历史抽屉按 id 高亮）
    Q_PROPERTY(QString currentSessionId READ currentSessionId NOTIFY currentSessionIdChanged)
    //是否还有更早的历史消息
    Q_PROPERTY(bool hasMoreHistory READ hasMoreHistory NOTIFY hasMoreHistoryChanged)
    // Enter 是否发送；false 时 Ctrl+Enter 发送
    Q_PROPERTY(bool sendWithEnter READ sendWithEnter NOTIFY sendWithEnterChanged)
    //聊天列表数据源
    Q_PROPERTY(ChatMessageListModel* messageModel READ messageModel CONSTANT)
    //历史会话数据源
    Q_PROPERTY(ChatSessionListModel* sessionModel READ sessionModel CONSTANT)
    //开场白：承载开场白的那条消息 id（空 = 当前会话没有可切换的开场白）
    Q_PROPERTY(QString greetingMessageId READ greetingMessageId NOTIFY greetingStateChanged FINAL)
    //当前用的是第几条（从 0 开始）
    Q_PROPERTY(int greetingIndex READ greetingIndex NOTIFY greetingStateChanged FINAL)
    //开场白总数（首条消息 + 备选）；≤1 时不显示切换控件
    Q_PROPERTY(int greetingCount READ greetingCount NOTIFY greetingStateChanged FINAL)
    //当前会话已经加载的消息条数（开场白切换只在"只剩开场白一条"时开放）
    Q_PROPERTY(int messageCount READ messageCount NOTIFY messageCountChanged FINAL)
    //最近一次生成失败的结论 + 建议（空 = 没有错误）
    Q_PROPERTY(QString lastGenerationError READ lastGenerationError NOTIFY lastGenerationErrorChanged FINAL)
    //最近一次生成失败的技术细节（分类/地址/HTTP 状态/原始错误）
    Q_PROPERTY(QString lastGenerationErrorDetail READ lastGenerationErrorDetail NOTIFY lastGenerationErrorDetailChanged FINAL)


public:
    ~ChatViewModel() override;

    bool isGenerating() const;//正在生成
    QString currentCharName() const;//角色名称
    QString currentSessionTitle() const;
    QString currentSessionId() const;
    bool hasMoreHistory() const;
    bool sendWithEnter() const;
    QString currentCharAvatarPath();//角色头像

    ChatMessageListModel* messageModel() const;
    ChatSessionListModel* sessionModel() const;

    QString greetingMessageId() const;
    int greetingIndex() const;
    int greetingCount() const;
    int messageCount() const;
    QString lastGenerationError() const;
    QString lastGenerationErrorDetail() const;



    // UI操作接口（全部转发至ChatService）
    Q_INVOKABLE void sendMessage(const QString& content);//发送消息
    Q_INVOKABLE void regenerateLastMessage();//重新生成最后消息
    Q_INVOKABLE void regenerateMessage(const QString& messageId);//重新生成指定回复（最后一条助手消息）
    Q_INVOKABLE void continueGeneration();//继续生成
    Q_INVOKABLE void cancelGeneration();//取消生成
    Q_INVOKABLE void editMessage(const QString& messageId, const QString& newContent);//编辑消息
    Q_INVOKABLE void deleteMessage(const QString& messageId);//删除消息
    Q_INVOKABLE void createNewSession();//新建会话()
    Q_INVOKABLE void switchSession(const QString& sessionId);//切换会话
    Q_INVOKABLE void deleteAllSession();//删除当前角色全部会话
    Q_INVOKABLE void deleteSession(const QString& sessionId);//删除单条会话
    Q_INVOKABLE void renameSession(const QString& sessionId, const QString& title);//重命名会话
    Q_INVOKABLE void loadMoreHistory();//加载更多历史记录
    Q_INVOKABLE void switchGreeting(int delta);//切换开场白（+1 下一条 / -1 上一条，循环）
    Q_INVOKABLE void dismissGenerationError();//关闭错误提示
    Q_INVOKABLE void copyGenerationError();//复制错误详情到剪贴板
    // 消息显示文本：只在展示时替换身份宏，数据库里仍存原始文本
    Q_INVOKABLE QString renderMessageText(const QString& raw,
                                          const QString& userName,
                                          const QString& charName) const;



signals:
    void isGeneratingChanged();
    void currentCharNameChanged();
    void currentSessionTitleChanged();
    void currentSessionIdChanged();
    void hasMoreHistoryChanged();
    void sendWithEnterChanged();
    void generationError(const QString& errorMsg);
    void lastGenerationErrorChanged();
    void lastGenerationErrorDetailChanged();

    void operationError(const QString& errorMsg);//错误信息
    void operationTip(const QString& tipMsg);//提示信息

    void currentCharAvatarPathChanged();
    void greetingStateChanged();//开场白状态（承载消息/序号/总数）变更
    void messageCountChanged();//消息条数变更

private slots:
    // 监听ChatService信号，更新视图模型
    void onSessionSwitched(const QString& characterId, const QString& sessionId);//会话变更
    void onMessageAppended(const ChatMessageEntity& message);//消息追加
    void onMessageUpdated(const QString& messageId, const QString& content);
    void onMessageStatusChanged(const QString& messageId, int status);
    void onMessageDeleted(const QString& messageId);
    void onGenerationStarted(const QString& messageId);
    void onGenerationChunk(const QString& messageId, const QString& chunk);//收到流块
    void onGenerationFinished(const QString& messageId, const QString& fullText);
    void onGenerationFailed(const QString& messageId, int errorCode, const QString& errorMsg);
    void onGenerationErrorDetail(const QString& detail);
    void onGenerationCanceled(const QString& messageId);

private:
    void loadInitialMessages();//加载初始消息
    void clearGenerationError();//清空错误提示（内部用，统一发通知）
    void refreshSessionList();//刷新会话列表
    ChatMessageItemModel* createItemFromEntity(const ChatMessageEntity& entity);//从实体创建项

    // 当前会话/角色没了（会话被删、角色被删）：把页面上的角色名、标题、消息列表一起清空
    void clearCurrentChatState();

    // 视图状态（仅用于UI展示，不做业务判断）
    bool m_isGenerating = false;//正在生成
    QString m_currentCharName;
    QString m_currentCharAvatarPath;
    QString m_currentSessionTitle;
    QString m_currentSessionId;
    QDateTime m_oldestMessageTime;
    QString m_oldestMessageId;
    bool m_hasMoreHistory = false;
    QString m_lastGenerationError;//给用户看的结论+建议
    QString m_lastGenerationErrorDetail;//技术细节

    // 列表模型
    ChatMessageListModel* m_messageModel = nullptr;
    ChatSessionListModel* m_sessionModel = nullptr;

    ChatService* m_chatService = nullptr;
    CharacterService* m_characterService = nullptr;
    CompletionPresetService* m_completionPresetService = nullptr;

    // 构造函数必须 private：QML 单例只有在"无法默认构造"时才会调用 create()。
    // 若保持 public，引擎会自己 new 一个实例，C++ 侧的静态实例就与 QML 拿到的
    // 不是同一个对象（会出现"数据写进 A、界面读的是 B"这类极难定位的问题）。
    explicit ChatViewModel(QObject* parent = nullptr);
};

#endif // CHATVIEWMODEL_H