#include "WorldInfoViewModel.h"
#include "WorldInfoService.h"
#include "WorldInfoBookItemModel.h"
#include "WorldInfoEntryItemModel.h"
#include "WorldInfoBookEntity.h"
#include "WorldInfoEntryEntity.h"
#include "ExceptionHandler.h"

// ======================================
// 构造与析构
// ======================================


WorldInfoViewModel::WorldInfoViewModel(QObject* parent)
    : QObject(parent)
    , m_bookModel(new WorldInfoBookListModel(this))
    , m_entryModel(new WorldInfoEntryListModel(this))
{
    auto& service = WorldInfoService::instance();

    // 绑定 Service 层信号
    connect(&service, &WorldInfoService::entryListChanged,
            this, [this](const QString& bookId) {
                // 仅当前选中文集发生条目变更时刷新列表
                if (bookId == m_currentBookId) {
                    refreshEntryList();
                }
            });

    // Model 的业务字段变化 → 通知 Service 落盘
    connect(m_entryModel, &WorldInfoEntryListModel::entryChanged,
            this, [](const WorldInfoEntryEntity & entry) {
                WorldInfoService::instance().updateEntry(entry);
            });

    connect(&service, &WorldInfoService::operationError,
            this, &WorldInfoViewModel::operationError);


    // 初始化加载文集列表
    refreshBookList();
}

WorldInfoViewModel::~WorldInfoViewModel() = default;

// ======================================
// 属性读取
// ======================================
WorldInfoBookListModel* WorldInfoViewModel::bookModel() const
{
    return m_bookModel;
}

WorldInfoEntryListModel* WorldInfoViewModel::entryModel() const
{
    return m_entryModel;
}

QVariantMap WorldInfoViewModel::currentEntry() const
{
    return entryToMap(m_currentEntry);
}

QString WorldInfoViewModel::currentBookId() const
{
    return m_currentBookId;
}

// ======================================
// 文集操作
// ======================================
void WorldInfoViewModel::refreshBookList()
{
    QList<WorldInfoBookEntity> books = WorldInfoService::instance().getAllBooks();
    QList<WorldInfoBookItemModel*> items;
    items.reserve(books.size());

    for (const auto& book : books) {
        items.append(new WorldInfoBookItemModel(book));
    }
    m_bookModel->resetData(items);
}

void WorldInfoViewModel::selectBook(const QString& bookId)
{
    if (m_currentBookId == bookId) return;

    m_currentBookId = bookId;
    m_currentEntry = {}; // 切换文集时清空当前条目

    emit currentBookIdChanged();
    emit currentEntryChanged();
    emit currentBookNameChanged();

    refreshEntryList();
}

void WorldInfoViewModel::createBook()
{
    qDebug()<<"[WorldInfoViewModel]新建世界书";

    QString name="新世界书";

    QString id = WorldInfoService::instance().createBook(name);
    if (!id.isEmpty()) {
        selectBook(id); // 新建后自动选中
    }
}

void WorldInfoViewModel::deleteBook()
{
    try{
        if(m_currentBookId.isEmpty())
            THROW(UnknownError, "[WorldInfoViewModel]m_currentBookId为空");

        bool success = WorldInfoService::instance().deleteBook(m_currentBookId);
        if (success) {
            m_currentBookId.clear();
            m_currentEntry = {};
            m_entryModel->clear();
            emit currentBookIdChanged();
            emit currentEntryChanged();
        }
        refreshBookList();

        ExceptionHandler::instance().tipOccurred("删除成功");
    }
    catch (const AppException& e) {ExceptionHandler::instance().handleException(e);}
    catch (const std::exception& e) {ExceptionHandler::instance().handleException(e);}
    catch (...) {ExceptionHandler::instance().handleException();}



}

void WorldInfoViewModel::duplicateBook(const QString& bookId)
{
    QString newId = WorldInfoService::instance().duplicateBook(bookId);
    if (!newId.isEmpty()) {
        selectBook(newId);
    }
}

void WorldInfoViewModel::toggleBookEnabled(const QString& bookId)
{
    WorldInfoService::instance().toggleBookEnabled(bookId);
    refreshBookList();
}

void WorldInfoViewModel::toggleBookGlobal(const QString& bookId)
{
}

void WorldInfoViewModel::renameBook(const QString& bookId, const QString& newName)
{
    QString trimmedName = newName.trimmed();
    if (trimmedName.isEmpty()) {
        emit operationError("文集名称不能为空");
        return;
    }

    WorldInfoBookEntity book = WorldInfoService::instance().getBookById(bookId);
    if (book.id.isEmpty()) return;

    book.name = trimmedName;
    WorldInfoService::instance().updateBook(book);
}

// ======================================
// 条目操作
// ======================================
void WorldInfoViewModel::refreshEntryList()
{
    if (m_currentBookId.isEmpty()) {
        m_entryModel->clear();
        return;
    }

    QList<WorldInfoEntryEntity> entries = WorldInfoService::instance().getEntriesByBook(m_currentBookId);
    m_entryModel->resetData(entries);
}

void WorldInfoViewModel::selectEntry(const QString& entryId)
{
    // m_currentEntry = WorldInfoService::instance().getEntryById(entryId);
    emit currentEntryChanged();
}

void WorldInfoViewModel::createEntry()
{
    if (m_currentBookId.isEmpty()) {
        emit operationError("请先选择一个文集");
        return;
    }

    QString id = WorldInfoService::instance().createEntry(m_currentBookId);
    if (!id.isEmpty()) {
        selectEntry(id); // 新建后自动选中进入编辑
    }
}

void WorldInfoViewModel::saveEntry(const QVariantMap& entryData)
{
    // if (m_currentEntry.id.isEmpty()) return;

    // // 增量更新字段，未传入的保留原值
    // WorldInfoEntryEntity entry = m_currentEntry;

    // if (entryData.contains("name")) entry.name = entryData["name"].toString().trimmed();
    // if (entryData.contains("content")) entry.content = entryData["content"].toString();
    // if (entryData.contains("priority")) entry.priority = entryData["priority"].toInt();
    // if (entryData.contains("depth")) entry.depth = entryData["depth"].toInt();
    // if (entryData.contains("probability")) entry.probability = entryData["probability"].toInt();
    // if (entryData.contains("insertPosition")) {
    //     entry.insertPosition = static_cast<InsertPosition>(entryData["insertPosition"].toInt());
    // }

    // // 关键词列表
    // if (entryData.contains("keywords")) entry.keywords = entryData["keywords"].toStringList();
    // if (entryData.contains("secondaryKeywords")) entry.secondaryKeywords = entryData["secondaryKeywords"].toStringList();

    // // 匹配规则开关
    // if (entryData.contains("isEnabled")) entry.isEnabled = entryData["isEnabled"].toBool();
    // if (entryData.contains("isRecursive")) entry.isRecursive = entryData["isRecursive"].toBool();
    // if (entryData.contains("isCaseSensitive")) entry.isCaseSensitive = entryData["isCaseSensitive"].toBool();
    // if (entryData.contains("matchWholeWords")) entry.matchWholeWords = entryData["matchWholeWords"].toBool();
    // if (entryData.contains("useRegex")) entry.useRegex = entryData["useRegex"].toBool();
    // if (entryData.contains("isSelective")) entry.isSelective = entryData["isSelective"].toBool();

    // // 扩展字段
    // if (entryData.contains("extensions")) entry.extensions = entryData["extensions"].toMap();

    // bool success = WorldInfoService::instance().updateEntry(entry);
    // if (success) {
    //     m_currentEntry = entry;
    //     emit currentEntryChanged();
    // }
}

void WorldInfoViewModel::deleteEntry(const QString& entryId)
{
    bool success = WorldInfoService::instance().deleteEntry(entryId);
    if (success && m_currentEntry.id == entryId) {
        m_currentEntry = {};
        emit currentEntryChanged();
    }
}

void WorldInfoViewModel::toggleEntryEnabled(const QString& entryId)
{
    WorldInfoService::instance().toggleEntryEnabled(entryId);

    // 同步更新本地选中状态，UI 实时刷新
    if (m_currentEntry.id == entryId) {
        m_currentEntry.isEnabled = !m_currentEntry.isEnabled;
        emit currentEntryChanged();
    }
}

void WorldInfoViewModel::moveEntry(const QString& entryId, const QString& targetBookId)
{
    WorldInfoService::instance().moveEntryToBook(entryId, targetBookId);

    // 移出当前文集后清空选中
    if (m_currentEntry.id == entryId) {
        m_currentEntry = {};
        emit currentEntryChanged();
    }
}

void WorldInfoViewModel::batchDelete(const QStringList& entryIds)
{
    WorldInfoService::instance().batchDeleteEntries(entryIds);

    // 若删除包含当前选中条目，清空详情
    if (entryIds.contains(m_currentEntry.id)) {
        m_currentEntry = {};
        emit currentEntryChanged();
    }
}

void WorldInfoViewModel::batchSetEnabled(const QStringList& entryIds, bool enabled)
{
    WorldInfoService::instance().batchToggleEntries(entryIds, enabled);
}

// ======================================
// 导入导出（SillyTavern 格式）
// ======================================
void WorldInfoViewModel::importBook(const QUrl& fileUrl)
{
    QString filePath = fileUrl.toLocalFile();
    WorldInfoService::instance().importBookFromSillyTavern(filePath);
}

void WorldInfoViewModel::exportCurrentBook(const QString& savePath)
{
    if (m_currentBookId.isEmpty()) {
        emit operationError("请先选择要导出的文集");
        return;
    }
    WorldInfoService::instance().exportBookToSillyTavern(m_currentBookId, savePath);
}

void WorldInfoViewModel::exportAllBooks(const QString& savePath)
{
    WorldInfoService::instance().exportAllBooksToSillyTavern(savePath);
}

// ======================================
// 私有辅助：条目实体 ↔ QVariantMap 互转
// ======================================
QVariantMap WorldInfoViewModel::entryToMap(const WorldInfoEntryEntity& entry) const
{
    QVariantMap map;
    if (entry.id.isEmpty()) return map;

    // 基础标识
    map["id"] = entry.id;
    map["bookId"] = entry.bookId;
    map["name"] = entry.name;

    // 关键词
    map["keywords"] = entry.keywords;
    map["secondaryKeywords"] = entry.secondaryKeywords;

    // 核心内容
    map["content"] = entry.content;

    // // 触发控制
    // map["priority"] = entry.priority;
    // map["probability"] = entry.probability;

    // // 匹配规则开关
    // map["isRecursive"] = entry.isRecursive;
    // map["isCaseSensitive"] = entry.isCaseSensitive;
    // map["matchWholeWords"] = entry.matchWholeWords;
    // map["useRegex"] = entry.useRegex;
    // map["isSelective"] = entry.isSelective;
    // map["isEnabled"] = entry.isEnabled;

    // 扩展
    map["extensions"] = entry.extensions;

    return map;
}

WorldInfoEntryEntity WorldInfoViewModel::mapToEntry(const QVariantMap& map) const
{
    WorldInfoEntryEntity entry;

    entry.name = map.value("name", "新条目").toString().trimmed();
    entry.content = map.value("content", "").toString();
    // entry.priority = map.value("priority", 100).toInt();
    // entry.probability = map.value("probability", 100).toInt();

    // entry.keywords = map.value("keywords", QStringList{}).toStringList();
    // entry.secondaryKeywords = map.value("secondaryKeywords", QStringList{}).toStringList();

    // entry.isRecursive = map.value("isRecursive", false).toBool();
    // entry.isCaseSensitive = map.value("isCaseSensitive", false).toBool();
    // entry.matchWholeWords = map.value("matchWholeWords", false).toBool();
    // entry.useRegex = map.value("useRegex", false).toBool();
    // entry.isSelective = map.value("isSelective", false).toBool();
    entry.isEnabled = map.value("isEnabled", true).toBool();

    entry.extensions = map.value("extensions", QVariantMap{}).toMap();

    return entry;
}

QString WorldInfoViewModel::getCurrentBookName() const
{
    if (m_currentBookId.isEmpty())
        return {};

    WorldInfoBookEntity entity = WorldInfoService::instance().getBookById(m_currentBookId);
    qDebug()<<entity.name;
    return entity.name;
}

void WorldInfoViewModel::setCurrentBookName(const QString &newCurrentBookName)
{
    try{
        if (m_currentBookId.isEmpty())
            THROW(UnknownError, "[WorldInfoViewModel]m_currentBookId为空");

        WorldInfoBookEntity entity = WorldInfoService::instance().getBookById(m_currentBookId);
        entity.name=newCurrentBookName;
        WorldInfoService::instance().updateBook(entity);
        emit currentBookNameChanged();
    }
    catch (const AppException& e) {ExceptionHandler::instance().handleException(e);}
    catch (const std::exception& e) {ExceptionHandler::instance().handleException(e);}
    catch (...) {ExceptionHandler::instance().handleException();}

}