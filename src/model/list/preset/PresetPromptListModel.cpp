#include "PresetPromptListModel.h"

PresetPromptListModel::PresetPromptListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

PresetPromptListModel::~PresetPromptListModel()
{
    clear();
}

int PresetPromptListModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

QVariant PresetPromptListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()){
        return QVariant();
    }

    const PromptEntity& item = m_items.at(index.row());

    switch (role) {
        case IdRole:
            return item.id;
        case IdentifierRole:
            return item.identifier;
        case NameRole:
            return item.name;
        case MessageTypeRole:
            return item.role;
        case ContentRole:
            return item.content;
        case TriggerTypeRole:
            return item.injection_trigger;
        case PositionRole:
            return item.injection_position;
        case IsActiveRole:
            return item.isActive;
        case DepthRole:
            return item.injection_depth;
        case SortOrderRole:
            return item.injection_order;
        case ExpandedStateRole:
            return m_expandedIds.contains(item.id);
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> PresetPromptListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[IdentifierRole] = "identifier";
    roles[NameRole] = "name";
    roles[MessageTypeRole] = "role";
    roles[ContentRole] = "content";
    roles[TriggerTypeRole] = "triggerType";
    roles[PositionRole] = "position";
    roles[IsActiveRole] = "isActive";
    roles[DepthRole] = "depth";
    roles[SortOrderRole] = "order";
    roles[ExpandedStateRole] = "expandedState";
    return roles;
}

bool PresetPromptListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return false;

    PromptEntity& e = m_items[index.row()];
    switch (role)
    {
    // ========== 业务字段：改内存 + 通知外部落盘 ==========
        case NameRole: {
            const QString v = value.toString().trimmed();
            if (e.name == v) return false;
            e.name = v;
            emit dataChanged(index, index, { NameRole});
            break;
        }
        case ContentRole: {
            const QString v = value.toString();
            if (e.content == v) return false;
            e.content = v;
            emit dataChanged(index, index, { ContentRole });
            break;
        }
        case MessageTypeRole: {
            const GlobalConstants::MessageType v =
                static_cast<GlobalConstants::MessageType>(value.toInt());
            if (e.role == v) return false;
            e.role = v;
            emit dataChanged(index, index, { MessageTypeRole });
            break;
        }
        case PositionRole: {
            const int v = value.toInt();
            if (e.injection_position == v) return false;
            e.injection_position = v;
            emit dataChanged(index, index, { PositionRole });
            break;
        }
        case IsActiveRole: {
            const bool v = value.toBool();
            if (e.isActive == v) return false;
            e.isActive = v;
            emit dataChanged(index, index, { IsActiveRole });
            break;
        }
        // ========== UI 状态：改内存，不落盘 ==========
        case ExpandedStateRole: {
            const bool v = value.toBool();
            const bool current = m_expandedIds.contains(e.id);
            if (current == v) return false;
            if (v) m_expandedIds.insert(e.id);
            else   m_expandedIds.remove(e.id);
            emit dataChanged(index, index, { ExpandedStateRole });
            return true;
            break;
        }
        default:
            return false;
    }

    updateItem(e);
    emit entryWriting(e);
    return true;
}

void PresetPromptListModel::loadList(const QList<PromptEntity>& list)
{
    qDebug() << "loadList: before reset, items =" << m_items.size();
    beginResetModel();
    m_items.clear();
    m_items = list;
    endResetModel();
    qDebug() << "loadList: after reset, items =" << m_items.size();
}

void PresetPromptListModel::appendItem(PromptEntity& item)
{
    // if (!item) return;
    // const int row = m_items.size();
    // beginInsertRows(QModelIndex(), row, row);
    // m_items.append(item);
    // endInsertRows();
}

void PresetPromptListModel::removeItem(const QString& id)
{
    // int row = indexOfId(id);
    // if (row < 0) return;

    // beginRemoveRows(QModelIndex(), row, row);
    // PresetPromptItemModel* item = m_items.takeAt(row);
    // item->deleteLater();
    // endRemoveRows();
}

void PresetPromptListModel::updateItem(const PromptEntity& entry)
{
    const int row = indexOfId(entry.id);
    if (row < 0) return;    //不在列表里,返回(可当作新增)

    m_items[row] = entry;

    QModelIndex idx = index(row);
    emit dataChanged(idx, idx);
}

void PresetPromptListModel::clear()
{
    // if (m_items.isEmpty()) return;

    // beginResetModel();
    // qDeleteAll(m_items);
    // m_items.clear();
    // endResetModel();
}

int PresetPromptListModel::indexOfId(const QString& id) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).id == id) {
            return i;
        }
    }
    return -1;
}
