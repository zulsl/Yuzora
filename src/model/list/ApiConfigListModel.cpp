#include "ApiConfigListModel.h"

ApiConfigListModel::ApiConfigListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ApiConfigListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_configs.size();
}

QVariant ApiConfigListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_configs.size())
        return QVariant();

    const ApiConfigEntity &config = m_configs.at(index.row());

    switch (role) {
    case IdRole:
        return config.id;
    case NameRole:
        return config.name;
    case ModelTypeRole:
        return static_cast<int>(config.modelType);
    case BaseUrlRole:
        return config.baseDomain;
    case IsOpenAICompatibleRole:
        return config.isOpenAICompatible;
    case IsActiveRole:
        return config.isActive;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ApiConfigListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[ModelTypeRole] = "modelType";
    roles[BaseUrlRole] = "baseUrl";
    roles[IsOpenAICompatibleRole] = "isOpenAICompatible";
    roles[IsActiveRole] = "isActive";
    return roles;
}

// ==================== 列表操作接口 ====================

void ApiConfigListModel::loadList(const QList<ApiConfigEntity> &list)
{
    beginResetModel();
    m_configs = list;
    endResetModel();
}

void ApiConfigListModel::addItem(const ApiConfigEntity &item)
{
    const int row = m_configs.size();
    beginInsertRows(QModelIndex(), row, row);
    m_configs.append(item);
    endInsertRows();
}

void ApiConfigListModel::updateItem(const ApiConfigEntity &item)
{
    const int row = indexOfId(item.id);
    if (row < 0)
        return;

    m_configs[row] = item;
    emit dataChanged(index(row, 0), index(row, 0));
}

void ApiConfigListModel::removeItem(const QString &id)
{
    const int row = indexOfId(id);
    if (row < 0)
        return;

    beginRemoveRows(QModelIndex(), row, row);
    m_configs.removeAt(row);
    endRemoveRows();
}

void ApiConfigListModel::clear()
{
    beginResetModel();
    m_configs.clear();
    endResetModel();
}

// ==================== 私有工具方法 ====================

int ApiConfigListModel::indexOfId(const QString &id) const
{
    for (int i = 0; i < m_configs.size(); ++i) {
        if (m_configs.at(i).id == id) {
            return i;
        }
    }
    return -1;
}