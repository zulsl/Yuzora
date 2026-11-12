#include "WorldInfoEntryListModel.h"

WorldInfoEntryListModel::WorldInfoEntryListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

WorldInfoEntryListModel::~WorldInfoEntryListModel() = default;

int WorldInfoEntryListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant WorldInfoEntryListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return QVariant();

    const WorldInfoEntryEntity& e = m_entries.at(index.row());
    switch (role){
        case EntryIdRole:           return e.id;
        case BookIdRole:            return e.bookId;
        case NameRole:              return e.name;
        case ContentRole:           return e.content;
        case KeywordsRole:          return e.keywords;
        case SecondaryKeywordsRole: return e.secondaryKeywords;
        case TriggerControlRole:    return calcTrigger(e);
        case InsertPositionRole:    return calcPositionIndex(e);
        case InsertDepthRole:       return e.insertDepth;
        case ScanDepthRole:         return e.scanDepth;
        case SortOrderRole:         return e.sortOrder;
        case UseProbabilityRole:    return e.useProbability;
        case ProbabilityRole:       return e.probability;
        case SelectiveRole:         return e.selective;
        case SelectiveLogicRole:    return e.selectiveLogic;
        case IsEnabledRole:         return e.isEnabled;
        case ExpandedStateRole:     return m_expandedIds.contains(e.id);
        default:                    return QVariant();
    }
}

bool WorldInfoEntryListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return false;

    // 模型只保留 UI 状态（展开）可写；业务字段必须通过 ViewModel 命令更新。
    if (role != ExpandedStateRole) {
        return false;
    }

    WorldInfoEntryEntity& e = m_entries[index.row()];
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

bool WorldInfoEntryListModel::updateField(const QString& id, int role, const QVariant& value)
{
    const int row = rowByIdInternal(id);
    if (row < 0) {
        return false;
    }

    WorldInfoEntryEntity& e = m_entries[row];
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
    case KeywordsRole: {
        const QStringList v = value.toStringList();
        if (e.keywords == v) return false;
        e.keywords = v;
        break;
    }
    case SecondaryKeywordsRole: {
        const QStringList v = value.toStringList();
        if (e.secondaryKeywords == v) return false;
        e.secondaryKeywords = v;
        break;
    }
    case IsEnabledRole: {
        const bool v = value.toBool();
        if (e.isEnabled == v) return false;
        e.isEnabled = v;
        break;
    }
    case TriggerControlRole:
        parseTrigger(e, value.toInt());
        break;
    case InsertPositionRole:
        parsePositionIndex(e, value.toInt());
        break;
    case InsertDepthRole: {
        const int v = value.toInt();
        if (e.insertDepth == v) return false;
        e.insertDepth = v;
        break;
    }
    case ScanDepthRole: {
        const int v = value.toInt();
        if (e.scanDepth == v) return false;
        e.scanDepth = v;
        break;
    }
    case SortOrderRole: {
        const int v = value.toInt();
        if (e.sortOrder == v) return false;
        e.sortOrder = v;
        break;
    }
    case SelectiveRole: {
        const bool v = value.toBool();
        if (e.selective == v) return false;
        e.selective = v;
        break;
    }
    case SelectiveLogicRole: {
        const int v = qBound(0, value.toInt(), 3);
        if (static_cast<int>(e.selectiveLogic) == v) return false;
        e.selectiveLogic = static_cast<WorldInfoEntryEntity::SelectiveLogic>(v);
        break;
    }
    default:
        return false;
    }

    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, { role });
    return true;
}

Qt::ItemFlags WorldInfoEntryListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;
    return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
}

QHash<int, QByteArray> WorldInfoEntryListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[EntryIdRole]           = "entryId";
    roles[BookIdRole]            = "bookId";
    roles[NameRole]              = "name";
    roles[ContentRole]           = "content";
    roles[KeywordsRole]          = "keywords";
    roles[SecondaryKeywordsRole] = "secondaryKeywords";
    roles[TriggerControlRole]    = "triggerControl";
    roles[InsertPositionRole]    = "insertPosition";
    roles[InsertDepthRole]       = "insertDepth";
    roles[ScanDepthRole]         = "scanDepth";
    roles[SortOrderRole]         = "sortOrder";
    roles[UseProbabilityRole]    = "useProbability";
    roles[ProbabilityRole]       = "probability";
    roles[SelectiveRole]         = "selective";
    roles[SelectiveLogicRole]    = "selectiveLogic";
    roles[IsEnabledRole]         = "isEnabled";
    roles[ExpandedStateRole]     = "expandedState";
    return roles;
}

void WorldInfoEntryListModel::resetData(const QList<WorldInfoEntryEntity>& entries)
{
    beginResetModel();
    m_entries = entries;

    // 清掉已经不存在的 id 的展开状态
    QSet<QString> validIds;
    validIds.reserve(m_entries.size());
    for (const auto& e : m_entries)
        validIds.insert(e.id);
    m_expandedIds.intersect(validIds);

    endResetModel();
}

void WorldInfoEntryListModel::clear()
{
    beginResetModel();
    m_entries.clear();
    m_expandedIds.clear();
    endResetModel();
}

int WorldInfoEntryListModel::rowById(const QString &id) const
{
    return rowByIdInternal(id);
}

WorldInfoEntryEntity WorldInfoEntryListModel::entryAt(int row) const
{
    if (row < 0 || row >= m_entries.size())
        return {};
    return m_entries.at(row);
}

WorldInfoEntryEntity WorldInfoEntryListModel::entryById(const QString &id) const
{
    const int row = rowByIdInternal(id);
    return row < 0 ? WorldInfoEntryEntity{} : m_entries.at(row);
}

void WorldInfoEntryListModel::updateEntry(const WorldInfoEntryEntity &entry)
{
    const int row = rowByIdInternal(entry.id);
    if (row < 0) {
        // 不在列表里，当作新增
        appendEntry(entry);
        return;
    }

    m_entries[row] = entry;

    QModelIndex idx = index(row);
    emit dataChanged(idx, idx);
}

void WorldInfoEntryListModel::removeEntry(const QString &id)
{
    const int row = rowByIdInternal(id);
    if (row < 0) return;

    beginRemoveRows(QModelIndex(), row, row);
    m_entries.removeAt(row);
    m_expandedIds.remove(id);
    endRemoveRows();
}

void WorldInfoEntryListModel::appendEntry(const WorldInfoEntryEntity &entry)
{
    const int row = m_entries.size();
    beginInsertRows(QModelIndex(), row, row);
    m_entries.append(entry);
    endInsertRows();
}

void WorldInfoEntryListModel::toggleExpanded(const QString &id)
{
    const int row = rowByIdInternal(id);
    if (row < 0) return;

    if (m_expandedIds.contains(id))
        m_expandedIds.remove(id);
    else
        m_expandedIds.insert(id);

    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, { ExpandedStateRole });
}

void WorldInfoEntryListModel::setExpanded(const QString &id, bool expanded)
{
    const int row = rowByIdInternal(id);
    if (row < 0) return;

    const bool current = m_expandedIds.contains(id);
    if (current == expanded) return;

    if (expanded)
        m_expandedIds.insert(id);
    else
        m_expandedIds.remove(id);

    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, { ExpandedStateRole });
}

void WorldInfoEntryListModel::collapseAll()
{
    if (m_expandedIds.isEmpty()) return;
    m_expandedIds.clear();
    if (!m_entries.isEmpty())
        emit dataChanged(index(0, 0), index(m_entries.size() - 1, 0), { ExpandedStateRole });
}


int WorldInfoEntryListModel::rowByIdInternal(const QString &id) const
{
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).id == id)
            return i;
    }
    return -1;
}

QString WorldInfoEntryListModel::buildKeywordPreview(const WorldInfoEntryEntity &entry)
{
    if (entry.keywords.isEmpty())
        return QStringLiteral("(无关键词)");

    const int maxShow = 3;
    QStringList preview = entry.keywords.mid(0, maxShow);
    QString s = preview.join(", ");
    if (entry.keywords.size() > maxShow)
        s += QStringLiteral(" …");
    return s;
}

void WorldInfoEntryListModel::parsePositionIndex(WorldInfoEntryEntity &e, int idx)
{
    switch (idx) {
        case 0: e.position = WorldInfoEntryEntity::BeforeChar; break;
        case 1: e.position = WorldInfoEntryEntity::AfterChar; break;
        case 2: e.position = WorldInfoEntryEntity::EMTop; break;
        case 3: e.position = WorldInfoEntryEntity::EMBottom; break;
        case 4: e.position = WorldInfoEntryEntity::ANTop; break;
        case 5: e.position = WorldInfoEntryEntity::ANBottom; break;
        case 6:
            e.position = WorldInfoEntryEntity::AtDepth;
            e.insertRole = GlobalConstants::MessageType::System;
            break;
        case 7:
            e.position = WorldInfoEntryEntity::AtDepth;
            e.insertRole = GlobalConstants::MessageType::User;
            break;
        case 8: e.position = WorldInfoEntryEntity::AtDepth;
            e.insertRole = GlobalConstants::MessageType::Assistant;
            break;
    }
}

int WorldInfoEntryListModel::calcPositionIndex(const WorldInfoEntryEntity& en)const
{
    int idx;
    if (en.position == WorldInfoEntryEntity::Position::AtDepth) {
        switch (en.insertRole) {
            case GlobalConstants::MessageType::System   : idx = 6; break;
            case GlobalConstants::MessageType::User     : idx = 7; break;
            case GlobalConstants::MessageType::Assistant: idx = 8; break;
            default: idx = 6;
        }
    } else {
        switch (en.position) {
            case WorldInfoEntryEntity::Position::BeforeChar : idx = 0; break;
            case WorldInfoEntryEntity::Position::AfterChar  : idx = 1; break;
            case WorldInfoEntryEntity::Position::ANTop      : idx = 4; break;
            case WorldInfoEntryEntity::Position::ANBottom   : idx = 5; break;
            case WorldInfoEntryEntity::Position::EMTop      : idx = 2; break;
            case WorldInfoEntryEntity::Position::EMBottom   : idx = 3; break;
            default: idx = 0;
        }
    }
    return idx;

}

int WorldInfoEntryListModel::calcTrigger(const WorldInfoEntryEntity &en)const
{
    if (en.constant)
        return 0;
    if (en.vectorized)
        return 2;
    return 1;
}

void WorldInfoEntryListModel::parseTrigger(WorldInfoEntryEntity &e,int idx)
{
    switch(idx){
        case 0: e.constant=true;e.vectorized=false;break;
        case 1: e.constant=false;e.vectorized=false;break;
        case 2: e.constant=false;e.vectorized=true;break;
    }
}

