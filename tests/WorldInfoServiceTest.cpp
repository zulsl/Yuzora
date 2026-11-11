#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlQuery>

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
    const QJsonArray array = value.value<QJsonArray>();
    QStringList result;
    for (const QJsonValue& item : array)
        result.append(item.toString());
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
QJsonDocument Utils::readJsonFile(const QString&) { return {}; }
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
};

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
