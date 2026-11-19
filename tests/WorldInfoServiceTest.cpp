#include <QtTest>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "ConnectionProviderRegistry.h"
#include "IConnectionProvider.h"
#include "Utils.h"
#include "WorldInfoBookDao.h"
#include "WorldInfoEntryDao.h"
#include "WorldInfoService.h"

void Utils::writeLog(const QString&, const QString&, const QString&) {}
QString Utils::stringListToJson(const QStringList& values)
{
    return QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(values))
                                 .toJson(QJsonDocument::Compact));
}
QStringList Utils::jsonToStringList(const QVariant& value)
{
    // 与生产实现一致：DAO 把列表存成 JSON 字符串（TEXT），不是 QVariant(QJsonArray)。
    QStringList result;
    QByteArray jsonData = value.toByteArray();
    if (jsonData.isEmpty()) {
        const QString str = value.toString();
        if (str.isEmpty()) return result;
        jsonData = str.toUtf8();
    }
    const QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isArray()) return result;
    for (const QJsonValue& item : doc.array())
        if (item.isString()) result.append(item.toString());
    return result;
}
QString Utils::variantMapToJson(const QVariantMap& map)
{
    return QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(map))
                                 .toJson(QJsonDocument::Compact));
}
QVariantMap Utils::jsonToVariantMap(const QVariant& value)
{
    return QJsonDocument::fromJson(value.toString().toUtf8()).object().toVariantMap();
}
QJsonObject Utils::variantMapToJsonObject(const QVariantMap& map)
{
    return QJsonObject::fromVariantMap(map);
}
QVariantMap Utils::jsonObjectToVariantMap(const QJsonObject& obj)
{
    return obj.toVariantMap();
}
QJsonDocument Utils::readJsonFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll());
}
bool Utils::writeJsonFile(const QString& path, const QJsonDocument& doc)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    return file.write(doc.toJson(QJsonDocument::Indented)) >= 0;
}

class WorldInfoTestProvider : public IConnectionProvider
{
public:
    WorldInfoTestProvider()
    {
        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                       QStringLiteral("world-info-service-test"));
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());

        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral(
            "CREATE TABLE world_info_books ("
            "id TEXT PRIMARY KEY, name TEXT, is_enabled INTEGER,"
            "created_at TEXT, updated_at TEXT)")));
        QVERIFY(query.exec(QStringLiteral(
            "CREATE TABLE world_info_entries ("
            "id TEXT PRIMARY KEY, book_id TEXT, name TEXT, keywords TEXT,"
            "secondary_keywords TEXT, content TEXT, is_enabled INTEGER,"
            "constant INTEGER, vectorized INTEGER, position INTEGER, role INTEGER,"
            "insert_depth INTEGER, sort_order INTEGER, use_probability INTEGER,"
            "probability INTEGER, selective INTEGER, selective_logic INTEGER,"
            "scan_depth INTEGER, extensions TEXT)")));
    }

    ~WorldInfoTestProvider() override
    {
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(QStringLiteral("world-info-service-test"));
    }

    QSqlDatabase connection() override { return db; }

private:
    QSqlDatabase db;
};

class WorldInfoServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void bookAndEntryLifecycle();
    void importsV2CharacterBook();
    void importsInternalWorldEntries();
    void importsStWorldFile();
    void reimportSameNameReplacesEntries();
    void exportUsesSillyTavernWorldFormat();
    void characterBookJsonUsesV2Shape();
};

// 角色卡 V2 的 character_book：keys / position:"after_char" / insertion_order /
// extensions.depth ... 与 ST world 文件的 key / 数字 position / order / depth 不同。
// 以前只按后者解析，导致角色卡导入后 keywords 全空、position 全变成 before。
void WorldInfoServiceTest::importsV2CharacterBook()
{
    WorldInfoTestProvider provider;
    WorldInfoBookDao bookDao;
    WorldInfoEntryDao entryDao;
    bookDao.setConnectionProvider(&provider);
    entryDao.setConnectionProvider(&provider);
    WorldInfoService service(&bookDao, &entryDao);

    QJsonObject extensions;
    extensions["position"] = 1;
    extensions["depth"] = 1;
    extensions["role"] = 2;
    extensions["probability"] = 80;
    extensions["useProbability"] = true;
    extensions["vectorized"] = true;

    QJsonObject entry;
    entry["comment"] = QStringLiteral("AfterEntry");
    entry["content"] = QStringLiteral("AFTER");
    entry["keys"] = QJsonArray{QStringLiteral("k1"), QStringLiteral("k2")};
    entry["secondary_keys"] = QJsonArray{QStringLiteral("s1")};
    entry["position"] = QStringLiteral("after_char");
    entry["enabled"] = true;
    entry["constant"] = true;
    entry["insertion_order"] = 42;
    entry["extensions"] = extensions;

    QJsonObject book;
    book["name"] = QStringLiteral("CardBook");
    book["entries"] = QJsonArray{entry};

    const QString bookId = service.importWorldBook(book);
    QVERIFY(!bookId.isEmpty());

    const QList<WorldInfoEntryEntity> entries = service.getEntriesByBook(bookId);
    QCOMPARE(entries.size(), 1);
    const WorldInfoEntryEntity& e = entries.first();
    QCOMPARE(e.name, QStringLiteral("AfterEntry"));
    QCOMPARE(e.keywords, (QStringList{QStringLiteral("k1"), QStringLiteral("k2")}));
    QCOMPARE(e.secondaryKeywords, QStringList{QStringLiteral("s1")});
    QCOMPARE(static_cast<int>(e.position),
             static_cast<int>(WorldInfoEntryEntity::AfterChar));
    QCOMPARE(e.insertDepth, 1);
    QCOMPARE(static_cast<int>(e.insertRole),
             static_cast<int>(GlobalConstants::MessageType::Assistant));
    QCOMPARE(e.sortOrder, 42);
    QCOMPARE(e.probability, 80);
    QVERIFY(e.useProbability);
    QVERIFY(e.vectorized);
    QVERIFY(e.isEnabled);
    QVERIFY(e.constant);
}

// ST world 文件用 key / 数字 position / order / disable / depth；
// entries 可能是 {uid: entry} 对象，要按 uid 数值顺序导入。
void WorldInfoServiceTest::importsInternalWorldEntries()
{
    WorldInfoTestProvider provider;
    WorldInfoBookDao bookDao;
    WorldInfoEntryDao entryDao;
    bookDao.setConnectionProvider(&provider);
    entryDao.setConnectionProvider(&provider);
    WorldInfoService service(&bookDao, &entryDao);

    QJsonObject first;
    first["comment"] = QStringLiteral("First");
    first["key"] = QJsonArray{QStringLiteral("w1")};
    first["position"] = 0;
    first["order"] = 7;
    first["depth"] = 3;
    first["disable"] = false;

    QJsonObject second;
    second["comment"] = QStringLiteral("Second");
    second["key"] = QJsonArray{QStringLiteral("w2")};
    second["position"] = 1;
    second["order"] = 9;

    QJsonObject entriesObject;
    entriesObject["10"] = first;   // 对象 key 是字符串，数值排序后 2 在 10 前面
    entriesObject["2"] = second;

    QJsonObject book;
    book["name"] = QStringLiteral("WorldFile");
    book["entries"] = entriesObject;

    const QString bookId = service.importWorldBook(book);
    QVERIFY(!bookId.isEmpty());

    const QList<WorldInfoEntryEntity> entries = service.getEntriesByBook(bookId);
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries[0].name, QStringLiteral("Second"));   // uid=2 先插
    QCOMPARE(entries[1].name, QStringLiteral("First"));    // uid=10 后插
    QCOMPARE(entries[1].keywords, QStringList{QStringLiteral("w1")});
    QCOMPARE(static_cast<int>(entries[0].position),
             static_cast<int>(WorldInfoEntryEntity::AfterChar));
    QCOMPARE(entries[1].sortOrder, 7);
    QCOMPARE(entries[1].insertDepth, 3);
    QVERIFY(entries[1].isEnabled);
}

// ST world 文件整体导入：{ entries:{uid:...}, originalData:{name,entries:[V2]} }。
// 必须优先用内部 entries（字段准），书名从 originalData.name 取，不能把两种 entries 混在一起。
void WorldInfoServiceTest::importsStWorldFile()
{
    WorldInfoTestProvider provider;
    WorldInfoBookDao bookDao;
    WorldInfoEntryDao entryDao;
    bookDao.setConnectionProvider(&provider);
    entryDao.setConnectionProvider(&provider);
    WorldInfoService service(&bookDao, &entryDao);

    QJsonObject innerEntry;
    innerEntry["comment"] = QStringLiteral("Inner");
    innerEntry["key"] = QJsonArray{QStringLiteral("k")};
    innerEntry["position"] = 1;
    innerEntry["order"] = 5;
    innerEntry["disable"] = false;

    QJsonObject innerEntries;
    innerEntries["7"] = innerEntry;

    QJsonObject v2Entry;
    v2Entry["comment"] = QStringLiteral("V2");
    v2Entry["keys"] = QJsonArray{QStringLiteral("v2")};
    v2Entry["position"] = QStringLiteral("before_char");
    QJsonObject original;
    original["name"] = QStringLiteral("MyWorld");
    original["entries"] = QJsonArray{v2Entry};

    QJsonObject root;
    root["entries"] = innerEntries;
    root["originalData"] = original;

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("MyWorld.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QJsonDocument(root).toJson());
    file.close();

    QCOMPARE(service.importBookFromSillyTavern(path), 1);
    QCOMPARE(service.getAllBooks().size(), 1);

    const WorldInfoBookEntity book = service.getAllBooks().first();
    QCOMPARE(book.name, QStringLiteral("MyWorld"));

    const QList<WorldInfoEntryEntity> entries = service.getEntriesByBook(book.id);
    QCOMPARE(entries.size(), 1);   // 只用内部 entries，originalData 的 V2 条目不应混入
    QCOMPARE(entries.first().name, QStringLiteral("Inner"));
    QCOMPARE(entries.first().keywords, QStringList{QStringLiteral("k")});
    QCOMPARE(static_cast<int>(entries.first().position),
             static_cast<int>(WorldInfoEntryEntity::AfterChar));
    QCOMPARE(entries.first().sortOrder, 5);
}

// 重新导入同名世界书：书 id 保持不变（角色的 world_book_ids 不会断），条目整体替换。
void WorldInfoServiceTest::reimportSameNameReplacesEntries()
{
    WorldInfoTestProvider provider;
    WorldInfoBookDao bookDao;
    WorldInfoEntryDao entryDao;
    bookDao.setConnectionProvider(&provider);
    entryDao.setConnectionProvider(&provider);
    WorldInfoService service(&bookDao, &entryDao);

    QJsonObject oldEntry;
    oldEntry["comment"] = QStringLiteral("Old");
    oldEntry["content"] = QStringLiteral("OLD");
    QJsonObject oldBook;
    oldBook["name"] = QStringLiteral("SameName");
    oldBook["entries"] = QJsonArray{oldEntry};

    const QString bookId = service.importWorldBook(oldBook);
    QVERIFY(!bookId.isEmpty());
    QCOMPARE(service.getEntriesByBook(bookId).size(), 1);

    QJsonObject newEntry;
    newEntry["comment"] = QStringLiteral("New");
    newEntry["content"] = QStringLiteral("NEW");
    QJsonObject newBook;
    newBook["name"] = QStringLiteral("SameName");
    newBook["entries"] = QJsonArray{newEntry, newEntry};

    const QString sameId = service.importWorldBook(newBook);
    QCOMPARE(sameId, bookId);
    QCOMPARE(service.getAllBooks().size(), 1);
    const QList<WorldInfoEntryEntity> entries = service.getEntriesByBook(bookId);
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries.first().name, QStringLiteral("New"));
    QVERIFY(service.getEntriesByBook(bookId).first().content == QStringLiteral("NEW"));
}

void WorldInfoServiceTest::bookAndEntryLifecycle()
{
    WorldInfoTestProvider provider;
    WorldInfoBookDao bookDao;
    WorldInfoEntryDao entryDao;
    bookDao.setConnectionProvider(&provider);
    entryDao.setConnectionProvider(&provider);

    WorldInfoService service(&bookDao, &entryDao);

    const QString bookId = service.createBook(QStringLiteral("Book"));
    QVERIFY(!bookId.isEmpty());
    QCOMPARE(service.getAllBooks().size(), 1);

    WorldInfoBookEntity book = service.getBookById(bookId);
    book.name = QStringLiteral("Renamed");
    QVERIFY(service.updateBook(book));
    QCOMPARE(service.getBookById(bookId).name, QStringLiteral("Renamed"));

    const QString entryId = service.createEntry(bookId, QStringLiteral("Entry"));
    QVERIFY(!entryId.isEmpty());
    QCOMPARE(service.getEntriesByBook(bookId).size(), 1);

    QVERIFY(service.toggleEntryEnabled(entryId));
    QVERIFY(!service.getEntriesByBook(bookId).first().isEnabled);

    QVERIFY(service.toggleBookEnabled(bookId));
    QVERIFY(service.getBookById(bookId).isEnabled);
    QCOMPARE(service.getEntryForEnabledBooks().size(), 0); // 条目当前被禁用

    QVERIFY(service.toggleEntryEnabled(entryId));
    QCOMPARE(service.getEntryForEnabledBooks().size(), 1);

    QVERIFY(service.deleteEntry(entryId));
    QVERIFY(service.getEntriesByBook(bookId).isEmpty());

    QVERIFY(service.deleteBook(bookId));
    QVERIFY(service.getAllBooks().isEmpty());
}

// 导出的世界书必须是 ST 内部格式：entries 是 {"<uid>": {...}} **对象**而不是数组，
// 顶层字段用 ST 的 camelCase，禁用写 disable。以前导出的是数组，ST 直接读不出来。
void WorldInfoServiceTest::exportUsesSillyTavernWorldFormat()
{
    WorldInfoTestProvider provider;
    WorldInfoBookDao bookDao;
    WorldInfoEntryDao entryDao;
    bookDao.setConnectionProvider(&provider);
    entryDao.setConnectionProvider(&provider);
    WorldInfoService service(&bookDao, &entryDao);

    QJsonObject entry;
    entry["key"] = QJsonArray{QStringLiteral("k1")};
    entry["comment"] = QStringLiteral("条目一");
    entry["content"] = QStringLiteral("内容");
    entry["position"] = 4;   // @depth
    entry["depth"] = 2;
    entry["role"] = 1;
    entry["order"] = 30;
    entry["disable"] = true;
    // group 我们没有对应的数据库列，只在 extensions 里持久化：
    // 导入时补进去、导出时抬回顶层，这样它不会在一次往返里消失。
    entry["group"] = QStringLiteral("g1");

    QJsonObject book;
    book["name"] = QStringLiteral("导出书");
    book["entries"] = QJsonArray{entry};
    const QString bookId = service.importWorldBook(book);
    QVERIFY(!bookId.isEmpty());

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("book.json"));
    QVERIFY(service.exportBookToSillyTavern(bookId, path));

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QJsonObject exported = QJsonDocument::fromJson(file.readAll()).object();

    QCOMPARE(exported.value("name").toString(), QStringLiteral("导出书"));

    const QJsonValue entriesValue = exported.value("entries");
    QVERIFY2(entriesValue.isObject(),
             "ST 按 Object.entries 遍历世界书，entries 必须是对象");
    const QJsonObject entriesObj = entriesValue.toObject();
    QCOMPARE(entriesObj.size(), 1);

    const QString uidKey = entriesObj.keys().first();
    QCOMPARE(uidKey, QStringLiteral("0"));
    const QJsonObject first = entriesObj.value(uidKey).toObject();
    QCOMPARE(first.value("uid").toInt(), 0);
    QCOMPARE(first.value("comment").toString(), QStringLiteral("条目一"));
    QCOMPARE(first.value("content").toString(), QStringLiteral("内容"));
    QCOMPARE(first.value("key").toArray().size(), 1);
    QCOMPARE(first.value("position").toInt(), 4);
    QCOMPARE(first.value("depth").toInt(), 2);
    QCOMPARE(first.value("order").toInt(), 30);
    QVERIFY(first.value("disable").toBool());
    QCOMPARE(first.value("group").toString(), QStringLiteral("g1"));

    // 往返：把导出物再导一遍，条目的位置/深度/顺序/启用状态都要保持
    QJsonObject roundTrip = exported;
    const QString reimportedId = service.importWorldBook(roundTrip);
    QCOMPARE(reimportedId, bookId);   // 同名就地覆盖

    const QList<WorldInfoEntryEntity> reloaded = service.getEntriesByBook(reimportedId);
    QCOMPARE(reloaded.size(), 1);
    const WorldInfoEntryEntity& e = reloaded.first();
    QCOMPARE(static_cast<int>(e.position), 4);
    QCOMPARE(e.insertDepth, 2);
    QCOMPARE(e.sortOrder, 30);
    QVERIFY(!e.isEnabled);
    // group 没有独立列，靠 extensions 持久化——这正是导出时能抬回顶层的前提
    QCOMPARE(e.extensions.value(QStringLiteral("group")).toString(), QStringLiteral("g1"));
}

// 角色卡里的 character_book 是 V2 规范格式，和世界书文件不一样：
// entries 是**数组**、字段名 snake_case、position 只能是 before_char / after_char。
// 其它位置（@depth / 作者注释 / 示例消息）靠 extensions.position 表达，
// 这也是 ST 的 convertCharacterBook 认的写法。
void WorldInfoServiceTest::characterBookJsonUsesV2Shape()
{
    WorldInfoTestProvider provider;
    WorldInfoBookDao bookDao;
    WorldInfoEntryDao entryDao;
    bookDao.setConnectionProvider(&provider);
    entryDao.setConnectionProvider(&provider);
    WorldInfoService service(&bookDao, &entryDao);

    QJsonObject inBookEntry;
    inBookEntry["key"] = QJsonArray{QStringLiteral("k1")};
    inBookEntry["keysecondary"] = QJsonArray{QStringLiteral("s1")};
    inBookEntry["comment"] = QStringLiteral("深度条目");
    inBookEntry["content"] = QStringLiteral("正文");
    inBookEntry["position"] = 4;      // @depth，V2 只能用 extensions 表达
    inBookEntry["depth"] = 3;
    inBookEntry["role"] = 1;
    inBookEntry["order"] = 55;
    inBookEntry["disable"] = false;

    QJsonObject book;
    book["name"] = QStringLiteral("卡片书");
    book["entries"] = QJsonArray{inBookEntry};
    const QString bookId = service.importWorldBook(book);
    QVERIFY(!bookId.isEmpty());

    const QJsonObject cardBook = service.bookToCharacterBookJson(bookId);
    QVERIFY(!cardBook.isEmpty());
    QCOMPARE(cardBook.value("name").toString(), QStringLiteral("卡片书"));
    QVERIFY(cardBook.value("entries").isArray());

    const QJsonArray entries = cardBook.value("entries").toArray();
    QCOMPARE(entries.size(), 1);
    const QJsonObject e = entries.first().toObject();

    QCOMPARE(e.value("id").toInt(), 0);
    QCOMPARE(e.value("keys").toArray().size(), 1);
    QCOMPARE(e.value("secondary_keys").toArray().size(), 1);
    QCOMPARE(e.value("comment").toString(), QStringLiteral("深度条目"));
    QCOMPARE(e.value("content").toString(), QStringLiteral("正文"));
    QCOMPARE(e.value("insertion_order").toInt(), 55);
    QVERIFY(e.value("enabled").toBool());
    // V2 的 position 只有两个合法值，真实的 @depth 放在 extensions 里
    QCOMPARE(e.value("position").toString(), QStringLiteral("before_char"));
    const QJsonObject ext = e.value("extensions").toObject();
    QCOMPARE(ext.value("position").toInt(), 4);
    QCOMPARE(ext.value("depth").toInt(), 3);
    QCOMPARE(ext.value("role").toInt(), 1);

    // 空书 / 不存在的书 → 空对象（角色卡那边据此决定不写 character_book）
    QVERIFY(service.bookToCharacterBookJson(QStringLiteral("不存在")).isEmpty());

    const QString emptyBookId = service.createBook(QStringLiteral("空书"));
    QVERIFY(!emptyBookId.isEmpty());
    QVERIFY(service.bookToCharacterBookJson(emptyBookId).isEmpty());
}

QTEST_GUILESS_MAIN(WorldInfoServiceTest)

#include "WorldInfoServiceTest.moc"
