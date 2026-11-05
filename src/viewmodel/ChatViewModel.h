#ifndef CHATVIEWMODEL_H
#define CHATVIEWMODEL_H

#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>
#include "ChatMessageEntity.h"
#include "ChatMessageListModel.h"
#include "ChatSessionListModel.h"

class QJSEngine;

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
    //当前预设名称
    Q_PROPERTY(QString currentPresetName READ currentPresetName NOTIFY currentPresetChanged)
    //聊天列表数据源
    Q_PROPERTY(ChatMessageListModel* messageModel READ messageModel CONSTANT)
    //历史会话数据源
    Q_PROPERTY(ChatSessionListModel* sessionModel READ sessionModel CONSTANT)


public:
    explicit ChatViewModel(QObject* parent = nullptr);
    ~ChatViewModel() override;

    bool isGenerating() const;//正在生成
    QString currentCharName() const;//角色名称
    QString currentSessionTitle() const;
    QString currentPresetName() const;//预设名称
    QString currentCharAvatarPath();//角色头像

    ChatMessageListModel* messageModel() const;
    ChatSessionListModel* sessionModel() const;



    // UI操作接口（全部转发至ChatService）
    Q_INVOKABLE void sendMessage(const QString& content);//发送消息
    Q_INVOKABLE void regenerateLastMessage();//重新生成最后消息
    Q_INVOKABLE void continueGeneration();//继续生成
    Q_INVOKABLE void cancelGeneration();//取消生成
    Q_INVOKABLE void editMessage(const QString& messageId, const QString& newContent);//编辑消息
    Q_INVOKABLE void deleteMessage(const QString& messageId);//删除消息
    Q_INVOKABLE void switchCharacter(const QString& charId);//切换角色(无用,待优化)
    Q_INVOKABLE void createNewSession();//新建会话()
    Q_INVOKABLE void switchSession(const QString& sessionId);//切换会话
    Q_INVOKABLE void deleteAllSession();//删除当前角色全部会话
    Q_INVOKABLE void loadMoreHistory();//加载更多历史记录



signals:
    void isGeneratingChanged();
    void currentCharNameChanged();
    void currentSessionTitleChanged();
    void currentPresetChanged();
    void generationError(const QString& errorMsg);

    void operationError(const QString& errorMsg);//错误信息
    void operationTip(const QString& tipMsg);//提示信息

    void currentCharAvatarPathChanged();

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
    void onGenerationCanceled(const QString& messageId);

private:
    void loadInitialMessages();//加载初始消息
    void refreshSessionList();//刷新会话列表
    ChatMessageItemModel* createItemFromEntity(const ChatMessageEntity& entity);//从实体创建项

    // 视图状态（仅用于UI展示，不做业务判断）
    bool m_isGenerating = false;//正在生成
    QString m_currentCharName;
    QString m_currentCharAvatarPath;
    QString m_currentSessionTitle;
    QString m_currentPresetName;
    int m_historyLoadOffset = 0;//历史负载偏移

    // 列表模型
    ChatMessageListModel* m_messageModel = nullptr;
    ChatSessionListModel* m_sessionModel = nullptr;
};

#endif // CHATVIEWMODEL_H