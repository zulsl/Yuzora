#include "CharacterListModel.h"
#include "CharacterItemViewModel.h"
#include "Utils.h"

CharacterListModel::CharacterListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

CharacterListModel::~CharacterListModel()
{
    clear();
}

int CharacterListModel::rowCount(const QModelIndex & parent) const
{
    // 无效父节点返回0，符合Qt模型视图规范
    if (parent.isValid()) {
        return 0;
    }
    return m_items.size();
}

QVariant CharacterListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return QVariant();
    }

    CharacterItemViewModel* item = m_items.at(index.row());
    switch (role) {
    case IdRole:            return item->id();
    case NameRole:          return item->name();
    case DescriptionRole:   return item->description();
    case AvatarPathRole:    return item->avatarPath();
    case IsFavoriteRole:    return item->isFavorite();
    case UpdateTimeRole:    return item->updateTimeText();
    default:                return QVariant();
    }
}

QHash<int, QByteArray> CharacterListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole]            = "id";
    roles[NameRole]          = "name";
    roles[DescriptionRole]   = "description";
    roles[AvatarPathRole]    = "avatarPath";
    roles[IsFavoriteRole]    = "isFavorite";
    roles[UpdateTimeRole]    = "updateTimeText";
    return roles;
}

void CharacterListModel::appendItem(CharacterItemViewModel* item)
{
    if (!item) return;
    const int row = m_items.size();
    beginInsertRows(QModelIndex(), row, row);
    m_items.append(item);
    endInsertRows();
}

void CharacterListModel::insertItem(int row, CharacterItemViewModel* item)
{
    if (!item || row < 0 || row > m_items.size()) return;
    beginInsertRows(QModelIndex(), row, row);
    m_items.insert(row, item);
    endInsertRows();
}

void CharacterListModel::updateItem(const QString& charId)
{
    int row = indexOfId(charId);
    if (row < 0) return;
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx);
}

void CharacterListModel::updateItem(const CharacterEntity& entity)
{
    const int row = indexOfId(entity.id);
    if (row < 0) return;

    m_items[row]->updateFromEntity(entity);
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx);
}

void CharacterListModel::removeItem(const QString& charId)
{
    int row = indexOfId(charId);
    if (row < 0) return;

    beginRemoveRows(QModelIndex(), row, row);
    CharacterItemViewModel* item = m_items.takeAt(row);
    item->deleteLater();
    endRemoveRows();
}

void CharacterListModel::clear()
{
    if (m_items.isEmpty()) return;

    beginResetModel();
    qDeleteAll(m_items);
    m_items.clear();
    endResetModel();
}

void CharacterListModel::loadList(const QList<CharacterItemViewModel*>& list)
{
    beginResetModel();
    // 释放旧数据内存，避免泄漏
    qDeleteAll(m_items);
    m_items = list;
    endResetModel();
}

CharacterItemViewModel* CharacterListModel::getItemById(const QString& charId) const
{
    int idx = indexOfId(charId);
    return idx >= 0 ? m_items.at(idx) : nullptr;
}

CharacterItemViewModel* CharacterListModel::lastItem() const
{
    return m_items.isEmpty() ? nullptr : m_items.last();
}

int CharacterListModel::rowById(const QString& charId) const
{
    return indexOfId(charId);
}

// 内部辅助：按ID查找索引
int CharacterListModel::indexOfId(const QString& charId) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i)->id() == charId) {
            return i;
        }
    }
    return -1;
}