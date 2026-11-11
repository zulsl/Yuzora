#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QUuid>

#include "CharacterDao.h"
#include "ConnectionProviderRegistry.h"
#include "IConnectionProvider.h"
#include "Utils.h"

// CharacterDao 只需要 Utils 的 JSON/UUID 工具；测试用轻量实现隔离文件系统。
void Utils::writeLog(const QString&, const QString&, const QString&) {}
QString Utils::stringListToJson(const QStringList& values)
{
    return QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(values))
                                 .toJson(QJsonDocument::Compact));
}
QStringList Utils::jsonToStringList(const QVariant& value)
{
    const QJsonDocument doc = QJsonDocument::fromJson(value.toString().toUtf8());
    QStringList result;
    for (const QJsonValue& item : doc.array())
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

class CharacterTestProvider : public IConnectionProvider
{
public:
    CharacterTestProvider()
    {
        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                       QStringLiteral("character-dao-test"));
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());
        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral(R"(
            CREATE TABLE characters (
                id TEXT PRIMARY KEY, name TEXT, description TEXT, first_message TEXT,
                alternate_greetings TEXT, creator_notes TEXT, creator TEXT, personality TEXT,
                scenario TEXT, example_dialogue TEXT, system_prompt TEXT,
                post_history_instructions TEXT, character_version TEXT, tags TEXT,
                card_format_version INTEGER, avatar_path TEXT, world_book_ids TEXT,
                last_session_id TEXT, is_favorite INTEGER, extensions TEXT,
                created_at TEXT, updated_at TEXT
            ))")));
    }

    ~CharacterTestProvider() override
    {
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(QStringLiteral("character-dao-test"));
    }

    QSqlDatabase connection() override { return db; }

private:
    QSqlDatabase db;
};

class CharacterDaoProviderTest : public QObject
{
    Q_OBJECT

private slots:
    void customQueriesUseInjectedProvider();
};

void CharacterDaoProviderTest::customQueriesUseInjectedProvider()
{
    CharacterTestProvider provider;
    CharacterDao dao;
    dao.setConnectionProvider(&provider);

    CharacterEntity entity;
    entity.id = QStringLiteral("char-1");
    entity.name = QStringLiteral("Alice");
    entity.description = QStringLiteral("desc");
    entity.alternateGreetings = {QStringLiteral("hi")};
    entity.tags = {QStringLiteral("tag")};
    entity.avatarPath = QStringLiteral("old.png");
    entity.updatedAt = QDateTime::currentDateTime();
    entity.createdAt = entity.updatedAt;

    QVERIFY(dao.insert(entity));
    const QString id = entity.id;
    QCOMPARE(dao.selectById(id).name, QStringLiteral("Alice"));
    QCOMPARE(dao.searchByName(QStringLiteral("Ali")).size(), 1);

    entity.name = QStringLiteral("Bob");
    QVERIFY(dao.update(entity));
    QCOMPARE(dao.selectById(id).name, QStringLiteral("Bob"));

    QVERIFY(dao.updateAvatar(id, QStringLiteral("new.png")));
    QCOMPARE(dao.selectById(id).avatarPath, QStringLiteral("new.png"));

    QVERIFY(dao.remove(id));
    QVERIFY(dao.selectAll().isEmpty());
}

QTEST_GUILESS_MAIN(CharacterDaoProviderTest)

#include "CharacterDaoProviderTest.moc"
