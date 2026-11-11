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
};

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

QTEST_GUILESS_MAIN(CompletionPresetServiceTest)

#include "CompletionPresetServiceTest.moc"
