#include "ApiConfigFormViewModel.h"
#include "ApiConfigService.h"

ApiConfigFormViewModel* ApiConfigFormViewModel::instance()
{
    static ApiConfigFormViewModel inst;
    return &inst;
}

ApiConfigFormViewModel::ApiConfigFormViewModel(QObject* parent)
    : QObject(parent)
{
}

ApiConfigFormViewModel::~ApiConfigFormViewModel() = default;

// ==================== 属性读取 ====================
bool ApiConfigFormViewModel::isBuiltin() const
{
    return m_currentSelectConfig.isBuiltin;
}

bool ApiConfigFormViewModel::isActive() const
{
    return m_currentSelectConfig.isActive;
}

bool ApiConfigFormViewModel::showDomainInput() const
{
    // 内置配置隐藏域名/路径，自定义配置显示
    return !m_currentSelectConfig.isBuiltin;
}

QString ApiConfigFormViewModel::configName() const
{
    return m_currentSelectConfig.name;
}

QString ApiConfigFormViewModel::apiKey() const
{
    return m_currentSelectConfig.apiKey;
}

QString ApiConfigFormViewModel::baseDomain() const
{
    return m_currentSelectConfig.baseDomain;
}

QString ApiConfigFormViewModel::apiPath() const
{
    return m_currentSelectConfig.apiPath;
}

QStringList ApiConfigFormViewModel::modelList() const
{
    return m_currentSelectConfig.availableModels;
}

int ApiConfigFormViewModel::currentModelsIndex() const
{
    return m_currentModelsIndex;
}

// ==================== 载入与保存 ====================
void ApiConfigFormViewModel::loadConfig(const ApiConfigEntity& config)
{
    m_currentSelectConfig = config;

    m_currentModelsIndex = config.availableModels.isEmpty() ? -1 : getModelsIndex();
    emit currentModelsIndexChanged();

    emit modelListChanged();
    emit configNameChanged();
    emit apiKeyChanged();
    emit baseDomainChanged();
    emit apiPathChanged();
    emit currentConfigChanged();
}

int ApiConfigFormViewModel::getModelsIndex()
{
    const QStringList& list = m_currentSelectConfig.availableModels;
    if (list.isEmpty()) return -1;

    int idx = list.indexOf(m_currentSelectConfig.currentModel);
    return idx < 0 ? 0 : idx;
}

void ApiConfigFormViewModel::saveCurrentConfig()
{
    if (m_currentSelectConfig.id.isEmpty()) return;
    ApiConfigService::instance().updateConfig(m_currentSelectConfig);
}

// ==================== 字段写入 ====================
void ApiConfigFormViewModel::setConfigName(const QString& name)
{
    if (m_currentSelectConfig.name == name) return;
    m_currentSelectConfig.name = name;
    saveCurrentConfig();
    emit configNameChanged();
}

void ApiConfigFormViewModel::setApiKey(const QString& key)
{
    if (m_currentSelectConfig.apiKey == key) return;
    m_currentSelectConfig.apiKey = key;
    saveCurrentConfig();
    emit apiKeyChanged();
}

void ApiConfigFormViewModel::setBaseDomain(const QString& domain)
{
    if (m_currentSelectConfig.baseDomain == domain) return;
    m_currentSelectConfig.baseDomain = domain;
    saveCurrentConfig();
    emit baseDomainChanged();
}

void ApiConfigFormViewModel::setApiPath(const QString& path)
{
    if (m_currentSelectConfig.apiPath == path) return;
    m_currentSelectConfig.apiPath = path;
    saveCurrentConfig();
    emit apiPathChanged();
}

void ApiConfigFormViewModel::setCurrentModelsIndex(int newIndex)
{
    if (m_currentModelsIndex == newIndex) return;

    // 越界保护：空列表、-1、超长都不去碰 availableModels
    if (newIndex < 0 || newIndex >= m_currentSelectConfig.availableModels.size()) {
        m_currentModelsIndex = newIndex;
        emit currentModelsIndexChanged();
        return;
    }

    m_currentSelectConfig.currentModel = m_currentSelectConfig.availableModels[newIndex];
    ApiConfigService::instance().updateConfig(m_currentSelectConfig);

    m_currentModelsIndex = newIndex;
    emit currentModelsIndexChanged();
}
