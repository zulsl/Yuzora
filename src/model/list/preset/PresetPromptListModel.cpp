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
        case SystemPromptRole:
            return item.system_prompt;
        case MarkerRole:
            return item.marker;
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
    roles[SystemPromptRole] = "systemPrompt";
    roles[MarkerRole] = "marker";
    roles[ExpandedStateRole] = "expandedState";
    return roles;
}

bool PresetPromptListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return false;

    // 业务字段只能通过 CompletionPresetViewModel 的命令更新；
    // 模型自身仅保存展开态这类 UI 状态。
    if (role != ExpandedStateRole) {
        return false;
    }

    PromptEntity& e = m_items[index.row()];
    const bool v = value.toBool();
    const bool current = m_expandedIds.contains(e.id);
    if (current == v) {
        return false;
    }

    if (v) {
        m_expandedIds.insert(e.id);
    } else {
        m_expandedIds.remove(e.id);
    }
    emit dataChanged(index, index, { ExpandedStateRole });
    return true;
}

bool PresetPromptListModel::updateField(const QString& id, int role, const QVariant& value)
{
    const int row = indexOfId(id);
    if (row < 0) {
        return false;
    }

    PromptEntity& e = m_items[row];
    switch (role) {
    case NameRole: {
        const QString v = value.toString().trimmed();
        if (e.name == v) return false;
        e.name = v;
        break;
    }
    case ContentRole: {
        const QString v = value.toString();
        if (e.content == v) return false;
        e.content = v;
        break;
    }
    case MessageTypeRole: {
        const auto v = static_cast<GlobalConstants::MessageType>(value.toInt());
        if (e.role == v) return false;
        e.role = v;
        break;
    }
    case PositionRole: {
        const int v = value.toInt();
        if (e.injection_position == v) return false;
        e.injection_position = v;
        break;
    }
    case IsActiveRole: {
        const bool v = value.toBool();
        if (e.isActive == v) return false;
        e.isActive = v;
        break;
    }
    default:
        return false;
    }

    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, { role });
    return true;
}

void PresetPromptListModel::loadList(const QList<PromptEntity>& list)
{
    beginResetModel();
    m_items.clear();
    m_items = list;
    endResetModel();
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

PromptEntity PresetPromptListModel::entryById(const QString& id) const
{
    const int row = indexOfId(id);
    return row < 0 ? PromptEntity{} : m_items.at(row);
}

void PresetPromptListModel::clear()
{
    if (m_items.isEmpty() && m_expandedIds.isEmpty()) {
        return;
    }

    beginResetModel();
    m_items.clear();
    m_expandedIds.clear();
    endResetModel();
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

bool PresetPromptListModel::moveRow(int from, int to)
{
    if (from == to)
        return false;
    if (from < 0 || from >= m_items.size())
        return false;
    if (to < 0 || to >= m_items.size())
        return false;

    // beginMoveRows 的 destinationChild 是"插到哪一行之前"，所以向下移动要 +1，
    // 否则 QAbstractItemModel 会直接拒绝这次移动。
    const int destinationChild = to > from ? to + 1 : to;
    if (!beginMoveRows(QModelIndex(), from, from, QModelIndex(), destinationChild))
        return false;

    m_items.move(from, to);
    endMoveRows();
    return true;
}

QStringList PresetPromptListModel::identifiers() const
{
    QStringList ids;
    ids.reserve(m_items.size());
    for (const PromptEntity& item : m_items)
        ids.append(item.identifier);
    return ids;
}

void PresetPromptListModel::collapseAll()
{
    if (m_expandedIds.isEmpty() || m_items.isEmpty())
        return;

    m_expandedIds.clear();
    emit dataChanged(index(0), index(m_items.size() - 1), { ExpandedStateRole });
}
