#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "CompletionPresetDao.h"
#include "CompletionPresetService.h"
#include "ConnectionProviderRegistry.h"
#include "IConnectionProvider.h"
#include "PromptDao.h"
#include "Utils.h"

void Utils::writeLog(const QString&, const QString&, const QString&) {}
QString Utils::stringListToJson(const QStringList& values)
{
    return QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(values))
                                 .toJson(QJsonDocument::Compact));
}
QStringList Utils::jsonToStringList(const QVariant& value)
{
    // 与生产实现一致：DAO 把列表存成 JSON 字符串（TEXT）。
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
QJsonDocument Utils::readJsonFile(const QString&) { return {}; }
bool Utils::writeJsonFile(const QString&, const QJsonDocument&) { return false; }

class PresetTestProvider : public IConnectionProvider
{
public:
    PresetTestProvider()
    {
        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                       QStringLiteral("preset-service-test"));
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());

        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral(
            "CREATE TABLE completion_presets ("
            "id TEXT PRIMARY KEY, name TEXT, is_default INTEGER,"
            "max_context_token INTEGER, max_response_token INTEGER, stream_enabled INTEGER,"
            "temperature REAL, top_p REAL, top_k REAL, frequency_penalty REAL,"
            "presence_penalty REAL, repetition_penalty REAL, top_a REAL, min_p REAL,"
            "extra_settings TEXT, created_at TEXT, updated_at TEXT,"
            "max_context_unlocked INTEGER, names_behavior INTEGER, continue_postfix TEXT,"
            "continue_prefill INTEGER, squash_system_messages INTEGER, function_calling INTEGER,"
            "show_thoughts INTEGER, prompt_order TEXT, impersonation_Prompt TEXT,"
            "wi_Format TEXT, scenario_Format TEXT, personality_Format TEXT,"
            "group_nudge_prompt TEXT, new_chat_prompt TEXT, new_group_chat_prompt TEXT,"
            "new_example_chat_prompt TEXT, continue_nudge_prompt TEXT)")));
        QVERIFY(query.exec(QStringLiteral(
            "CREATE TABLE completion_presets_prompts ("
            "id TEXT PRIMARY KEY, identifier TEXT, preset_id TEXT, name TEXT,"
            "is_active INTEGER, role TEXT, content TEXT, system_prompt INTEGER,"
            "marker INTEGER, injection_position INTEGER, injection_depth INTEGER,"
            "injection_order INTEGER, forbid_overrides INTEGER)")));
    }

    ~PresetTestProvider() override
    {
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(QStringLiteral("preset-service-test"));
    }

    QSqlDatabase connection() override { return db; }

private:
    QSqlDatabase db;
};

class CompletionPresetServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void presetAndPromptLifecycle();
    void importSameNameUpdatesInPlace();
    void importPreservesExplicitEmptyChatSettings();
    void importFallsBackToStDefaultsWhenKeysMissing();
    void nullColumnsFallBackToStDefaults();
};

// 最小 ST 预设 JSON：一条 main 提示词
static QJsonObject makeSillyTavernPresetJson(const QString& mainContent)
{
    QJsonObject orderEntry;
    orderEntry["identifier"] = QStringLiteral("main");
    orderEntry["enabled"] = true;

    QJsonObject orderList;
    orderList["character_id"] = 100001;
    orderList["order"] = QJsonArray{orderEntry};

    QJsonObject prompt;
    prompt["identifier"] = QStringLiteral("main");
    prompt["name"] = QStringLiteral("Main");
    prompt["content"] = mainContent;
    prompt["role"] = QStringLiteral("system");
    prompt["system_prompt"] = true;
    prompt["marker"] = false;
    prompt["enabled"] = true;
    prompt["injection_position"] = 0;
    prompt["injection_depth"] = 4;
    prompt["injection_order"] = 100;
    prompt["forbid_overrides"] = false;

    QJsonObject preset;
    preset["openai_max_context"] = 8192;
    preset["openai_max_tokens"] = 512;
    preset["temperature"] = 1.0;
    preset["prompt_order"] = QJsonArray{orderList};
    preset["prompts"] = QJsonArray{prompt};
    return preset;
}

void CompletionPresetServiceTest::presetAndPromptLifecycle()
{
    PresetTestProvider provider;
    ConnectionProviderRegistry::setProvider(&provider);

    CompletionPresetDao presetDao;
    PromptDao promptDao;
    presetDao.setConnectionProvider(&provider);
    promptDao.setConnectionProvider(&provider);

    CompletionPresetService service(&presetDao, &promptDao);

    CompletionPresetEntity preset;
    preset.name = QStringLiteral("Preset");
    const QString presetId = service.createPreset(preset);
    QVERIFY(!presetId.isEmpty());
    QCOMPARE(service.getAllPresets().size(), 1);

    preset.id = presetId;
    preset.name = QStringLiteral("Renamed");
    QVERIFY(service.updatePreset(preset));
    QCOMPARE(service.getPresetById(presetId).name, QStringLiteral("Renamed"));

    PromptEntity prompt;
    prompt.identifier = QStringLiteral("main");
    prompt.name = QStringLiteral("Main");
    prompt.content = QStringLiteral("hello");
    prompt.presetId = presetId;
    QVERIFY(service.addPrompt(presetId, prompt));
    QCOMPARE(service.getPromptsByPresetId(presetId).size(), 1);

    const PromptEntity created = service.getPromptsByPresetId(presetId).first();
    QVERIFY(service.deletePrompt(presetId, created.id));
    QVERIFY(service.getPromptsByPresetId(presetId).isEmpty());
}

// 从 ST 重新导出后再导入：同名预设应就地覆盖（保留 id 与激活状态），
// 不能留下第二份旧数据——否则请求体里会多出 ST 已经关掉的提示词。
void CompletionPresetServiceTest::importSameNameUpdatesInPlace()
{
    PresetTestProvider provider;
    ConnectionProviderRegistry::setProvider(&provider);

    CompletionPresetDao presetDao;
    PromptDao promptDao;
    presetDao.setConnectionProvider(&provider);
    promptDao.setConnectionProvider(&provider);

    CompletionPresetService service(&presetDao, &promptDao);

    QCOMPARE(service.importFromSillyTavernJson(
                 makeSillyTavernPresetJson(QStringLiteral("v1")),
                 QStringLiteral("Fushin")),
             1);
    QCOMPARE(service.getAllPresets().size(), 1);
    const QString presetId = service.getAllPresets().first().id;
    QVERIFY(service.setActivePreset(presetId));
    QCOMPARE(service.getPromptsByPresetId(presetId).first().content, QStringLiteral("v1"));

    QCOMPARE(service.importFromSillyTavernJson(
                 makeSillyTavernPresetJson(QStringLiteral("v2")),
                 QStringLiteral("Fushin")),
             1);

    QCOMPARE(service.getAllPresets().size(), 1);                 // 没有产生第二份
    QCOMPARE(service.getAllPresets().first().id, presetId);      // id 保留
    QVERIFY(service.getPresetById(presetId).isActive);           // 激活状态保留
    const QList<PromptEntity> prompts = service.getPromptsByPresetId(presetId);
    QCOMPARE(prompts.size(), 1);                                 // 旧提示词被整体替换
    QCOMPARE(prompts.first().content, QStringLiteral("v2"));
}

// 狐神抚这类预设显式写了 new_chat_prompt="" / new_example_chat_prompt=""：
// 这是"不要标记"，导入后必须保持空，不能被 ST 默认值覆盖。
void CompletionPresetServiceTest::importPreservesExplicitEmptyChatSettings()
{
    PresetTestProvider provider;
    ConnectionProviderRegistry::setProvider(&provider);

    CompletionPresetDao presetDao;
    PromptDao promptDao;
    presetDao.setConnectionProvider(&provider);
    promptDao.setConnectionProvider(&provider);
    CompletionPresetService service(&presetDao, &promptDao);

    QJsonObject json = makeSillyTavernPresetJson(QStringLiteral("main"));
    json["new_chat_prompt"] = QString();
    json["new_example_chat_prompt"] = QString();
    json["wi_format"] = QString();
    json["scenario_format"] = QString();
    json["personality_format"] = QString();
    QCOMPARE(service.importFromSillyTavernJson(json, QStringLiteral("EmptySettings")), 1);

    const CompletionPresetEntity preset = service.getAllPresets().first();
    QVERIFY(preset.newChatPrompt.isEmpty());
    QVERIFY(preset.newExampleChatPrompt.isEmpty());
    QVERIFY(preset.wiFormat.isEmpty());
    QVERIFY(preset.scenarioFormat.isEmpty());
    QVERIFY(preset.personalityFormat.isEmpty());
}

// JSON 里根本没有这些字段（早期导出/局部预设）：用 ST 的 default_*。
void CompletionPresetServiceTest::importFallsBackToStDefaultsWhenKeysMissing()
{
    PresetTestProvider provider;
    ConnectionProviderRegistry::setProvider(&provider);

    CompletionPresetDao presetDao;
    PromptDao promptDao;
    presetDao.setConnectionProvider(&provider);
    promptDao.setConnectionProvider(&provider);
    CompletionPresetService service(&presetDao, &promptDao);

    QCOMPARE(service.importFromSillyTavernJson(
                 makeSillyTavernPresetJson(QStringLiteral("main")),
                 QStringLiteral("Defaults")),
             1);

    const CompletionPresetEntity preset = service.getAllPresets().first();
    QCOMPARE(preset.newChatPrompt, QStringLiteral("[Start a new Chat]"));
    QCOMPARE(preset.newExampleChatPrompt, QStringLiteral("[Example Chat]"));
    QCOMPARE(preset.wiFormat, QStringLiteral("{0}"));
    QCOMPARE(preset.scenarioFormat, QStringLiteral("{{scenario}}"));
    QCOMPARE(preset.personalityFormat, QStringLiteral("{{personality}}"));
}

// 旧版本建的行这些列是 NULL（不是空串）：读出来要用 ST 默认，
// 否则新建的预设会悄悄少掉 [Start a new Chat]。
void CompletionPresetServiceTest::nullColumnsFallBackToStDefaults()
{
    PresetTestProvider provider;
    ConnectionProviderRegistry::setProvider(&provider);

    CompletionPresetDao presetDao;
    PromptDao promptDao;
    presetDao.setConnectionProvider(&provider);
    promptDao.setConnectionProvider(&provider);
    CompletionPresetService service(&presetDao, &promptDao);

    QSqlQuery query(provider.connection());
    QVERIFY(query.exec(QStringLiteral(
        "INSERT INTO completion_presets (id, name) VALUES ('legacy', 'Legacy')")));

    const CompletionPresetEntity preset = service.getPresetById(QStringLiteral("legacy"));
    QCOMPARE(preset.name, QStringLiteral("Legacy"));
    QCOMPARE(preset.newChatPrompt, QStringLiteral("[Start a new Chat]"));
    QCOMPARE(preset.newExampleChatPrompt, QStringLiteral("[Example Chat]"));
    QCOMPARE(preset.wiFormat, QStringLiteral("{0}"));
    QCOMPARE(preset.scenarioFormat, QStringLiteral("{{scenario}}"));
    QCOMPARE(preset.personalityFormat, QStringLiteral("{{personality}}"));
}

QTEST_GUILESS_MAIN(CompletionPresetServiceTest)

#include "CompletionPresetServiceTest.moc"
