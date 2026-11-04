#include "ChatMessageListModel.h"
#include "ChatMessageItemModel.h"
#include "GlobalConstants.h"

// ======================================
// 构造与析构
// ======================================
ChatMessageListModel::ChatMessageListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

ChatMessageListModel::~ChatMessageListModel() = default;

// ======================================
// 模型基础接口实现
// ======================================
int ChatMessageListModel::rowCount(const QModelIndex& parent) const
{
    // 列表模型无子项，有效父索引返回0行
    if (parent.isValid()) {
        return 0;
    }
    return m_messages.size();
}

QVariant ChatMessageListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_messages.size()) {
        return QVariant();
    }

    const ChatMessageItemModel* item = m_messages.at(index.row());
    switch (role) {
    case IdRole:
        return item->id();
    case MessageTypeRole:
        return item->messageType();
    case StatusRole:
        return item->status();
    case ContentRole:
        return item->content();
    case SenderNameRole:
        return item->senderName();
    case TimeTextRole:
        return item->timeText();
    case IsEditedRole:
        return item->isEdited();
    case IsHiddenRole:
        return item->isHidden();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ChatMessageListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[MessageTypeRole] = "messageType";
    roles[StatusRole] = "status";
    roles[ContentRole] = "content";
    roles[SenderNameRole] = "senderName";
    roles[TimeTextRole] = "timeText";
    roles[IsEditedRole] = "isEdited";
    roles[IsHiddenRole] = "isHidden";
    return roles;
}

// ======================================
// 列表增量操作
// ======================================
void ChatMessageListModel::appendMessage(ChatMessageItemModel* message)
{
    if (!message) return;

    const int row = m_messages.size();
    beginInsertRows(QModelIndex(), row, row);
    m_messages.append(message);
    endInsertRows();
}

void ChatMessageListModel::insertMessage(int row, ChatMessageItemModel* message)
{
    if (!message || row < 0 || row > m_messages.size()) return;

    beginInsertRows(QModelIndex(), row, row);
    m_messages.insert(row, message);
    endInsertRows();
}

void ChatMessageListModel::updateMessageContent(const QString& messageId, const QString& content)
{
    const int row = getMessageIndex(messageId);
    if (row < 0) return;

    ChatMessageItemModel* item = m_messages[row];
    if (item->content() == content) return;

    item->setContent(content);
    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, { ContentRole });
}

void ChatMessageListModel::updateMessageStatus(const QString& messageId, int status)
{
    const int row = getMessageIndex(messageId);
    if (row < 0) return;

    ChatMessageItemModel* item = m_messages[row];
    if (item->status() == status) return;

    item->setStatus(static_cast<GlobalConstants::MessageGenerationStatus>(status));
    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, { StatusRole });
}

void ChatMessageListModel::removeMessage(const QString& messageId)
{
    const int row = getMessageIndex(messageId);
    if (row < 0) return;

    beginRemoveRows(QModelIndex(), row, row);
    ChatMessageItemModel* item = m_messages.takeAt(row);
    item->deleteLater();
    endRemoveRows();
}

void ChatMessageListModel::clear()
{
    if (m_messages.isEmpty()) return;

    beginResetModel();
    qDeleteAll(m_messages);
    m_messages.clear();
    endResetModel();
}

void ChatMessageListModel::resetData(const QList<ChatMessageItemModel*>& list)
{
    beginResetModel();
    qDeleteAll(m_messages);
    m_messages = list;
    endResetModel();
}

void ChatMessageListModel::appendMessageContent(const QString &messageId, const QString &chunk)
{
    int row = getMessageIndex(messageId);
    if (row < 0) return;

    // 1. 修改底层数据
    m_messages[row]->appendContent(chunk);

    // 2. 通知视图：这一行的 content 角色数据变了，请刷新
    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {ContentRole});
}

// ======================================
// 查询工具方法
// ======================================
ChatMessageItemModel* ChatMessageListModel::getMessageById(const QString& messageId) const
{
    for (ChatMessageItemModel* item : m_messages) {
        if (item->id() == messageId) {
            return item;
        }
    }
    return nullptr;
}

ChatMessageItemModel* ChatMessageListModel::lastMessage() const
{
    if (m_messages.isEmpty()) {
        return nullptr;
    }
    return m_messages.last();
}

int ChatMessageListModel::getMessageIndex(const QString& messageId) const
{
    for (int i = 0; i < m_messages.size(); ++i) {
        if (m_messages[i]->id() == messageId) {
            return i;
        }
    }
    return -1;
}