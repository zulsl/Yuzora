#include "ApiConfigViewModel.h"

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
    connect(m_service, &ApiConfigService::fetchModelListEnd,
            this, &ApiConfigViewModel::onFetchModelListState);

    // 初始化加载配置列表
    refreshList();
}

ApiConfigViewModel::~ApiConfigViewModel() = default;

// ==================== 列表/选中 ====================
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
    return m_currentSelectConfig.isActive;
}

bool ApiConfigViewModel::fetchModelListState() const
{
    return m_fetchModelListState;
}

// ==================== 表单字段 ====================
bool ApiConfigViewModel::isBuiltin() const
{
    return m_currentSelectConfig.isBuiltin;
}

bool ApiConfigViewModel::showDomainInput() const
{
    // 内置配置隐藏域名/路径，自定义配置显示
    return !m_currentSelectConfig.isBuiltin;
}

QString ApiConfigViewModel::configName() const
{
    return m_currentSelectConfig.name;
}

QString ApiConfigViewModel::apiKey() const
{
    return m_currentSelectConfig.apiKey;
}

QString ApiConfigViewModel::baseDomain() const
{
    return m_currentSelectConfig.baseDomain;
}

QString ApiConfigViewModel::apiPath() const
{
    return m_currentSelectConfig.apiPath;
}

QStringList ApiConfigViewModel::modelList() const
{
    return m_currentSelectConfig.availableModels;
}

int ApiConfigViewModel::currentModelsIndex() const
{
    return m_currentModelsIndex;
}

// ==================== 选中 ====================
void ApiConfigViewModel::selectConfig(const QString &configId)
{
    if (m_currentConfigId == configId) return;
    m_currentConfigId = configId;

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

bool ApiConfigViewModel::addCustomConfig(const QString &configName)
{
    QString trimmedName = configName.trimmed();
    if (trimmedName.isEmpty()) {
        emit operationError(QStringLiteral("配置名称不能为空"));
        return false;
    }

    QString newId = m_service->createCustomConfig(trimmedName);
    if (newId.isEmpty()) {
        emit operationError(QStringLiteral("创建自定义配置失败"));
        return false;
    }

    // 刷新列表并自动选中新建的配置
    refreshList();
    setCurrentConfigId(newId);
    loadCurrentConfigDetail();
    return true;
}

void ApiConfigViewModel::deleteCurrentConfig()
{
    if (m_currentConfigId.isEmpty()) return;
    if (m_currentSelectConfig.isBuiltin) {
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

void ApiConfigViewModel::fetchModelList()
{
    if (m_fetchModelListState) return;

    if (m_currentConfigId.isEmpty()) {
        emit operationError(QStringLiteral("请先选择配置"));
        return;
    }

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

// ==================== 表单编辑 ====================
void ApiConfigViewModel::setConfigName(const QString& name)
{
    if (m_currentSelectConfig.name == name) return;
    m_currentSelectConfig.name = name;
    saveCurrentConfig();
    emit configNameChanged();
}

void ApiConfigViewModel::setApiKey(const QString& key)
{
    if (m_currentSelectConfig.apiKey == key) return;
    m_currentSelectConfig.apiKey = key;
    saveCurrentConfig();
    emit apiKeyChanged();
}

void ApiConfigViewModel::setBaseDomain(const QString& domain)
{
    if (m_currentSelectConfig.baseDomain == domain) return;
    m_currentSelectConfig.baseDomain = domain;
    saveCurrentConfig();
    emit baseDomainChanged();
}

void ApiConfigViewModel::setApiPath(const QString& path)
{
    if (m_currentSelectConfig.apiPath == path) return;
    m_currentSelectConfig.apiPath = path;
    saveCurrentConfig();
    emit apiPathChanged();
}

void ApiConfigViewModel::setCurrentModelsIndex(int newIndex)
{
    if (m_currentModelsIndex == newIndex) return;

    // 越界保护：空列表、-1、超长都不去碰 availableModels
    if (newIndex < 0 || newIndex >= m_currentSelectConfig.availableModels.size()) {
        m_currentModelsIndex = newIndex;
        emit currentModelsIndexChanged();
        return;
    }

    m_currentSelectConfig.currentModel = m_currentSelectConfig.availableModels[newIndex];
    saveCurrentConfig();

    m_currentModelsIndex = newIndex;
    emit currentModelsIndexChanged();
}

// ==================== 加载/保存 ====================
void ApiConfigViewModel::loadCurrentConfigDetail()
{
    m_currentSelectConfig = m_service->getConfigById(m_currentConfigId);
    m_currentModelsIndex = m_currentSelectConfig.availableModels.isEmpty()
                         ? -1 : getModelsIndex();

    emit currentModelsIndexChanged();
    emit modelListChanged();
    emit configNameChanged();
    emit apiKeyChanged();
    emit baseDomainChanged();
    emit apiPathChanged();
    emit currentConfigChanged();
    emit currentIsActiveChanged();

    m_fetchModelListState = false;
    emit fetchModelListStateChanged();
}

void ApiConfigViewModel::saveCurrentConfig()
{
    if (m_currentSelectConfig.id.isEmpty()) return;
    if (!m_service->updateConfig(m_currentSelectConfig)) return;

    // 合并后列表和表单共享同一份实体，改名等字段变化可立即反映到列表
    m_configListModel->updateItem(m_currentSelectConfig);
}

int ApiConfigViewModel::getModelsIndex()
{
    const QStringList& list = m_currentSelectConfig.availableModels;
    if (list.isEmpty()) return -1;

    int idx = list.indexOf(m_currentSelectConfig.currentModel);
    return idx < 0 ? 0 : idx;
}

// ==================== Service 回调槽 ====================
void ApiConfigViewModel::onFetchModelListState(const QString &configId)
{
    // 只处理当前配置的结束信号，避免旧请求把新请求的转圈状态关掉
    if (!configId.isEmpty() && configId != m_currentConfigId) return;

    m_fetchModelListState = false;
    emit fetchModelListStateChanged();
}
