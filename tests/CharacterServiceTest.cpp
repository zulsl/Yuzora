#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "CharacterDao.h"
#include "CharacterService.h"
#include "ConnectionProviderRegistry.h"
#include "IConnectionProvider.h"
#include "Utils.h"
#include "WorldInfoService.h"
#include "AppException.h"
#include "DatabaseManager.h"

// CharacterService 的这些依赖在测试路径中不执行；仅提供链接符号。
AppException::AppException(ErrorCode code, const QString& message)
    : m_code(code), m_message(message), m_whatBuffer(message.toUtf8()) {}
const char* AppException::what() const noexcept { return m_whatBuffer.constData(); }
QString AppException::getMessage() const { return m_message; }

DatabaseManager& DatabaseManager::instance()
{
    qFatal("DatabaseManager::instance() should not be called in this test");
    static DatabaseManager* invalid = nullptr;
    return *invalid;
}
bool DatabaseManager::beginTransaction() { return false; }
bool DatabaseManager::commitTransaction() { return false; }
bool DatabaseManager::rollbackTransaction() { return false; }

// CharacterService 的文件/世界书依赖在本次测试中用空实现隔离。
void Utils::writeLog(const QString&, const QString&, const QString&) {}
QString Utils::stringListToJson(const QStringList& values)
{
    return QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(values))
                                 .toJson(QJsonDocument::Compact));
}
QStringList Utils::jsonToStringList(const QVariant& value)
{
    QStringList result;
    const QJsonDocument doc = QJsonDocument::fromJson(value.toString().toUtf8());
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
QString Utils::getAvatarDirPath() { return QStringLiteral("."); }
QJsonObject Utils::extractPngMetadata(const QString&) { return {}; }
bool Utils::injectPngMetadata(const QString&, const QJsonObject&) { return false; }
QJsonDocument Utils::readJsonFile(const QString&) { return {}; }
bool Utils::writeJsonFile(const QString&, const QJsonDocument&) { return false; }

// 注入 nullptr 的 WorldInfoService 时不应触达这两个符号。
WorldInfoService& WorldInfoService::instance()
{
    qFatal("WorldInfoService::instance() should not be called in this test");
    static WorldInfoService* invalid = nullptr;
    return *invalid;
}
QString WorldInfoService::importWorldBook(QJsonObject&) { return {}; }

class CharacterServiceTestProvider : public IConnectionProvider
{
public:
    CharacterServiceTestProvider()
    {
        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                       QStringLiteral("character-service-test"));
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

    ~CharacterServiceTestProvider() override
    {
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(QStringLiteral("character-service-test"));
    }

    QSqlDatabase connection() override { return db; }

private:
    QSqlDatabase db;
};

class CharacterServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void createUpdateDeleteThroughInjectedDao();
};

void CharacterServiceTest::createUpdateDeleteThroughInjectedDao()
{
    CharacterServiceTestProvider provider;
    CharacterDao dao;
    dao.setConnectionProvider(&provider);
    CharacterService service(&dao, static_cast<WorldInfoService*>(nullptr));

    CharacterEntity entity;
    entity.name = QStringLiteral("Alice");
    const QString id = service.createCharacter(entity);
    QVERIFY(!id.isEmpty());
    QCOMPARE(service.getAllCharacters().size(), 1);
    QCOMPARE(service.getCharacterById(id).name, QStringLiteral("Alice"));

    CharacterEntity loaded = service.getCharacterById(id);
    loaded.name = QStringLiteral("Bob");
    QVERIFY(service.updateCharacter(loaded));
    QCOMPARE(service.getCharacterById(id).name, QStringLiteral("Bob"));

    QVERIFY(service.deleteCharacter(id));
    QVERIFY(service.getAllCharacters().isEmpty());
}

QTEST_GUILESS_MAIN(CharacterServiceTest)

#include "CharacterServiceTest.moc"
