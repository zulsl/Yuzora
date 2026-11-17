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
bool Utils::writeJsonFile(const QString&, const QJsonDocument&) { return false; }

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

QTEST_GUILESS_MAIN(WorldInfoServiceTest)

#include "WorldInfoServiceTest.moc"
