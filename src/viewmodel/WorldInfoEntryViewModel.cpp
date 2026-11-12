#include "WorldInfoEntryViewModel.h"
#include "WorldInfoService.h"
#include "ExceptionHandler.h"

#include <QTimer>

WorldInfoEntryViewModel* WorldInfoEntryViewModel::instance()
{
    static WorldInfoEntryViewModel inst;
    return &inst;
}

WorldInfoEntryViewModel::WorldInfoEntryViewModel(QObject* parent)
    : QObject(parent)
    , m_service(&WorldInfoService::instance())
    , m_entryModel(new WorldInfoEntryListModel(this))
    , m_saveTimer(new QTimer(this))
{
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(300);
    connect(m_saveTimer, &QTimer::timeout,
            this, &WorldInfoEntryViewModel::flushPendingEntryEdits);

    // 当前文集下的条目发生变化时刷新列表
    connect(m_service, &WorldInfoService::entryListChanged,
            this, [this](const QString& bookId) {
                if (bookId == m_currentBookId)
                    refreshEntryList();
            });

    connect(m_service, &WorldInfoService::operationError,
            this, &WorldInfoEntryViewModel::operationError);
}

WorldInfoEntryViewModel::~WorldInfoEntryViewModel() = default;

WorldInfoEntryListModel* WorldInfoEntryViewModel::entryModel() const
{
    return m_entryModel;
}

void WorldInfoEntryViewModel::setCurrentBookId(const QString& bookId)
{
    flushPendingEntryEdits();
    m_currentBookId = bookId;
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

void WorldInfoEntryViewModel::createEntry()
{
    if (m_currentBookId.isEmpty()) {
        emit operationError(QStringLiteral("请先选择一个文集"));
        return;
    }

    m_service->createEntry(m_currentBookId);
}

int WorldInfoEntryViewModel::roleForField(const QString& field) const
{
    if (field == QStringLiteral("name")) return WorldInfoEntryListModel::NameRole;
    if (field == QStringLiteral("content")) return WorldInfoEntryListModel::ContentRole;
    if (field == QStringLiteral("triggerControl")) return WorldInfoEntryListModel::TriggerControlRole;
    if (field == QStringLiteral("insertPosition")) return WorldInfoEntryListModel::InsertPositionRole;
    if (field == QStringLiteral("insertDepth")) return WorldInfoEntryListModel::InsertDepthRole;
    if (field == QStringLiteral("scanDepth")) return WorldInfoEntryListModel::ScanDepthRole;
    if (field == QStringLiteral("sortOrder")) return WorldInfoEntryListModel::SortOrderRole;
    if (field == QStringLiteral("isEnabled")) return WorldInfoEntryListModel::IsEnabledRole;
    if (field == QStringLiteral("keywords")) return WorldInfoEntryListModel::KeywordsRole;
    if (field == QStringLiteral("secondaryKeywords")) return WorldInfoEntryListModel::SecondaryKeywordsRole;
    if (field == QStringLiteral("selective")) return WorldInfoEntryListModel::SelectiveRole;
    if (field == QStringLiteral("selectiveLogic")) return WorldInfoEntryListModel::SelectiveLogicRole;
    return -1;
}

void WorldInfoEntryViewModel::setEntryField(const QString& entryId, const QString& field,
                                             const QVariant& value)
{
    if (entryId.isEmpty()) {
        return;
    }

    const int role = roleForField(field);
    if (role < 0 || !m_entryModel->updateField(entryId, role, value)) {
        return;
    }

    m_pendingFields[entryId].insert(field, value);

    // 文本编辑合并 300ms；开关/枚举/数字立即落库。
    if (field == QStringLiteral("name") || field == QStringLiteral("content")) {
        m_saveTimer->start();
    } else {
        flushPendingEntryEdits();
    }
}

void WorldInfoEntryViewModel::flushPendingEntryEdits()
{
    if (m_saveTimer->isActive()) {
        m_saveTimer->stop();
    }
    if (m_pendingFields.isEmpty()) {
        return;
    }

    const auto pending = m_pendingFields;
    m_pendingFields.clear();

    for (auto it = pending.constBegin(); it != pending.constEnd(); ++it) {
        const WorldInfoEntryEntity entry = m_entryModel->entryById(it.key());
        if (entry.id.isEmpty()) {
            continue;
        }
        if (!m_service->updateEntry(entry)) {
            // 保存失败时用数据库内容恢复，避免界面显示未落库的假状态。
            refreshEntryList();
            return;
        }
    }
}

void WorldInfoEntryViewModel::deleteEntry(const QString& entryId)
{
    flushPendingEntryEdits();
    m_service->deleteEntry(entryId);
}

void WorldInfoEntryViewModel::toggleEntryEnabled(const QString& entryId)
{
    setEntryField(entryId, QStringLiteral("isEnabled"),
                  !m_entryModel->entryById(entryId).isEnabled);
}

void WorldInfoEntryViewModel::moveEntry(const QString& entryId, const QString& targetBookId)
{
    flushPendingEntryEdits();
    m_service->moveEntryToBook(entryId, targetBookId);
}

void WorldInfoEntryViewModel::batchDelete(const QStringList& entryIds)
{
    flushPendingEntryEdits();
    m_service->batchDeleteEntries(entryIds);
}

void WorldInfoEntryViewModel::batchSetEnabled(const QStringList& entryIds, bool enabled)
{
    flushPendingEntryEdits();
    m_service->batchToggleEntries(entryIds, enabled);
}
