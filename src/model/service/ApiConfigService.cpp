#include "ApiConfigService.h"
#include "OpenAICompatibleApiClient.h"
#include "BaseApiClient.h"
#include "GlobalConstants.h"
#include "ModelFactory.h"
#include <QUuid>
#include <QDateTime>

ApiConfigService& ApiConfigService::instance()
{
    static ApiConfigService instance;
    return instance;
}

ApiConfigService::ApiConfigService(QObject *parent)
    : QObject(parent)
    , m_dao(new ApiConfigDao(this))
{
}

ApiConfigService::ApiConfigService(ApiConfigDao* dao, QObject* parent)
    : QObject(parent)
    , m_dao(dao ? dao : new ApiConfigDao(this))
{
}

ApiConfigService::~ApiConfigService() = default;

void ApiConfigService::ensureSeedData()
{
    initBuiltinConfigs();
}

// ==================== 列表与查询 ====================
QList<ApiConfigEntity> ApiConfigService::getAllConfigs()
{
    return m_dao->selectAll();
}

ApiConfigEntity ApiConfigService::getConfigById(const QString &id)
{
    return m_dao->selectById(id);
}

ApiConfigEntity ApiConfigService::getActiveConfig() const
{
    return m_dao->selectActive();
}

// ==================== 配置管理 ====================
QString ApiConfigService::createCustomConfig(const QString &name)
{
    ApiConfigEntity config;
    config.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    config.name = name.trimmed().isEmpty() ? QStringLiteral("自定义API") : name.trimmed();
    config.modelType = GlobalConstants::ModelType::OpenAI;
    config.isBuiltin = false;
    config.isOpenAICompatible = true;
    config.baseDomain = QString();
    config.apiPath = QStringLiteral("/v1/chat/completions");
    config.apiKey = QString();
    config.isActive = false;
    config.createdAt = QDateTime::currentDateTime();
    config.updatedAt = QDateTime::currentDateTime();

    if (!m_dao->insert(config)) {
        emit operationError(QStringLiteral("创建配置失败，数据库写入错误"));
        return {};
    }
    return config.id;
}

bool ApiConfigService::updateConfig(const ApiConfigEntity &config)
{
    ApiConfigEntity updated = config;
    updated.updatedAt = QDateTime::currentDateTime();

    bool success = m_dao->update(updated);
    if (success) {
        emit configUpdated(config.id);
    } else {
        emit operationError(QStringLiteral("更新配置失败"));
    }
    return success;
}

bool ApiConfigService::deleteConfig(const QString &id)
{
    if (isBuiltinConfig(id)) {
        emit operationError(QStringLiteral("内置预设配置不可删除"));
        return false;
    }

    bool success = m_dao->remove(id);
    if (success) {
        emit configDeleted(id);
    }
    return success;
}

bool ApiConfigService::setActiveConfig(const QString &id)
{
    // 先清除所有激活状态，再设置当前项
    if (!m_dao->clearAllActive()) {
        emit operationError(QStringLiteral("设置激活配置失败"));
        return false;
    }

    bool success = m_dao->setActive(id);
    if (success) {
    }else{
        emit operationError(QStringLiteral("设置当前使用配置失败"));
    }
    return success;
}

// ==================== 模型列表管理 ====================
bool ApiConfigService::updateModelList(const QString &configId, const QStringList& modelNames)
{
    ApiConfigEntity config = getConfigById(configId);
    if (config.id.isEmpty()) return false;

    config.availableModels=modelNames;
    return updateConfig(config);
}

// ==================== 模型列表 ====================
void ApiConfigService::fetchModelList(const QString &configId)
{
    ApiConfigEntity config = getConfigById(configId);
    if (config.id.isEmpty()) {
        emit operationError(QStringLiteral("配置不存在"));
        emit fetchModelListEnd(configId);
        return;
    }
    if (config.baseDomain.isEmpty()) {
        emit operationError(QStringLiteral("请先填写API域名"));
        emit fetchModelListEnd(configId);
        return;
    }

    // 根据id判断客户端类型
    // 创建临时客户端，完成后自动销毁
    BaseApiClient* tempClient = ModelFactory::instance().createClient(config.modelType);
    if (!tempClient) {
        emit operationError(QStringLiteral("该模型类型暂不支持获取模型列表"));
        emit fetchModelListEnd(configId);
        return;
    }
    tempClient->loadConfig(config);

    connect(tempClient, &BaseApiClient::modelListFetched,
            this, [this, configId, tempClient](const QStringList& modelNames) {
                if(updateModelList(configId,modelNames)){
                    emit operationTip("获取成功");
                    emit modelListUpdated(configId);
                }else{
                    emit operationError("数据库写入失败");
                }
                tempClient->deleteLater();
                emit fetchModelListEnd(configId);
            });

    connect(tempClient, &BaseApiClient::requestFailed,
            this, [this, configId, tempClient](const QString& error) {
                emit operationError(error);
                tempClient->deleteLater();
                emit fetchModelListEnd(configId);
            });

    tempClient->fetchModelList();
}

// ==================== 私有方法 ====================
void ApiConfigService::initBuiltinConfigs()
{
    // 已有配置则跳过初始化，避免重复插入
    if (!m_dao->selectAll().isEmpty()) {
        return;
    }


//预设模型

    //OpenAI 官方
    ApiConfigEntity openai;
    openai.id = Utils::generateUuid();
    openai.name = QStringLiteral("OpenAI 官方");
    openai.modelType = GlobalConstants::ModelType::OpenAI;
    openai.isBuiltin = true;
    openai.isOpenAICompatible = true;
    openai.baseDomain = QStringLiteral("https://api.openai.com/v1");
    openai.apiPath = QStringLiteral("/chat/completions");
    openai.availableModels = { "gpt-4o", "gpt-4-turbo", "gpt-3.5-turbo", "gpt-4o-mini" };
    openai.isActive = true;
    openai.createdAt = QDateTime::currentDateTime();
    openai.updatedAt = QDateTime::currentDateTime();
    m_dao->insert(openai);

    //DeepSeek
    ApiConfigEntity deepseek;
    deepseek.id = Utils::generateUuid();
    deepseek.name = QStringLiteral("DeepSeek");
    deepseek.modelType = GlobalConstants::ModelType::OpenAI;
    deepseek.isBuiltin = true;
    deepseek.isOpenAICompatible = true;
    deepseek.baseDomain = QStringLiteral("https://api.deepseek.com/v1");
    deepseek.apiPath = QStringLiteral("/chat/completions");
    deepseek.availableModels = { "deepseek-chat", "deepseek-reasoner" };
    deepseek.isActive = false;
    deepseek.createdAt = QDateTime::currentDateTime();
    deepseek.updatedAt = QDateTime::currentDateTime();
    m_dao->insert(deepseek);

    //智谱GLM
    ApiConfigEntity glm;
    glm.id = Utils::generateUuid();
    glm.name = QStringLiteral("GLM");
    glm.modelType = GlobalConstants::ModelType::OpenAI;
    glm.isBuiltin = true;
    glm.isOpenAICompatible = true;
    glm.baseDomain = QStringLiteral("https://open.bigmodel.cn/api/paas/v4");
    glm.apiPath = QStringLiteral("/chat/completions");
    glm.availableModels = { "glm-5.3"};
    glm.isActive = false;
    glm.createdAt = QDateTime::currentDateTime();
    glm.updatedAt = QDateTime::currentDateTime();
    m_dao->insert(glm);

    //豆包
    ApiConfigEntity Seed;
    Seed.id = Utils::generateUuid();
    Seed.name = QStringLiteral("豆包");
    Seed.modelType = GlobalConstants::ModelType::OpenAI;
    Seed.isBuiltin = true;
    Seed.isOpenAICompatible = true;
    Seed.baseDomain = QStringLiteral("https://ark.cn-beijing.volces.com/api/v3");
    Seed.apiPath = QStringLiteral("/chat/completions");
    Seed.availableModels = {
        "doubao-seed-2-1-pro-260915","doubao-seed-2-1-lite-260915",
        "doubao-seed-2-1-turbo-260628"};
    Seed.isActive = false;
    Seed.createdAt = QDateTime::currentDateTime();
    Seed.updatedAt = QDateTime::currentDateTime();
    m_dao->insert(Seed);

    //Kimi
    ApiConfigEntity kimi;
    kimi.id = Utils::generateUuid();
    kimi.name = QStringLiteral("Kimi");
    kimi.modelType = GlobalConstants::ModelType::OpenAI;
    kimi.isBuiltin = true;
    kimi.isOpenAICompatible = true;
    kimi.baseDomain = QStringLiteral("https://api.moonshot.cn/v1");
    kimi.apiPath = QStringLiteral("/chat/completions");
    kimi.availableModels = {
        "kimi-k3.1-pro","kimi-k3.1-pro-highspeed","kimi-k3"};
    kimi.isActive = false;
    kimi.createdAt = QDateTime::currentDateTime();
    kimi.updatedAt = QDateTime::currentDateTime();
    m_dao->insert(kimi);

    //千问
    ApiConfigEntity Qwen;
    Qwen.id = Utils::generateUuid();
    Qwen.name = QStringLiteral("千问");
    Qwen.modelType = GlobalConstants::ModelType::OpenAI;
    Qwen.isBuiltin = true;
    Qwen.isOpenAICompatible = true;
    Qwen.baseDomain = QStringLiteral("https://maas.qianwenaiapi.com/compatible-mode/v1");
    Qwen.apiPath = QStringLiteral("/chat/completions");
    Qwen.availableModels = {
        "qwen3.8-flash","qwen3.7-plus"};
    Qwen.isActive = false;
    Qwen.createdAt = QDateTime::currentDateTime();
    Qwen.updatedAt = QDateTime::currentDateTime();
    m_dao->insert(Qwen);

    //Gimini
    ApiConfigEntity Gimini;
    Gimini.id = Utils::generateUuid();
    Gimini.name = QStringLiteral("Gimini");
    Gimini.modelType = GlobalConstants::ModelType::OpenAI;
    Gimini.isBuiltin = true;
    Gimini.isOpenAICompatible = true;
    Gimini.baseDomain = QStringLiteral(
                "https://generativelanguage.googleapis.com/v1beta/openai");
    Gimini.apiPath = QStringLiteral("/chat/completions");
    Gimini.availableModels = {
        "gemini-3.5-flash-lite","gemini-4-hydrogen","gemini-4-argon","gemini-4-carbon"};
    Gimini.isActive = false;
    Gimini.createdAt = QDateTime::currentDateTime();
    Gimini.updatedAt = QDateTime::currentDateTime();
    m_dao->insert(Gimini);

    //Cluade
    ApiConfigEntity Cluade;
    Cluade.id = Utils::generateUuid();
    Cluade.name = QStringLiteral("Cluade");
    Cluade.modelType = GlobalConstants::ModelType::OpenAI;
    Cluade.isBuiltin = true;
    Cluade.isOpenAICompatible = true;
    Cluade.baseDomain = QStringLiteral("https://api.anthropic.com/v1");
    Cluade.apiPath = QStringLiteral("/chat/completions");
    Cluade.availableModels = {
        "claude-opus-5-5","claude-fable-5-2","claude-sonnet-5-5"};
    Cluade.isActive = false;
    Cluade.createdAt = QDateTime::currentDateTime();
    Cluade.updatedAt = QDateTime::currentDateTime();
    m_dao->insert(Cluade);

    //Ollama 本地
    // ApiConfigEntity ollama;
    // ollama.id = Utils::generateUuid();
    // ollama.name = QStringLiteral("Ollama 本地");
    // ollama.modelType = GlobalConstants::ModelType::OpenAI;
    // ollama.isBuiltin = true;
    // ollama.isOpenAICompatible = true;
    // ollama.baseDomain = QStringLiteral("http://localhost:11434");
    // ollama.apiPath = QStringLiteral("/v1/chat/completions");
    // ollama.availableModels = { "llama3", "qwen2", "mistral" };
    // ollama.timeout = 120000;
    // ollama.isActive = false;
    // ollama.createdAt = QDateTime::currentDateTime();
    // ollama.updatedAt = QDateTime::currentDateTime();
    // m_dao->insert(ollama);

}

bool ApiConfigService::isBuiltinConfig(const QString &id)
{
    ApiConfigEntity config = getConfigById(id);
    return config.isBuiltin;
}