#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include <QUuid>

#include "ApiConfigViewModel.h"
#include "ApiConfigDraft.h"
#include "ApiConfigService.h"
#include "ApiConfigDao.h"
#include "ConnectionProviderRegistry.h"
#include "IConnectionProvider.h"
#include "Utils.h"
#include "DatabaseManager.h"
#include "ModelFactory.h"
#include "BaseApiClient.h"
#include "GlobalConstants.h"

// ======================================
// 链接桩
// ======================================
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

DatabaseManager& DatabaseManager::instance()
{
    qFatal("DatabaseManager::instance() should not be called in this test");
    static DatabaseManager* invalid = nullptr;
    return *invalid;
}
bool DatabaseManager::beginTransaction() { return false; }
bool DatabaseManager::commitTransaction() { return false; }
bool DatabaseManager::rollbackTransaction() { return false; }

// 拉取模型列表需要真实网络客户端，本测试不覆盖那条路径
ModelFactory& ModelFactory::instance()
{
    qFatal("ModelFactory::instance() should not be called in this test");
    static ModelFactory* invalid = nullptr;
    return *invalid;
}
BaseApiClient* ModelFactory::createClient(GlobalConstants::ModelType) { return nullptr; }

// ======================================
// 内存库
// ======================================
class ApiConfigTestProvider : public IConnectionProvider
{
public:
    ApiConfigTestProvider()
    {
        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                       QStringLiteral("api-config-vm-test"));
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());
        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral(R"(
            CREATE TABLE api_configs (
                id TEXT NOT NULL PRIMARY KEY,
                name TEXT NOT NULL,
                model_type INTEGER NOT NULL DEFAULT 0,
                is_builtin INTEGER NOT NULL DEFAULT 0,
                is_openai_compatible INTEGER NOT NULL DEFAULT 1,
                base_domain TEXT,
                api_path TEXT,
                api_key TEXT,
                available_models TEXT,
                current_model TEXT,
                extra_headers TEXT,
                extra_params TEXT,
                timeout INTEGER NOT NULL DEFAULT 60000,
                is_active INTEGER NOT NULL DEFAULT 0,
                created_at TEXT NOT NULL,
                updated_at TEXT NOT NULL
            ))")));
    }

    ~ApiConfigTestProvider() override
    {
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(QStringLiteral("api-config-vm-test"));
    }

    QSqlDatabase connection() override { return db; }

private:
    QSqlDatabase db;
};

// 先建库 + DAO + Service，测试里再按需插数据、最后才构造 ViewModel
struct Core
{
    ApiConfigTestProvider provider;
    ApiConfigDao dao;
    ApiConfigService service;

    Core() : service(&dao)
    {
        dao.setConnectionProvider(&provider);
    }

    ApiConfigEntity makeConfig(const QString& name, const QStringList& models,
                               const QString& currentModel, bool active)
    {
        ApiConfigEntity config;
        config.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        config.name = name;
        config.modelType = GlobalConstants::ModelType::OpenAI;
        config.isOpenAICompatible = true;
        config.baseDomain = QStringLiteral("https://example.test/v1");
        config.apiPath = QStringLiteral("/chat/completions");
        config.availableModels = models;
        config.currentModel = currentModel;
        config.isActive = active;
        config.createdAt = QDateTime::currentDateTime();
        config.updatedAt = config.createdAt;
        return config;
    }
};

class ApiConfigViewModelTest : public QObject
{
    Q_OBJECT

private slots:
    void selectsActiveConfigOnLoad();
    void persistsDefaultCurrentModel();
    void editsAreDebounced();
    void switchingConfigFlushesPendingEdits();
    void applyCurrentConfigMarksActive();
    void deletingCurrentConfigFallsBack();
    void fetchWithoutConfigReportsError();
};

// ======================================
// 用例
// ======================================
void ApiConfigViewModelTest::selectsActiveConfigOnLoad()
{
    Core c;
    const ApiConfigEntity inactive = c.makeConfig(QStringLiteral("备用"), {"m1"}, {}, false);
    const ApiConfigEntity active = c.makeConfig(QStringLiteral("当前"), {"a1", "a2"}, {}, true);
    QVERIFY(c.dao.insert(inactive));
    QVERIFY(c.dao.insert(active));

    ApiConfigViewModel vm(&c.service);

    QCOMPARE(vm.currentConfigId(), active.id);
    QCOMPARE(vm.draft()->name(), QStringLiteral("当前"));
    QCOMPARE(vm.draft()->modelList(), QStringList({"a1", "a2"}));
    QVERIFY(vm.draft()->isActive());
}

// 界面默认显示列表第一条时，current_model 必须跟着落库（以前一直是 NULL）
void ApiConfigViewModelTest::persistsDefaultCurrentModel()
{
    Core c;
    const ApiConfigEntity config = c.makeConfig(QStringLiteral("当前"), {"m1", "m2"}, {}, true);
    QVERIFY(c.dao.insert(config));

    ApiConfigViewModel vm(&c.service);

    QCOMPARE(vm.draft()->currentModel(), QStringLiteral("m1"));
    QCOMPARE(c.dao.selectById(config.id).currentModel, QStringLiteral("m1"));
}

// 连续输入只在停手后落一次库
void ApiConfigViewModelTest::editsAreDebounced()
{
    Core c;
    const ApiConfigEntity config = c.makeConfig(QStringLiteral("当前"), {"m1"}, {}, true);
    QVERIFY(c.dao.insert(config));

    ApiConfigViewModel vm(&c.service);
    vm.draft()->setApiKey(QStringLiteral("k1"));
    vm.draft()->setApiKey(QStringLiteral("k2"));

    // 防抖窗口内不写库
    QCOMPARE(c.dao.selectById(config.id).apiKey, QString());
    QTRY_COMPARE_WITH_TIMEOUT(c.dao.selectById(config.id).apiKey, QStringLiteral("k2"), 3000);
    QVERIFY(!vm.draft()->isDirty());
}

// 改完立刻切换配置：旧草稿要先把改动写掉，不能丢
void ApiConfigViewModelTest::switchingConfigFlushesPendingEdits()
{
    Core c;
    const ApiConfigEntity first = c.makeConfig(QStringLiteral("A"), {"a"}, {}, true);
    const ApiConfigEntity second = c.makeConfig(QStringLiteral("B"), {"b"}, {}, false);
    QVERIFY(c.dao.insert(first));
    QVERIFY(c.dao.insert(second));

    ApiConfigViewModel vm(&c.service);
    vm.selectConfig(second.id);
    vm.draft()->setApiKey(QStringLiteral("b-key"));
    vm.selectConfig(first.id);

    QCOMPARE(c.dao.selectById(second.id).apiKey, QStringLiteral("b-key"));
    QCOMPARE(vm.currentConfigId(), first.id);
    QCOMPARE(vm.draft()->name(), QStringLiteral("A"));
}

void ApiConfigViewModelTest::applyCurrentConfigMarksActive()
{
    Core c;
    const ApiConfigEntity active = c.makeConfig(QStringLiteral("A"), {"a"}, {}, true);
    const ApiConfigEntity other = c.makeConfig(QStringLiteral("B"), {"b"}, {}, false);
    QVERIFY(c.dao.insert(active));
    QVERIFY(c.dao.insert(other));

    ApiConfigViewModel vm(&c.service);
    vm.selectConfig(other.id);
    QVERIFY(!vm.draft()->isActive());

    vm.applyCurrentConfig();

    QCOMPARE(c.dao.selectActive().id, other.id);
    QVERIFY(vm.draft()->isActive());
}

void ApiConfigViewModelTest::deletingCurrentConfigFallsBack()
{
    Core c;
    const ApiConfigEntity active = c.makeConfig(QStringLiteral("A"), {"a"}, {}, true);
    const ApiConfigEntity custom = c.makeConfig(QStringLiteral("自定义"), {"c"}, {}, false);
    QVERIFY(c.dao.insert(active));
    QVERIFY(c.dao.insert(custom));

    ApiConfigViewModel vm(&c.service);
    vm.selectConfig(custom.id);

    // 删除成功要走通用提示通道（NotificationBridge 直接转 toast）
    QSignalSpy tipSpy(&vm, &ApiConfigViewModel::operationTip);
    vm.deleteCurrentConfig();

    QCOMPARE(c.dao.selectById(custom.id).id, QString());
    QCOMPARE(vm.currentConfigId(), active.id);
    QCOMPARE(vm.configListModel()->rowCount(), 1);
    QCOMPARE(tipSpy.count(), 1);
    QCOMPARE(tipSpy.first().at(0).toString(), QStringLiteral("配置已删除"));
}

void ApiConfigViewModelTest::fetchWithoutConfigReportsError()
{
    Core c;
    ApiConfigViewModel vm(&c.service);

    QSignalSpy spy(&vm, &ApiConfigViewModel::operationError);
    vm.fetchModelList();

    QCOMPARE(spy.count(), 1);
    QVERIFY(!vm.isFetchingModels());
}

QTEST_GUILESS_MAIN(ApiConfigViewModelTest)
#include "ApiConfigViewModelTest.moc"
