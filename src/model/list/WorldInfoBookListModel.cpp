#include "WorldInfoBookListModel.h"
#include "WorldInfoBookItemModel.h"

WorldInfoBookListModel::WorldInfoBookListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

WorldInfoBookListModel::~WorldInfoBookListModel()
{
    qDeleteAll(m_items);
    m_items.clear();
}

int WorldInfoBookListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return m_items.size();
}

QVariant WorldInfoBookListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    WorldInfoBookItemModel* item = m_items.at(index.row());
    switch (role)
    {
    case IdRole:
        return item->id();
    case NameRole:
        return item->name();
    case DescRole:
        return item->description();
    case EnabledRole:
        return item->isEnabled();
    case GlobalRole:
        return item->isGlobal();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> WorldInfoBookListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[DescRole] = "description";
    roles[EnabledRole] = "isEnabled";
    roles[GlobalRole] = "isGlobal";
    return roles;
}

void WorldInfoBookListModel::resetData(const QList<WorldInfoBookItemModel*>& list)
{
    beginResetModel();
    qDeleteAll(m_items);
    m_items = list;
    endResetModel();
}

void WorldInfoBookListModel::clear()
{
    beginResetModel();
    qDeleteAll(m_items);
    m_items.clear();
    endResetModel();
}

WorldInfoBookItemModel* WorldInfoBookListModel::getItemById(const QString& id) const
{
    for (auto item : m_items)
    {
        if (item->id() == id)
            return item;
    }
    return nullptr;
}