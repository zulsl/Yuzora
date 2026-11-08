#include "WorldInfoEntryViewModel.h"
#include "WorldInfoService.h"
#include "ExceptionHandler.h"

WorldInfoEntryViewModel* WorldInfoEntryViewModel::instance()
{
    static WorldInfoEntryViewModel inst;
    return &inst;
}

WorldInfoEntryViewModel::WorldInfoEntryViewModel(QObject* parent)
    : QObject(parent)
    , m_service(&WorldInfoService::instance())
    , m_entryModel(new WorldInfoEntryListModel(this))
{
    // 当前文集下的条目发生变化时刷新列表
    connect(m_service, &WorldInfoService::entryListChanged,
            this, [this](const QString& bookId) {
                if (bookId == m_currentBookId)
                    refreshEntryList();
            });

    // 列表项的业务字段变化 → 通知 Service 落盘
    connect(m_entryModel, &WorldInfoEntryListModel::entryChanged,
            this, [this](const WorldInfoEntryEntity& entry) {
                m_service->updateEntry(entry);
            });

    connect(m_service, &WorldInfoService::operationError,
            this, &WorldInfoEntryViewModel::operationError);
}

WorldInfoEntryViewModel::~WorldInfoEntryViewModel() = default;

WorldInfoEntryListModel* WorldInfoEntryViewModel::entryModel() const
{
    return m_entryModel;
}

QVariantMap WorldInfoEntryViewModel::currentEntry() const
{
    return entryToMap(m_currentEntry);
}

void WorldInfoEntryViewModel::setCurrentBookId(const QString& bookId)
{
    m_currentBookId = bookId;
    m_currentEntry = {}; // 切换文集时清空当前条目
    emit currentEntryChanged();
    refreshEntryList();
}

void WorldInfoEntryViewModel::refreshEntryList()
{
    if (m_currentBookId.isEmpty()) {
        m_entryModel->clear();
        return;
    }

    QList<WorldInfoEntryEntity> entries =
        m_service->getEntriesByBook(m_currentBookId);
    m_entryModel->resetData(entries);
}

void WorldInfoEntryViewModel::selectEntry(const QString& entryId)
{
    Q_UNUSED(entryId)
    emit currentEntryChanged();
}

void WorldInfoEntryViewModel::createEntry()
{
    if (m_currentBookId.isEmpty()) {
        emit operationError(QStringLiteral("请先选择一个文集"));
        return;
    }

    QString id = m_service->createEntry(m_currentBookId);
    if (!id.isEmpty())
        selectEntry(id);
}

void WorldInfoEntryViewModel::saveEntry(const QVariantMap& entryData)
{
    // 保持与原实现一致：QML 直接写模型角色字段即可落盘，此接口暂为占位
    Q_UNUSED(entryData)
}

void WorldInfoEntryViewModel::deleteEntry(const QString& entryId)
{
    bool success = m_service->deleteEntry(entryId);
    if (success && m_currentEntry.id == entryId) {
        m_currentEntry = {};
        emit currentEntryChanged();
    }
}

void WorldInfoEntryViewModel::toggleEntryEnabled(const QString& entryId)
{
    m_service->toggleEntryEnabled(entryId);

    if (m_currentEntry.id == entryId) {
        m_currentEntry.isEnabled = !m_currentEntry.isEnabled;
        emit currentEntryChanged();
    }
}

void WorldInfoEntryViewModel::moveEntry(const QString& entryId, const QString& targetBookId)
{
    m_service->moveEntryToBook(entryId, targetBookId);

    if (m_currentEntry.id == entryId) {
        m_currentEntry = {};
        emit currentEntryChanged();
    }
}

void WorldInfoEntryViewModel::batchDelete(const QStringList& entryIds)
{
    m_service->batchDeleteEntries(entryIds);

    if (entryIds.contains(m_currentEntry.id)) {
        m_currentEntry = {};
        emit currentEntryChanged();
    }
}

void WorldInfoEntryViewModel::batchSetEnabled(const QStringList& entryIds, bool enabled)
{
    m_service->batchToggleEntries(entryIds, enabled);
}

// ======================================
// 私有辅助：条目实体 ↔ QVariantMap 互转
// ======================================
QVariantMap WorldInfoEntryViewModel::entryToMap(const WorldInfoEntryEntity& entry) const
{
    QVariantMap map;
    if (entry.id.isEmpty()) return map;

    map["id"] = entry.id;
    map["bookId"] = entry.bookId;
    map["name"] = entry.name;
    map["keywords"] = entry.keywords;
    map["secondaryKeywords"] = entry.secondaryKeywords;
    map["content"] = entry.content;
    map["extensions"] = entry.extensions;

    return map;
}

WorldInfoEntryEntity WorldInfoEntryViewModel::mapToEntry(const QVariantMap& map) const
{
    WorldInfoEntryEntity entry;

    entry.name = map.value("name", "新条目").toString().trimmed();
    entry.content = map.value("content", "").toString();
    entry.isEnabled = map.value("isEnabled", true).toBool();
    entry.extensions = map.value("extensions", QVariantMap{}).toMap();

    return entry;
}
