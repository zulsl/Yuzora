#include "ApiConfigViewModel.h"
#include "ApiConfigFormViewModel.h"
#include "ApiConfigService.h"
#include "ApiConfigListModel.h"

ApiConfigViewModel::ApiConfigViewModel(QObject *parent)
    : QObject(parent)
    , m_service(&ApiConfigService::instance())
    , m_configListModel(new ApiConfigListModel(this))
{
    // Service 层信号
    connect(m_service, &ApiConfigService::operationError,
            this, &ApiConfigViewModel::operationError);
    connect(m_service, &ApiConfigService::operationTip,
            this, &ApiConfigViewModel::operationTip);
    connect(m_service, &ApiConfigService::testSuccess,
            this, &ApiConfigViewModel::onTestSuccess);
    connect(m_service, &ApiConfigService::fetchModelListEnd,
            this, &ApiConfigViewModel::onFetchModelListState);

    // 初始化加载配置列表
    refreshList();
}

ApiConfigViewModel::~ApiConfigViewModel() = default;

// ==================== 属性读取 ====================
ApiConfigListModel* ApiConfigViewModel::configListModel() const
{
    return m_configListModel;
}

QString ApiConfigViewModel::currentConfigId() const
{
    return m_currentConfigId;
}

bool ApiConfigViewModel::currentIsActive() const
{
    return ApiConfigFormViewModel::instance()->isActive();
}

bool ApiConfigViewModel::testingConnection() const
{
    return m_testingConnection;
}

bool ApiConfigViewModel::fetchModelListState() const
{
    return m_fetchModelListState;
}

// ==================== 选中 ====================
void ApiConfigViewModel::selectConfig(const QString &configId)
{
    if (m_currentConfigId == configId) return;
    m_currentConfigId = configId;

    // 【方案 C5｜已有信号的触发时机变更】
    // 原先 currentConfigIdChanged 在整个类里从未有效发出（两处 emit 都是注释），
    // 导致绑定 ApiConfigViewModel.currentConfigId 的 QML 不会刷新。
    emit currentConfigIdChanged();

    // 切换配置时加载详情表单
    loadCurrentConfigDetail();
}

void ApiConfigViewModel::setCurrentConfigId(const QString &id)
{
    if (m_currentConfigId == id) return;
    selectConfig(id);
}

// ==================== 刷新配置列表 ====================
void ApiConfigViewModel::refreshList()
{
    QList<ApiConfigEntity> configs = m_service->getAllConfigs();
    m_configListModel->loadList(configs);

    setCurrentConfigId(m_service->getActiveConfig().id);

    // 刷新表单
    loadCurrentConfigDetail();
}

void ApiConfigViewModel::addCustomConfig(const QString &configName)
{
    QString trimmedName = configName.trimmed();
    if (trimmedName.isEmpty()) {
        emit operationError(QStringLiteral("配置名称不能为空"));
        return;
    }

    QString newId = m_service->createCustomConfig(trimmedName);
    if (newId.isEmpty()) {
        emit operationError(QStringLiteral("创建自定义配置失败"));
        return;
    }

    // 刷新列表并自动选中新建的配置
    refreshList();
    setCurrentConfigId(newId);
    loadCurrentConfigDetail();
}

void ApiConfigViewModel::deleteCurrentConfig()
{
    if (m_currentConfigId.isEmpty()) return;
    if (ApiConfigFormViewModel::instance()->isBuiltin()) {
        emit operationError(QStringLiteral("内置配置不可删除"));
        return;
    }

    if (m_service->deleteConfig(m_currentConfigId)) {
        emit deleteSuccess();
        // 刷新列表会自动重新选中有效项
        refreshList();
    } else {
        emit operationError(QStringLiteral("删除配置失败"));
    }
}

void ApiConfigViewModel::testCurrentConnection()
{
    if (m_currentConfigId.isEmpty() || m_testingConnection) return;

    m_testingConnection = true;
    emit testingConnectionChanged();

    m_service->testConnection(m_currentConfigId);
}

void ApiConfigViewModel::fetchModelList()
{
    if (m_fetchModelListState) return;

    m_fetchModelListState = true;
    emit fetchModelListStateChanged();

    m_service->fetchModelList(m_currentConfigId);
}

void ApiConfigViewModel::applyCurrentConfig()
{
    if (m_currentConfigId.isEmpty()) return;

    m_service->setActiveConfig(m_currentConfigId);
    refreshList();
}

// ==================== 加载详情表单 ====================
void ApiConfigViewModel::loadCurrentConfigDetail()
{
    // 表单域自己持有实体；这里只负责把选中的配置交给它
    ApiConfigFormViewModel::instance()->loadConfig(m_service->getConfigById(m_currentConfigId));

    m_fetchModelListState = false;
    emit fetchModelListStateChanged();

    emit currentIsActiveChanged();
}

// ==================== Service 回调槽 ====================
void ApiConfigViewModel::onTestSuccess(const QString &configId)
{
    // 只响应当前选中配置的测试结果
    if (configId != m_currentConfigId) return;

    m_testingConnection = false;
    emit testingConnectionChanged();
    emit testSuccess();
}

void ApiConfigViewModel::onRequestFailed(const QString &configId, const QString &error)
{
    if (configId != m_currentConfigId) return;

    emit operationError(error);
}

void ApiConfigViewModel::onFetchModelListState(const QString &configId)
{
    Q_UNUSED(configId)
    m_fetchModelListState = false;
    emit fetchModelListStateChanged();
}
