#include "ApiConfigViewModel.h"
#include "ApiConfigService.h"
#include "ApiConfigListModel.h"
#include <QModelIndex>

ApiConfigViewModel::ApiConfigViewModel(QObject *parent)
    : QObject(parent)
    , m_service(&ApiConfigService::instance())
    , m_configListModel(new ApiConfigListModel(this))
{
    // 绑定 Service 层信号
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

// ==================== 属性 Getter ====================
ApiConfigListModel* ApiConfigViewModel::configListModel() const
{
    return m_configListModel;
}

QString ApiConfigViewModel::currentConfigId() const
{
    return m_currentConfigId;
}

bool ApiConfigViewModel::isCurrentBuiltin() const
{
    return m_currentSelectConfig.isBuiltin;
}

bool ApiConfigViewModel::currentIsActive() const
{
    return m_currentSelectConfig.isActive;
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

bool ApiConfigViewModel::testingConnection() const
{
    return m_testingConnection;
}

bool ApiConfigViewModel::fetchModelListState() const
{
    return m_fetchModelListState;
}

// void ApiConfigViewModel::setCurrentIndex(int index)
// {
    // if (m_currentIndex == index) return;//防止死循环
    // if (index < 0 || index >= m_configListModel->rowCount()) return;

    // m_currentIndex = index;
    // emit currentIndexChanged();

    // //保存当前id
    // QModelIndex idx = m_configListModel->index(index, 0);
    // m_currentConfigId = m_configListModel->data(idx, ApiConfigListModel::IdRole).toString();
    // emit currentConfigIdChanged();

    // 切换配置时加载详情表单
//     loadCurrentConfigDetail();
// }

void ApiConfigViewModel::selectConfig(const QString &configId)
{
    if (m_currentConfigId == configId) return;
    m_currentConfigId = configId;

    // 切换配置时加载详情表单
    loadCurrentConfigDetail();
}

void ApiConfigViewModel::setConfigName(const QString &name)
{
    if (m_currentSelectConfig.name == name) return;
    m_currentSelectConfig.name = name;
    saveCurrentConfig();
    emit configNameChanged();
}

void ApiConfigViewModel::setApiKey(const QString &key)
{
    if (m_currentSelectConfig.apiKey == key) return;
    m_currentSelectConfig.apiKey = key;
    saveCurrentConfig();
    emit apiKeyChanged();
}

void ApiConfigViewModel::setBaseDomain(const QString &domain)
{
    if (m_currentSelectConfig.baseDomain == domain) return;
    m_currentSelectConfig.baseDomain = domain;
    saveCurrentConfig();
    emit baseDomainChanged();
}

void ApiConfigViewModel::setApiPath(const QString &path)
{
    if (m_currentSelectConfig.apiPath == path) return;
    m_currentSelectConfig.apiPath = path;
    saveCurrentConfig();
    emit apiPathChanged();
}

void ApiConfigViewModel::setCurrentConfigId(const QString &id)
{
    //(可优化)
    if (m_currentConfigId == id) return;               // 守卫，防回环

    selectConfig(id);

    // m_currentConfigId = id;
    // emit currentConfigIdChanged();


    // emit currentPresetChanged();                       // 已有的业务信号
    // m_settings.setValue("ui/currentPresetId", id);     // 持久化
}


// ==================== 刷新配置列表 ====================
void ApiConfigViewModel::refreshList()
{
    QList<ApiConfigEntity> configs = m_service->getAllConfigs();
    m_configListModel->loadList(configs);

    setCurrentConfigId(m_service->getActiveConfig().id);

    //刷新表单
    loadCurrentConfigDetail();

    // if (m_currentIndex < 0 || m_currentIndex >= m_configListModel->rowCount())
    //     setCurrentIndex(0);
    // else setCurrentIndex(m_currentIndex);
//以下废弃
//     int index=-1;
//     for (int i = 0; i < configs.size(); ++i) {
//         if (configs[i].isActive) {
//             index = i;
//             break;
//         }
//     }
// // 默认选中当前选中的配置,然后是激活的配置,无激活则选第一项
//     //设置选中项
//     if (m_currentIndex >= 0) {
//         setCurrentIndex(m_currentIndex);
//     } else if(index>=0){
//         setCurrentIndex(index);
//     }else {
//         setCurrentIndex(0);
//     }
//     emit currentIsActiveChanged();
}

void ApiConfigViewModel::addCustomConfig(const QString &configName)
{
    QString trimmedName = configName.trimmed();
    if (trimmedName.isEmpty()) {
        emit operationError(QStringLiteral("配置名称不能为空"));
        return;
    }

    // 创建空白自定义配置，默认填充 OpenAI 兼容格式路径
    QString newId = m_service->createCustomConfig(trimmedName);
    if (newId.isEmpty()) {
        emit operationError(QStringLiteral("创建自定义配置失败"));
        return;
    }


    // 刷新列表并自动选中新建的配置
    refreshList();

    setCurrentConfigId(newId);
    loadCurrentConfigDetail();

    // for (int i = 0; i < m_configListModel->rowCount(); ++i) {
    //     QModelIndex idx = m_configListModel->index(i, 0);
    //     if (m_configListModel->data(idx, ApiConfigListModel::IdRole).toString() == newId) {
    //         setCurrentIndex(i);
    //         break;
    //     }
    // }
}

void ApiConfigViewModel::saveCurrentConfig()
{
    if (m_currentConfigId.isEmpty()) return;

    bool success = m_service->updateConfig(m_currentSelectConfig);
    if (success) {
        // // 同步更新列表项显示(暂时无用)
        // m_configListModel->updateItem(m_currentSelectConfig);
    }
}

void ApiConfigViewModel::deleteCurrentConfig()
{
    if (m_currentConfigId.isEmpty()) return;
    if (m_currentSelectConfig.isBuiltin) {
        emit operationError(QStringLiteral("内置配置不可删除"));
        return;
    }

    bool success = m_service->deleteConfig(m_currentConfigId);
    if (success) {
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
    m_currentSelectConfig = m_service->getConfigById(m_currentConfigId);

    // 批量触发表单属性变更
    m_fetchModelListState = false;
    emit fetchModelListStateChanged();

    m_currentModelsIndex = m_currentSelectConfig.availableModels.isEmpty()?-1:getModelsIndex();
    emit currentModelsIndexChanged();


    emit modelListChanged();
    emit currentIsActiveChanged();
    emit configNameChanged();
    emit apiKeyChanged();
    emit baseDomainChanged();
    emit apiPathChanged();
    emit currentConfigChanged();




    // if (m_currentConfigId.isEmpty()) {
    //     m_currentSelectConfig = ApiConfigEntity();
    //      // 空配置时重置模型索引
    //      m_currentModelsIndex = -1;
    //      emit currentModelsIndexChanged();
    //      return;
    // }
    // setCurrentModelsIndex(m_currentSelectConfig.currentModel);
}

int ApiConfigViewModel::getModelsIndex()
{
    //遍历模型列表
    const QStringList& list = m_currentSelectConfig.availableModels;
    if (list.isEmpty()) return -1;
    int idx = list.indexOf(m_currentSelectConfig.currentModel);
    return idx < 0 ? 0 : idx;

    // QStringList list=m_currentSelectConfig.availableModels;
    // QString modelName=m_currentSelectConfig.currentModel;
    // for(int i = 0; i < list.size(); ++i)
    // {
    //     if(list[i]==modelName){
    //         return i;
    //     }
    // }
    // return 0;
}


int ApiConfigViewModel::currentModelsIndex() const
{
    return m_currentModelsIndex;
}

void ApiConfigViewModel::setCurrentModelsIndex(int newIndex)
{
    //设置模型列表索引
    if (m_currentModelsIndex == newIndex) return;//防止死循环和越界
    // 越界保护：空列表、-1、超长都不去碰 availableModels
    if (newIndex < 0 || newIndex >= m_currentSelectConfig.availableModels.size()) {
        m_currentModelsIndex = newIndex;   // 或直接 return，看业务
        emit currentModelsIndexChanged();
        return;
    }

    //保存
    m_currentSelectConfig.currentModel=m_currentSelectConfig.availableModels[newIndex];
    ApiConfigService::instance().updateConfig(m_currentSelectConfig);

    m_currentModelsIndex = newIndex;
    emit currentModelsIndexChanged();



    // int listSize = m_currentSelectConfig.availableModels.size();
    // if (newCurrentModelsIndex < 0 || newCurrentModelsIndex >= listSize) return;
    // {
    //     m_currentModelsIndex = -1;
    //     emit currentModelsIndexChanged();
    //     return;
    // }
    // m_currentModelsIndex = newCurrentModelsIndex;
    // m_currentSelectConfig.currentModel=m_currentSelectConfig.availableModels[newCurrentModelsIndex];
    // emit currentModelsIndexChanged();
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

    // m_testingConnection = false;
    // emit testingConnectionChanged();
    emit operationError(error);
}

void ApiConfigViewModel::onFetchModelListState(const QString& configId)
{
    m_fetchModelListState = false;
    emit fetchModelListStateChanged();
}