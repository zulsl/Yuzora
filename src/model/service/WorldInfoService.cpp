#include "WorldInfoService.h"
#include "WorldInfoBookDao.h"
#include "Utils.h"
#include "CharacterService.h"
#include <QDateTime>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <algorithm>
#include "ExceptionHandler.h"

namespace {

// ST world 文件（worlds/*.json）的 entries 是 {uid: entry} 对象，
// 而角色卡 character_book / 我们自己导出的文件是数组。两种都接受。
// 对象按 uid 数值排序还原插入顺序：条目 order 相同时，稳定排序的结果取决于插入顺序。
QJsonArray entriesToArray(const QJsonValue& value)
{
    if (value.isArray()) return value.toArray();

    QJsonArray result;
    if (!value.isObject()) return result;

    const QJsonObject obj = value.toObject();
    QStringList keys = obj.keys();
    std::sort(keys.begin(), keys.end(), [](const QString& a, const QString& b) {
        bool okA = false;
        bool okB = false;
        const int ia = a.toInt(&okA);
        const int ib = b.toInt(&okB);
        if (okA && okB) return ia < ib;
        return a < b;
    });
    for (const QString& key : keys) result.append(obj.value(key));
    return result;
}

QStringList jsonStrings(const QJsonValue& value)
{
    QStringList out;
    if (value.isArray()) {
        for (const QJsonValue& v : value.toArray()) {
            if (v.isString()) out.append(v.toString());
            else if (v.isDouble()) out.append(QString::number(v.toInt()));
        }
    } else if (value.isString() && !value.toString().isEmpty()) {
        out.append(value.toString());
    }
    return out;
}

// 角色卡 V2 的字段大多藏在 extensions 里，world 文件则放在顶层。
// ST 的 convertCharacterBook 就是 "extensions 优先、顶层兜底"，这里保持一致。
QJsonValue pickValue(const QJsonObject& extensions, const QJsonObject& json,
                     const char* extKey, const char* key)
{
    const QLatin1String ext(extKey);
    if (extensions.contains(ext)) return extensions.value(ext);
    return json.value(QLatin1String(key));
}

} // namespace

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

QList<WorldInfoBookEntity> WorldInfoService::getEnabledBooks()
{
    QList<WorldInfoBookEntity> books;

    for (const WorldInfoBookEntity& book : m_BookDao->selectAll()) {
        if (!book.isEnabled) {
            continue;
        }
    }

    return books;
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

QList<WorldInfoEntryEntity> WorldInfoService::getEntriesForEnabledByBookId(const QString &bookId)
{
    QList<WorldInfoEntryEntity> result;

    QList<WorldInfoEntryEntity> entries=m_EntryDao->selectEntriesByBookId(bookId);
    for (const WorldInfoEntryEntity& entry : entries) {
        if (entry.isEnabled) {
            result.append(entry);
        }
    }
    return result;
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

    const QJsonObject root = doc.object();
    if (root.isEmpty()) {
        emit operationError("文件读取失败或格式无效");
        return 0;
    }

    // ST 的 world 文件长这样：{ "entries": {uid: entry}, "originalData": {角色卡原始书} }
    // - entries 是内部格式（key / 数字 position / depth），字段最准，优先用；
    // - originalData 是角色卡 V2 格式（keys / "before_char" / extensions.depth），
    //   只作为没有 entries 时的兜底。
    // 我们自己导出的文件是 { name, isEnabled, entries: [...] }。
    const QJsonObject original = root.value("originalData").toObject();
    QJsonArray entries = entriesToArray(root.value("entries"));
    if (entries.isEmpty()) entries = entriesToArray(original.value("entries"));

    QJsonObject bookData;
    const QString name =
        original.value("name").toString(root.value("name").toString()).trimmed();
    bookData["name"] = name.isEmpty() ? QFileInfo(filePath).completeBaseName() : name;
    if (root.contains("isEnabled")) bookData["isEnabled"] = root.value("isEnabled");
    else if (original.contains("isEnabled")) bookData["isEnabled"] = original.value("isEnabled");
    bookData["entries"] = entries;

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
    WorldInfoBookEntity book = parseJsonToBook(bookData);
    if (book.name.isEmpty()) {
        emit operationError("导入世界书失败：书名为空");
        return {};
    }

    // entries 既可能是数组（角色卡 / 我们的导出），也可能是 {uid: entry} 对象（ST world 文件）
    const QJsonArray entries = entriesToArray(bookData.value("entries"));
    if (entries.isEmpty()) {
        emit operationError("导入世界书失败：没有条目");
        return {};
    }

    // 先全部解析，再落库：避免"书插进去了、条目解析失败"留下空壳。
    // bookId 先留空，确定 book id 之后再回填。
    QList<WorldInfoEntryEntity> parsed;
    for (const QJsonValue& e : entries) {
        if (!e.isObject()) continue;
        parsed.append(parseJsonToEntry(e.toObject(), QString()));
    }
    if (parsed.isEmpty()) {
        emit operationError("导入世界书失败：条目格式无效");
        return {};
    }

    // 同名书就地覆盖：保留 id（角色的 world_book_ids 链接不会断）与启用状态。
    // 以前每次导入都新建一本，用户想"重新导入修数据"时会得到第二本书、角色还指着旧的。
    WorldInfoBookEntity existing;
    for (const WorldInfoBookEntity& b : m_BookDao->selectAll()) {
        if (b.name == book.name) {
            existing = b;
            break;
        }
    }

    if (!existing.id.isEmpty()) {
        book.id = existing.id;
        book.isEnabled = existing.isEnabled;
        book.createdAt = existing.createdAt;
        book.updatedAt = QDateTime::currentDateTime();

        if (!m_BookDao->update(book)) {
            emit operationError("更新世界书失败");
            return {};
        }
        for (WorldInfoEntryEntity& entry : parsed) entry.bookId = book.id;
        if (!m_EntryDao->replaceForBook(book.id, parsed)) {
            emit operationError("替换世界书条目失败");
            return {};
        }

        emit bookUpdated(book.id);
        emit entryListChanged(book.id);
        emit bookListChanged();
        qInfo() << "[WorldInfoService] 同名世界书已覆盖更新:" << book.name << "id:" << book.id
                << "条目数:" << parsed.size();
        return book.id;
    }

    if (!m_BookDao->insert(book)) {
        emit operationError("导入世界书失败");
        return {};
    }

    for (WorldInfoEntryEntity& entry : parsed) entry.bookId = book.id;
    for (const WorldInfoEntryEntity& entry : parsed) {
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
        entryObj["depth"] = entry.insertDepth;
        // 内部格式用 disable（取反）；不写的话导出再导入会丢掉"条目被禁用"这个状态
        entryObj["disable"] = !entry.isEnabled;
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
    // ST 世界书文件本身没有 isEnabled 字段；只有我们自己导出的文件有。
    book.isEnabled = json.contains("isEnabled")
        ? json.value("isEnabled").toBool(false)
        : json.value("is_enabled").toBool(false);
    book.createdAt = QDateTime::currentDateTime();
    book.updatedAt = book.createdAt;
    return book;
}

WorldInfoEntryEntity WorldInfoService::parseJsonToEntry(const QJsonObject& json, const QString& bookId)
{
    WorldInfoEntryEntity entry;
    entry.id = Utils::generateUuid();
    // entry.CompatibleId;//原版有个兼容id
    entry.bookId = bookId;

    // 角色卡 V2 与 ST world 文件是两套字段名：
    //   V2（character_book）: keys / secondary_keys / position:"before_char" /
    //                        insertion_order / enabled / extensions.depth ...
    //   world（worlds/*.json）: key / keysecondary / position:0 / order / disable / depth ...
    // ST 的 convertCharacterBook 是「extensions 优先、顶层兜底」，这里照做。
    const QJsonObject ext = json.value("extensions").toObject();

    entry.keywords = jsonStrings(json.contains("key") ? json.value("key") : json.value("keys"));
    entry.secondaryKeywords = jsonStrings(json.contains("keysecondary")
        ? json.value("keysecondary") : json.value("secondary_keys"));

    entry.name = json.value("comment").toString(json.value("name").toString()).trimmed();
    if (entry.name.isEmpty()) entry.name = QStringLiteral("未命名条目");
    entry.content = json.value("content").toString();

    // 启用：world 文件用 disable（取反），角色卡用 enabled（缺省 true）
    if (json.contains("disable"))
        entry.isEnabled = !json.value("disable").toBool(false);
    else
        entry.isEnabled = json.value("enabled").toBool(true);

    entry.constant = json.value("constant").toBool(false);
    entry.vectorized = pickValue(ext, json, "vectorized", "vectorized").toBool(false);

    // 位置：extensions.position（数字）优先；V2 的 position 是 "before_char"/"after_char"；
    // world 文件的 position 是数字。ST 对未知字符串按 after 处理。
    int position = static_cast<int>(WorldInfoEntryEntity::BeforeChar);
    const QJsonValue extPosition = ext.value("position");
    const QJsonValue rawPosition = json.value("position");
    if (!extPosition.isUndefined() && !extPosition.isNull()) {
        position = extPosition.toInt();
    } else if (rawPosition.isString()) {
        position = rawPosition.toString() == QLatin1String("before_char")
            ? static_cast<int>(WorldInfoEntryEntity::BeforeChar)
            : static_cast<int>(WorldInfoEntryEntity::AfterChar);
    } else if (!rawPosition.isUndefined() && !rawPosition.isNull()) {
        position = rawPosition.toInt();
    }
    entry.position = static_cast<WorldInfoEntryEntity::Position>(position);

    const QJsonValue role = pickValue(ext, json, "role", "role");
    entry.insertRole = (role.isUndefined() || role.isNull())
        ? GlobalConstants::MessageType::System
        : static_cast<GlobalConstants::MessageType>(role.toInt());

    const QJsonValue depth = pickValue(ext, json, "depth", "depth");
    entry.insertDepth = (depth.isUndefined() || depth.isNull()) ? 4 : depth.toInt(4);

    // 角色卡是 insertion_order，world 文件是 order；缺省 100（与实体默认一致）
    entry.sortOrder = json.contains("order")
        ? json.value("order").toInt(100)
        : json.value("insertion_order").toInt(100);

    entry.selective = json.value("selective").toBool(false);
    entry.selectiveLogic = static_cast<WorldInfoEntryEntity::SelectiveLogic>(
        pickValue(ext, json, "selectiveLogic", "selectiveLogic").toInt(0));

    entry.useProbability = pickValue(ext, json, "useProbability", "useProbability").toBool(false);
    entry.probability = pickValue(ext, json, "probability", "probability").toInt(100);

    const QJsonValue scanDepth = pickValue(ext, json, "scan_depth", "scanDepth");
    entry.scanDepth = (scanDepth.isUndefined() || scanDepth.isNull()) ? -1 : scanDepth.toInt(-1);
    entry.useGroupScoring = pickValue(ext, json, "use_group_scoring", "useGroupScoring").toBool(false);
    entry.automationId = pickValue(ext, json, "automation_id", "automationId").toString();

    entry.group = pickValue(ext, json, "group", "group").toString();
    entry.groupOverride = pickValue(ext, json, "group_override", "groupOverride").toBool(false);
    entry.groupWeight = pickValue(ext, json, "group_weight", "groupWeight").toInt(0);

    entry.excludeRecursion = pickValue(ext, json, "exclude_recursion", "excludeRecursion").toBool(false);
    entry.preventRecursion = pickValue(ext, json, "prevent_recursion", "preventRecursion").toBool(false);
    entry.delayUntilRecursion =
        pickValue(ext, json, "delay_until_recursion", "delayUntilRecursion").toBool(false);
    entry.ignoreBudget = pickValue(ext, json, "ignore_budget", "ignoreBudget").toBool(false);

    entry.matchCharacterDescription = ext.value("match_character_description").toBool(false);
    entry.matchCharacterPersonality = ext.value("match_character_personality").toBool(false);
    entry.matchScenario = ext.value("match_scenario").toBool(false);
    entry.matchPersonaDescription = ext.value("match_persona_description").toBool(false);
    entry.matchCharacterDepthPrompt = ext.value("match_character_depth_prompt").toBool(false);
    entry.matchCreatorNotes = ext.value("match_creator_notes").toBool(false);

    entry.displayIndex = ext.value("display_index").toInt(0);
    entry.uid = json.value("uid").toInt(ext.value("uid").toInt(0));
    entry.sticky = ext.value("sticky").toInt(0);
    entry.cooldown = ext.value("cooldown").toInt(0);
    entry.delay = ext.value("delay").toInt(0);

    entry.extensions = Utils::jsonObjectToVariantMap(ext);
    return entry;
}