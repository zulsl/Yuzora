#include "ChatSessionListModel.h"


ChatSessionListModel::ChatSessionListModel(QObject *parent)
    : QAbstractListModel{parent}
{

}

int ChatSessionListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

QVariant ChatSessionListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    const ChatSessionEntity &entity = m_items.at(index.row());

    switch (role) {
    case IdRole:
        return entity.id;
    case CharacterIdRole:
        return entity.characterId;
    case TitleRole:
        return entity.title;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ChatSessionListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[CharacterIdRole] = "characterId";
    roles[TitleRole] = "title";
    return roles;
}

void ChatSessionListModel::loadList(const QList<ChatSessionEntity> &list)
{
    beginResetModel();
    m_items = list;
    endResetModel();
}

void ChatSessionListModel::addItem(const ChatSessionEntity &item)
{
    const int row = m_items.size();
    beginInsertRows(QModelIndex(), row, row);
    m_items.append(item);
    endInsertRows();
}

void ChatSessionListModel::updateItem(const ChatSessionEntity &item)
{
    const int row = indexOfId(item.id);
    if (row < 0)
        return;

    m_items[row] = item;
    emit dataChanged(index(row, 0), index(row, 0));
}

void ChatSessionListModel::removeItem(const QString &id)
{
    const int row = indexOfId(id);
    if (row < 0)
        return;

    beginRemoveRows(QModelIndex(), row, row);
    m_items.removeAt(row);
    endRemoveRows();
}

void ChatSessionListModel::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

int ChatSessionListModel::indexOfId(const QString& id) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).id == id) {
            return i;
        }
    }
    return -1;
}
