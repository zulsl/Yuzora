#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QFile>

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
bool Utils::writeJsonFile(const QString& path, const QJsonDocument& doc)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    return file.write(doc.toJson(QJsonDocument::Indented)) >= 0;
}

// 注入 nullptr 的 WorldInfoService 时不应触达这两个符号。
WorldInfoService& WorldInfoService::instance()
{
    qFatal("WorldInfoService::instance() should not be called in this test");
    static WorldInfoService* invalid = nullptr;
    return *invalid;
}
QString WorldInfoService::importWorldBook(QJsonObject&) { return {}; }

// 角色卡导出会把绑定的世界书塞进 data.character_book；这里给一个可辨认的桩，
// 用来验证"绑了书才会写 character_book"这条管线。书的真实序列化在
// WorldInfoServiceTest 里测（那边链接的是真的 WorldInfoService）。
QJsonObject WorldInfoService::bookToCharacterBookJson(const QString& bookId)
{
    if (bookId != QLatin1String("stub-book-id")) return {};
    QJsonObject book;
    book["name"] = QStringLiteral("桩世界书");
    book["entries"] = QJsonArray();
    return book;
}

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
    void exportedCardUsesSillyTavernV2Spec();
    void importDetectsCardKindWithoutFileExtension();
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

// 导出的角色卡必须是 ST 认的 V2 规格：spec 写 "chara_card_v2"（以前写成 "v2"，
// ST 会当成 V1 读，data 里的东西全丢），字段放在 data 下、附带 spec_version。
void CharacterServiceTest::exportedCardUsesSillyTavernV2Spec()
{
    CharacterServiceTestProvider provider;
    CharacterDao dao;
    dao.setConnectionProvider(&provider);
    // 传 nullptr：这次不测世界书入库，只测卡片结构
    CharacterService service(&dao, static_cast<WorldInfoService*>(nullptr));

    CharacterEntity entity;
    entity.name = QStringLiteral("测试角色");
    entity.description = QStringLiteral("描述");
    entity.firstMessage = QStringLiteral("开场白");
    entity.alternateGreetings = {QStringLiteral("备选一")};
    entity.systemPrompt = QStringLiteral("系统提示");
    entity.postHistoryInstructions = QStringLiteral("历史后指令");
    entity.scenario = QStringLiteral("场景");
    entity.personality = QStringLiteral("性格");
    entity.exampleDialogue = QStringLiteral("示例对话");
    entity.creator = QStringLiteral("作者");
    entity.creatorNotes = QStringLiteral("作者备注");
    entity.tags = {QStringLiteral("tag-a")};
    entity.characterVersion = QStringLiteral("1.2.3");
    const QString id = service.createCharacter(entity);
    QVERIFY(!id.isEmpty());

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("card.json"));
    QVERIFY(service.exportCharacterToFile(id, path));

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QJsonObject card = QJsonDocument::fromJson(file.readAll()).object();

    QCOMPARE(card.value("spec").toString(), QStringLiteral("chara_card_v2"));
    QCOMPARE(card.value("spec_version").toString(), QStringLiteral("2.0"));

    const QJsonObject data = card.value("data").toObject();
    QCOMPARE(data.value("name").toString(), QStringLiteral("测试角色"));
    QCOMPARE(data.value("first_mes").toString(), QStringLiteral("开场白"));
    QCOMPARE(data.value("system_prompt").toString(), QStringLiteral("系统提示"));
    // 这一项以前根本没导出，导入侧也读不到
    QCOMPARE(data.value("post_history_instructions").toString(), QStringLiteral("历史后指令"));
    QCOMPARE(data.value("alternate_greetings").toArray().size(), 1);
    QCOMPARE(data.value("tags").toArray().size(), 1);
    QVERIFY(data.value("create_date").isString());
    QVERIFY(!data.value("extensions").isUndefined());
    // 没绑定世界书就不该出现 character_book
    QVERIFY(!data.contains("character_book"));
    // V1 兼容别名
    QCOMPARE(card.value("name").toString(), QStringLiteral("测试角色"));
    QCOMPARE(card.value("creatorcomment").toString(), QStringLiteral("作者备注"));
}

// 安卓真机的坑：文档选择器（SAF）返回的 content:// URI 末段是
// image%3A1000000033 这种资源 id，根本没有 .png / .json 后缀，
// 以前只按 QFileInfo::suffix() 判断类型，整张卡被判成"不支持的文件格式"。
// 这里用"无扩展名的文件"模拟那种路径，断言类型是按内容嗅探出来的：
// PNG 走 PNG 分支、JSON 走 JSON 分支（本测试里两个解析函数是桩，
// 所以分别停在"不含角色卡元数据"和"JSON文件格式错误"）。
void CharacterServiceTest::importDetectsCardKindWithoutFileExtension()
{
    CharacterServiceTestProvider provider;
    CharacterDao dao;
    dao.setConnectionProvider(&provider);
    CharacterService service(&dao, static_cast<WorldInfoService*>(nullptr));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const auto writeFile = [&dir](const QString& name, const QByteArray& bytes) {
        const QString path = dir.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return QString();
        file.write(bytes);
        file.close();
        return path;
    };

    // 1) 无扩展名 + PNG 签名 -> 认成 PNG
    const QString pngPath = writeFile(QStringLiteral("card"),
                                      QByteArray("\x89PNG\r\n\x1a\n", 8));
    QVERIFY(!pngPath.isEmpty());
    QSignalSpy pngSpy(&service, &CharacterService::operationError);
    QVERIFY(service.importCharacterFromFile(pngPath).isEmpty());
    QCOMPARE(pngSpy.count(), 1);
    QCOMPARE(pngSpy.first().at(0).toString(),
             QStringLiteral("导入失败：PNG文件不含角色卡元数据"));

    // 2) 无扩展名 + JSON -> 认成 JSON（带 BOM 和前导空白也要认出来）
    const QString jsonPath = writeFile(QStringLiteral("book"),
                                       QByteArray("\xEF\xBB\xBF  \r\n{\"name\":\"x\"}"));
    QVERIFY(!jsonPath.isEmpty());
    QSignalSpy jsonSpy(&service, &CharacterService::operationError);
    QVERIFY(service.importCharacterFromFile(jsonPath).isEmpty());
    QCOMPARE(jsonSpy.count(), 1);
    QCOMPARE(jsonSpy.first().at(0).toString(),
             QStringLiteral("导入失败：JSON文件格式错误"));

    // 3) 无扩展名 + 认不出的内容 -> 才报"不支持"
    const QString junkPath = writeFile(QStringLiteral("junk"), QByteArray("hello"));
    QVERIFY(!junkPath.isEmpty());
    QSignalSpy junkSpy(&service, &CharacterService::operationError);
    QVERIFY(service.importCharacterFromFile(junkPath).isEmpty());
    QCOMPARE(junkSpy.count(), 1);
    QCOMPARE(junkSpy.first().at(0).toString(),
             QStringLiteral("导入失败：不支持的文件格式"));
}

QTEST_GUILESS_MAIN(CharacterServiceTest)
#include "CharacterServiceTest.moc"
