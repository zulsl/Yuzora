#include "ApiConfigViewModel.h"

namespace {
// 输入停手多久才落库
constexpr int SAVE_DEBOUNCE_MS = 300;
}

ApiConfigViewModel::ApiConfigViewModel(QObject* parent)
    : ApiConfigViewModel(&ApiConfigService::instance(), parent)
{
}

ApiConfigViewModel::ApiConfigViewModel(ApiConfigService* service, QObject* parent)
    : QObject(parent)
    , m_service(service ? service : &ApiConfigService::instance())
    , m_configListModel(new ApiConfigListModel(this))
    , m_draft(new ApiConfigDraft(this))
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(SAVE_DEBOUNCE_MS);
    connect(&m_saveTimer, &QTimer::timeout, this, &ApiConfigViewModel::flushPendingSave);

    // 草稿任何字段变化都只是"排一次保存"，真正写库在 timer 里
    connect(m_draft, &ApiConfigDraft::changed,
            this, &ApiConfigViewModel::scheduleSave);

    connect(m_service, &ApiConfigService::operationError,
            this, &ApiConfigViewModel::operationError);
    connect(m_service, &ApiConfigService::operationTip,
            this, &ApiConfigViewModel::operationTip);
    connect(m_service, &ApiConfigService::fetchModelListEnd,
            this, &ApiConfigViewModel::onFetchModelListState);
    connect(m_service, &ApiConfigService::modelListUpdated,
            this, &ApiConfigViewModel::onModelListUpdated);

    refreshList();
}

ApiConfigViewModel::~ApiConfigViewModel() = default;

// ==================== 属性 ====================
ApiConfigListModel* ApiConfigViewModel::configListModel() const
{
    return m_configListModel;
}

QString ApiConfigViewModel::currentConfigId() const
{
    return m_currentConfigId;
}

ApiConfigDraft* ApiConfigViewModel::draft() const
{
    return m_draft;
}

bool ApiConfigViewModel::isFetchingModels() const
{
    return m_fetching;
}

// ==================== 列表/选中 ====================
void ApiConfigViewModel::refreshList()
{
    const QList<ApiConfigEntity> configs = m_service->getAllConfigs();
    m_configListModel->loadList(configs);

    // 选谁：当前这条还在就用它，否则用激活配置，再不行取第一条
    QString target;
    if (!m_currentConfigId.isEmpty() && !m_service->getConfigById(m_currentConfigId).id.isEmpty())
        target = m_currentConfigId;
    else
        target = m_service->getActiveConfig().id;
    if (target.isEmpty() && !configs.isEmpty())
        target = configs.first().id;

    if (target.isEmpty()) {
        if (!m_currentConfigId.isEmpty()) {
            m_currentConfigId.clear();
            m_draft->setEntity(ApiConfigEntity{});
            emit currentConfigIdChanged();
        }
        return;
    }

    if (target == m_currentConfigId)
        loadDraft(target);      // 同一份：重新读库刷新草稿
    else
        selectConfig(target);   // 换一份：内部会先把旧草稿落库
}

void ApiConfigViewModel::selectConfig(const QString& configId)
{
    if (configId.isEmpty() || configId == m_currentConfigId) return;

    // 切走之前先把上一份没落库的编辑写掉，避免"改完配置 A 立刻切走"丢改动
    flushPendingSave();

    m_currentConfigId = configId;
    loadDraft(configId);
    emit currentConfigIdChanged();
}

void ApiConfigViewModel::loadDraft(const QString& configId)
{
    m_draft->setEntity(m_service->getConfigById(configId));
    ensureCurrentModelSelected();
}

// ==================== 草稿落库 ====================
void ApiConfigViewModel::ensureCurrentModelSelected()
{
    const QStringList models = m_draft->modelList();
    if (models.isEmpty()) return;

    // 已有选中值且还在列表里：不用动
    if (models.contains(m_draft->currentModel())) return;

    // 空值 / 已经不在新列表里：落到第一条并立即落库。
    // 以前这条路径不写库：界面显示第一条，数据库里 current_model 却一直是 NULL。
    m_draft->setCurrentModel(models.first());
    flushPendingSave();
}

void ApiConfigViewModel::scheduleSave()
{
    if (m_draft->isEmpty()) return;
    m_saveTimer.start();
}

void ApiConfigViewModel::flushPendingSave()
{
    m_saveTimer.stop();

    if (m_draft->isEmpty() || !m_draft->isDirty()) return;

    // 失败时保持 dirty，下次改动或切换配置时会再试一次
    if (!m_service->updateConfig(m_draft->entity())) return;

    m_draft->markSaved();
    m_configListModel->updateItem(m_draft->entity());
}

void ApiConfigViewModel::cancelPendingSave()
{
    m_saveTimer.stop();
}

// ==================== 操作 ====================
bool ApiConfigViewModel::addCustomConfig(const QString& configName)
{
    const QString trimmed = configName.trimmed();
    if (trimmed.isEmpty()) {
        emit operationError(QStringLiteral("配置名称不能为空"));
        return false;
    }

    flushPendingSave();

    const QString newId = m_service->createCustomConfig(trimmed);
    if (newId.isEmpty()) return false;   // 失败提示由 Service 发

    refreshList();
    selectConfig(newId);
    return true;
}

void ApiConfigViewModel::deleteCurrentConfig()
{
    if (m_currentConfigId.isEmpty()) return;

    if (m_draft->isBuiltin()) {
        emit operationError(QStringLiteral("内置配置不可删除"));
        return;
    }

    // 要删的配置不再保存：先停掉防抖 timer，避免删完又被写回一条
    cancelPendingSave();

    if (!m_service->deleteConfig(m_currentConfigId)) return;

    m_currentConfigId.clear();
    m_draft->setEntity(ApiConfigEntity{});
    refreshList();

    // 统一走 operationTip：NotificationBridge 只要转发两个通用信号，
    // 不用再为"删除成功"单独挂一个 onDeleteSuccess 处理器
    emit operationTip(QStringLiteral("配置已删除"));
}

void ApiConfigViewModel::applyCurrentConfig()
{
    if (m_currentConfigId.isEmpty()) return;

    flushPendingSave();
    if (!m_service->setActiveConfig(m_currentConfigId)) return;

    // 激活状态存在库里：重新读一遍草稿 + 列表（小圆点要跟着变）
    loadDraft(m_currentConfigId);
    m_configListModel->loadList(m_service->getAllConfigs());
}

void ApiConfigViewModel::fetchModelList()
{
    if (m_fetching) return;

    if (m_currentConfigId.isEmpty()) {
        emit operationError(QStringLiteral("请先选择配置"));
        return;
    }

    // 先把没落库的域名 / Key 写进库，否则 Service 拿的是旧配置
    flushPendingSave();

    m_fetching = true;
    emit isFetchingModelsChanged();

    m_service->fetchModelList(m_currentConfigId);
}

// ==================== Service 回调 ====================
void ApiConfigViewModel::onFetchModelListState(const QString& configId)
{
    // 只处理当前配置的结束信号，避免旧请求把新请求的转圈状态关掉
    if (!configId.isEmpty() && configId != m_currentConfigId) return;
    if (!m_fetching) return;

    m_fetching = false;
    emit isFetchingModelsChanged();
}

void ApiConfigViewModel::onModelListUpdated(const QString& configId)
{
    // 只刷新当前选中的配置：旧请求的结果不该改动现在这份表单
    if (configId.isEmpty() || configId != m_currentConfigId) return;

    // Service 已经把新模型写进库了，这里重新读一遍草稿（顺带兜底 currentModel）
    loadDraft(configId);
}
