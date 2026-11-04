#ifndef CHATMESSAGELISTMODEL_H
#define CHATMESSAGELISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "ChatMessageItemModel.h"

class ChatMessageListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        MessageTypeRole,
        StatusRole,
        ContentRole,
        SenderNameRole,
        TimeTextRole,
        IsEditedRole,
        IsHiddenRole
    };

    explicit ChatMessageListModel(QObject* parent = nullptr);
    ~ChatMessageListModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // 列表操作
    void appendMessage(ChatMessageItemModel* message);//追加消息项
    void insertMessage(int row, ChatMessageItemModel* message);
    void updateMessageContent(const QString& messageId, const QString& content);// 更新整条消息内容
    void updateMessageStatus(const QString& messageId, int status);// 更新消息状态
    void removeMessage(const QString& messageId);
    void clear();
    void resetData(const QList<ChatMessageItemModel*>& list);

    // 追加流式内容
    void appendMessageContent(const QString& messageId, const QString& chunk);


    // 查询
    ChatMessageItemModel* getMessageById(const QString& messageId) const;//获取列表项
    ChatMessageItemModel* lastMessage() const;
    int getMessageIndex(const QString& messageId) const;//获取列表项索引

private:
    QList<ChatMessageItemModel*> m_messages;
};

#endif // CHATMESSAGELISTMODEL_H