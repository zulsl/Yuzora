#include "WorldInfoService.h"
#include "WorldInfoBookDao.h"
#include "Utils.h"
#include "CharacterService.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include "ExceptionHandler.h"

// ======================================
// 单例实例
// ======================================
WorldInfoService& WorldInfoService::instance()
{
    static WorldInfoService instance;
    return instance;
}

WorldInfoService::WorldInfoService(QObject* parent)
    : QObject(parent)
    , m_BookDao(new WorldInfoBookDao(this))
    , m_EntryDao(new WorldInfoEntryDao(this))
{
}

WorldInfoService::WorldInfoService(WorldInfoBookDao* bookDao,
                                   WorldInfoEntryDao* entryDao,
                                   QObject* parent)
    : QObject(parent)
    , m_BookDao(bookDao ? bookDao : new WorldInfoBookDao(this))
    , m_EntryDao(entryDao ? entryDao : new WorldInfoEntryDao(this))
{
}

WorldInfoService::~WorldInfoService() = default;

QList<WorldInfoEntryEntity> WorldInfoService::getEntryForEnabledBooks()
{
    QList<WorldInfoEntryEntity> result;

    for (const WorldInfoBookEntity& book : m_BookDao->selectAll()) {
        if (!book.isEnabled) {
            continue;
        }

        const QList<WorldInfoEntryEntity> entries = m_EntryDao->selectEntriesByBookId(book.id);
        for (const WorldInfoEntryEntity& entry : entries) {
            if (entry.isEnabled) {
                result.append(entry);
            }
        }
    }

    return result;
}

// ======================================
// 文集业务
// ======================================
QList<WorldInfoBookEntity> WorldInfoService::getAllBooks()
{
    return m_BookDao->selectAll();
}

WorldInfoBookEntity WorldInfoService::getBookById(const QString& bookId)
{
    return m_BookDao->selectById(bookId);
}

QList<WorldInfoBookEntity> WorldInfoService::getEffectiveBooksForCharacter(const QString& charId)
{
    // // 生效范围 = 全局文集 + 角色绑定文集
    // QList<WorldInfoBookEntity> globalBooks = m_dao->selectGlobalBooks();

    // CharacterEntity character = CharacterService::instance().getCharacterById(charId);
    // QStringList boundIds = character.worldBookIds;

    // QList<WorldInfoBookEntity> boundBooks;
    // if (!boundIds.isEmpty()) {
    //     boundBooks = m_dao->selectBooksByIds(boundIds);
    // }

    // 按 ID 去重，避免全局文集同时被角色绑定导致重复
    QSet<QString> addedIds;
    QList<WorldInfoBookEntity> result;

    auto appendUnique = [&](const QList<WorldInfoBookEntity>& list) {
        for (const auto& book : list) {
            if (!addedIds.contains(book.id) && book.isEnabled) {
                result.append(book);
                addedIds.insert(book.id);
            }
        }
    };

    // appendUnique(globalBooks);
    // appendUnique(boundBooks);
    return result;
}

QString WorldInfoService::createBook(const QString& name, const QString& description)
{
    QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        emit operationError("文集名称不能为空");
        return {};
    }

    WorldInfoBookEntity book;
    book.id = Utils::generateUuid();
    book.name = trimmedName;
    book.isEnabled = false;
    book.createdAt = QDateTime::currentDateTime();
    book.updatedAt = QDateTime::currentDateTime();

    bool success = m_BookDao->insert(book);
    if (!success) {
        emit operationError("创建文集失败");
        return {};
    }

    emit bookListChanged();
    return book.id;
}

bool WorldInfoService::updateBook(const WorldInfoBookEntity& book)
{
    if (book.id.isEmpty() || book.name.trimmed().isEmpty()) {
        emit operationError("文集ID和名称不能为空");
        return false;
    }

    if (!m_BookDao->update(book)) {
        emit operationError("更新文集失败");
        return false;
    }

    emit bookUpdated(book.id);
    emit bookListChanged();
    return true;
}

bool WorldInfoService::deleteBook(const QString& bookId)
{
    if (m_BookDao->selectById(bookId).id.isEmpty()) {
        THROW(OperationError, "[WorldInfoService]文集不存在");
        return false;
    }

    bool success = m_BookDao->remove(bookId);
    if (!success) {
        THROW(OperationError, "删除文集失败");
    }

    emit bookListChanged();
    return success;
}

QString WorldInfoService::duplicateBook(const QString& sourceId)
{
    // WorldInfoBookEntity source = m_dao->selectBookById(sourceId);
    // if (source.id.isEmpty()) {
    //     emit operationError("源文集不存在");
    //     return {};
    // }

    // // 1. 复制文集本体
    // WorldInfoBookEntity newBook = source;
    // newBook.id = Utils::generateUuid();
    // newBook.name = source.name + " (副本)";
    // newBook.isGlobal = false;

    // QString newBookId = m_dao->insertBook(newBook);
    // if (newBookId.isEmpty()) {
    //     emit operationError("复制文集失败");
    //     return {};
    // }

    // // 2. 批量复制所有条目
    // QList<WorldInfoEntryEntity> entries = m_dao->selectEntriesByBookId(sourceId);
    // for (const auto& entry : entries) {
    //     WorldInfoEntryEntity newEntry = entry;
    //     newEntry.id = Utils::generateUuid();
    //     newEntry.bookId = newBookId;
    //     m_dao->insertEntry(newEntry);
    // }

    // emit bookListChanged();
    // return newBookId;
    return QString();
}

bool WorldInfoService::toggleBookEnabled(const QString& bookId)
{
    WorldInfoBookEntity book = m_BookDao->selectById(bookId);
    if (book.id.isEmpty()) {
        emit operationError("文集不存在");
        return false;
    }

    book.isEnabled = !book.isEnabled;
    bool success = m_BookDao->update(book);
    if (!success) {
        emit operationError("启用失败");
    }
    return success;
}


// ======================================
// 条目业务
// ======================================
QList<WorldInfoEntryEntity> WorldInfoService::getEntriesByBook(const QString& bookId)
{
    return m_EntryDao->selectEntriesByBookId(bookId);
}

QString WorldInfoService::createEntry(const QString& bookId, const QString& name)
{
    if (bookId.isEmpty()) {
        emit operationError("所属文集ID不能为空");
        return {};
    }
    if (m_BookDao->selectById(bookId).id.isEmpty()) {
        emit operationError("所属文集不存在");
        return {};
    }

    WorldInfoEntryEntity entry;
    entry.id = Utils::generateUuid();
    entry.bookId = bookId;
    entry.name = name.trimmed().isEmpty() ? QStringLiteral("新条目") : name.trimmed();
    entry.content.clear();
    entry.isEnabled = true;
    entry.constant = false;
    entry.vectorized = false;
    entry.position = WorldInfoEntryEntity::BeforeChar;
    entry.insertRole = GlobalConstants::MessageType::System;
    entry.insertDepth = 4;
    entry.sortOrder = 100;
    entry.probability = 100;
    entry.selective = true;

    if (!m_EntryDao->insert(entry)) {
        emit operationError("创建条目失败");
        return {};
    }

    emit entryListChanged(bookId);
    return entry.id;
}

bool WorldInfoService::updateEntry(const WorldInfoEntryEntity & entry)
{
    if (entry.id.isEmpty() || entry.bookId.isEmpty()) {
        emit operationError("条目ID与所属文集不能为空");
        return false;
    }

    bool success = m_EntryDao->update(entry);
    if (success) {
        // emit entryListChanged(entry.bookId);
        emit entryUpdated(entry.id);
    } else {
        emit operationError("更新条目失败");
    }
    return success;
}

bool WorldInfoService::deleteEntry(const QString& entryId)
{
    const WorldInfoEntryEntity entry = m_EntryDao->selectById(entryId);
    if (entry.id.isEmpty()) {
        emit operationError("条目不存在");
        return false;
    }

    if (!m_EntryDao->remove(entryId)) {
        emit operationError("删除条目失败");
        return false;
    }

    emit entryDeleted(entryId);
    emit entryListChanged(entry.bookId);
    return true;
}

bool WorldInfoService::toggleEntryEnabled(const QString& entryId)
{
    WorldInfoEntryEntity entry = m_EntryDao->selectById(entryId);
    if (entry.id.isEmpty()) {
        emit operationError("条目不存在");
        return false;
    }

    entry.isEnabled = !entry.isEnabled;
    if (!m_EntryDao->update(entry)) {
        emit operationError("更新条目状态失败");
        return false;
    }

    emit entryUpdated(entryId);
    emit entryListChanged(entry.bookId);
    return true;
}

bool WorldInfoService::moveEntryToBook(const QString& entryId, const QString& targetBookId)
{
    WorldInfoEntryEntity entry = m_EntryDao->selectById(entryId);
    if (entry.id.isEmpty()) {
        emit operationError("源条目不存在");
        return false;
    }
    if (m_BookDao->selectById(targetBookId).id.isEmpty()) {
        emit operationError("目标文集不存在");
        return false;
    }
    if (entry.bookId == targetBookId) {
        return true;
    }

    const QString oldBookId = entry.bookId;
    entry.bookId = targetBookId;
    if (!m_EntryDao->update(entry)) {
        emit operationError("移动条目失败");
        return false;
    }

    emit entryListChanged(oldBookId);
    emit entryListChanged(targetBookId);
    return true;
}

bool WorldInfoService::batchDeleteEntries(const QStringList& entryIds)
{
    if (entryIds.isEmpty()) {
        return true;
    }

    QSet<QString> affectedBooks;
    for (const QString& id : entryIds) {
        const WorldInfoEntryEntity entry = m_EntryDao->selectById(id);
        if (entry.id.isEmpty()) {
            continue;
        }
        if (!m_EntryDao->remove(id)) {
            emit operationError("批量删除条目失败");
            return false;
        }
        affectedBooks.insert(entry.bookId);
        emit entryDeleted(id);
    }

    for (const QString& bookId : affectedBooks) {
        emit entryListChanged(bookId);
    }
    return true;
}

bool WorldInfoService::batchToggleEntries(const QStringList& entryIds, bool enabled)
{
    if (entryIds.isEmpty()) {
        return true;
    }

    QSet<QString> affectedBooks;
    for (const QString& id : entryIds) {
        WorldInfoEntryEntity entry = m_EntryDao->selectById(id);
        if (entry.id.isEmpty()) {
            continue;
        }
        if (entry.isEnabled == enabled) {
            continue;
        }
        entry.isEnabled = enabled;
        if (!m_EntryDao->update(entry)) {
            emit operationError("批量更新条目状态失败");
            return false;
        }
        affectedBooks.insert(entry.bookId);
        emit entryUpdated(id);
    }

    for (const QString& bookId : affectedBooks) {
        emit entryListChanged(bookId);
    }
    return true;
}


// ======================================
// 导入导出
// ======================================
int WorldInfoService::importBookFromSillyTavern(const QString& filePath)
{
    QJsonDocument doc = Utils::readJsonFile(filePath);
    if (doc.isNull()) {
        emit operationError("文件读取失败或格式无效");
        return 0;
    }

    QJsonObject bookData=doc.object().value("originalData").toObject();

    const QString bookId = importWorldBook(bookData);
    if (bookId.isEmpty()) {
        return 0;
    }

    emit bookListChanged();
    emit importSuccess(1);
    return 1;
}

bool WorldInfoService::exportBookToSillyTavern(const QString& bookId, const QString& savePath)
{
    const WorldInfoBookEntity book = m_BookDao->selectById(bookId);
    if (book.id.isEmpty()) {
        emit operationError("文集不存在");
        return false;
    }

    const QList<WorldInfoEntryEntity> entries = m_EntryDao->selectEntriesByBookId(bookId);
    const QJsonObject stJson = bookToStJson(book, entries);
    if (!Utils::writeJsonFile(savePath, QJsonDocument(stJson))) {
        emit operationError("导出文件写入失败");
        return false;
    }

    emit exportSuccess();
    return true;
}

bool WorldInfoService::exportAllBooksToSillyTavern(const QString& savePath)
{
    const QList<WorldInfoBookEntity> books = m_BookDao->selectAll();
    if (books.isEmpty()) {
        emit operationError("无可导出的世界书");
        return false;
    }

    QJsonArray arr;
    for (const WorldInfoBookEntity& book : books) {
        const QList<WorldInfoEntryEntity> entries = m_EntryDao->selectEntriesByBookId(book.id);
        arr.append(bookToStJson(book, entries));
    }

    if (!Utils::writeJsonFile(savePath, QJsonDocument(arr))) {
        emit operationError("导出文件写入失败");
        return false;
    }

    emit exportSuccess();
    return true;
}

QString WorldInfoService::importWorldBook(QJsonObject &bookData)
{
    //导入书
    WorldInfoBookEntity book = parseJsonToBook(bookData);
    if (book.name.isEmpty() || !m_BookDao->insert(book)) {
        emit operationError("导入世界书失败");
        return {};
    }

    // 导入条目(json数组)
    QJsonArray entries = bookData.value("entries").toArray();
    for(const QJsonValue& e : entries)
    {
        if (!e.isObject()) continue;
        WorldInfoEntryEntity entry = parseJsonToEntry(e.toObject(), book.id);
        if (!m_EntryDao->insert(entry)) {
            emit operationError("导入世界书条目失败");
            // 回滚已插入的书，避免留下空壳
            m_BookDao->remove(book.id);
            return {};
        }
    }

    emit bookListChanged();

    return book.id;
}

// ======================================
// 私有：SillyTavern 格式互转
// ======================================
QJsonObject WorldInfoService::bookToStJson(
    const WorldInfoBookEntity& book,
    const QList<WorldInfoEntryEntity>& entries)
{
    QJsonObject obj;
    obj["name"] = book.name;
    obj["isEnabled"] = book.isEnabled;

    QJsonArray entriesArr;
    for (const WorldInfoEntryEntity& entry : entries) {
        QJsonObject entryObj;
        entryObj["name"] = entry.name;
        entryObj["key"] = QJsonArray::fromStringList(entry.keywords);
        entryObj["keysecondary"] = QJsonArray::fromStringList(entry.secondaryKeywords);
        entryObj["comment"] = entry.name;
        entryObj["content"] = entry.content;
        entryObj["constant"] = entry.constant;
        entryObj["vectorized"] = entry.vectorized;
        entryObj["position"] = static_cast<int>(entry.position);
        entryObj["role"] = static_cast<int>(entry.insertRole);
        entryObj["order"] = entry.sortOrder;
        entryObj["probability"] = entry.probability;
        entryObj["selective"] = entry.selective;
        entryObj["selectiveLogic"] = static_cast<int>(entry.selectiveLogic);
        entryObj["scanDepth"] = entry.scanDepth;
        entryObj["useGroupScoring"] = entry.useGroupScoring;
        entryObj["automationId"] = entry.automationId;
        entryObj["extensions"] = Utils::variantMapToJsonObject(entry.extensions);
        entriesArr.append(entryObj);
    }
    obj["entries"] = entriesArr;
    return obj;
}

WorldInfoBookEntity WorldInfoService::parseJsonToBook(const QJsonObject& json)
{

    WorldInfoBookEntity book;
    book.id = Utils::generateUuid();
    book.name = json.value("name").toString().trimmed();
    book.isEnabled = json.value("isEnabled").toBool(false);
    return book;
}

WorldInfoEntryEntity WorldInfoService::parseJsonToEntry(const QJsonObject& json, const QString& bookId)
{
    WorldInfoEntryEntity entry;
    entry.id = Utils::generateUuid();
    // entry.CompatibleId;//原版有个兼容id
    entry.bookId = bookId;

    // 关键词转换
    entry.keywords = Utils::jsonToStringList(json.value("key").toArray());
    entry.secondaryKeywords = Utils::jsonToStringList(json.value("keysecondary").toArray());

    entry.name = json.value("comment").toString("未命名条目").trimmed();
    entry.content = json.value("content").toString("");
    entry.isEnabled = !json.value("disable").toBool(false);

    entry.constant = json.value("constant").toBool();
    entry.vectorized = json.value("vectorized").toBool();
    entry.position = static_cast<WorldInfoEntryEntity::Position>(json.value("position").toInt());
    QJsonValue role = json.value("role");
    entry.insertRole = role.isNull()
        ?GlobalConstants::MessageType::Other
        :static_cast<GlobalConstants::MessageType>(role.toInt());
    entry.insertDepth = json.value("depth").toInt(4);
    entry.sortOrder = json.value("order").toInt();
    entry.useProbability = json.value("useProbability").toBool(false);
    entry.probability = json.value("probability").toInt(100);

    entry.selective = json.value("selective").toBool();
    entry.selectiveLogic = static_cast<WorldInfoEntryEntity::SelectiveLogic>(json.value("selectiveLogic").toInt());
    QJsonValue scanDepth = json.value("scanDepth");  entry.scanDepth = scanDepth.isNull()?-1:scanDepth.toInt();
    //区分大小写（未设置）
    //全词匹配（未设置）
    entry.useGroupScoring = json.value("useGroupScoring").toBool(false);
    entry.automationId = json.value("automationId").toString();



    entry.extensions = Utils::jsonObjectToVariantMap(json.value("extensions").toObject());
    return entry;
}