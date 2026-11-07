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
    // 首次启动初始化内置预设配置
    initBuiltinConfigs();
}

ApiConfigService::~ApiConfigService() = default;

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

    bool success = m_dao->insert(config);
    if (success) {
        //成功
    } else {
        emit operationError(QStringLiteral("创建配置失败，数据库写入错误"));
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

// ==================== 连接测试 ====================
void ApiConfigService::testConnection(const QString &configId)
{
    // ApiConfigEntity config = getConfigById(configId);
    // if (config.id.isEmpty()) {
    //     emit testFailed(configId, QStringLiteral("配置不存在"));
    //     return;
    // }
    // if (config.baseDomain.isEmpty() || config.apiPath.isEmpty()) {
    //     emit testFailed(configId, QStringLiteral("请先填写完整的 API 域名与接口路径"));
    //     return;
    // }

    // // 创建临时客户端，测试完成后自动销毁
    // OpenAICompatibleApiClient* testClient = new OpenAICompatibleApiClient(this);
    // testClient->loadConfig(config);

    // // 构造极简测试请求，max_tokens=1 快速返回
    // QJsonArray testMessages;
    // QJsonObject testMsg;
    // testMsg["role"] = "user";
    // testMsg["content"] = "hi";
    // testMessages.append(testMsg);

    // ModelConfig testCfg;
    // testCfg.modelName = config.availableModels.isEmpty() ? QString() : config.availableModels.first();
    // testCfg.maxResponseToken = 1;
    // testCfg.stream = false;

    // connect(testClient, &OpenAICompatibleApiClient::completionFinished,
    //         this, [this, configId, testClient](const QString&) {
    //             emit testSuccess(configId);
    //             testClient->deleteLater();
    //         });

    // connect(testClient, &OpenAICompatibleApiClient::requestFailed,
    //         this, [this, configId, testClient](const QString& error) {
    //             emit testFailed(configId, error);
    //             testClient->deleteLater();
    //         });

    // testClient->sendChatCompletion(testMessages, testCfg);
}

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
    openai.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    openai.name = QStringLiteral("OpenAI 官方");
    openai.modelType = GlobalConstants::ModelType::OpenAI;
    openai.isBuiltin = true;
    openai.isOpenAICompatible = true;
    openai.baseDomain = QStringLiteral("https://api.openai.com");
    openai.apiPath = QStringLiteral("/v1/chat/completions");
    openai.availableModels = { "gpt-4o", "gpt-4-turbo", "gpt-3.5-turbo", "gpt-4o-mini" };
    openai.isActive = true;
    openai.createdAt = QDateTime::currentDateTime();
    openai.updatedAt = QDateTime::currentDateTime();
    m_dao->insert(openai);

    //DeepSeek
    ApiConfigEntity deepseek;
    deepseek.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    deepseek.name = QStringLiteral("DeepSeek");
    deepseek.modelType = GlobalConstants::ModelType::OpenAI;
    deepseek.isBuiltin = true;
    deepseek.isOpenAICompatible = true;
    deepseek.baseDomain = QStringLiteral("https://api.deepseek.com");
    deepseek.apiPath = QStringLiteral("/chat/completions");
    deepseek.availableModels = { "deepseek-chat", "deepseek-reasoner" };
    deepseek.isActive = false;
    deepseek.createdAt = QDateTime::currentDateTime();
    deepseek.updatedAt = QDateTime::currentDateTime();
    m_dao->insert(deepseek);

    //Ollama 本地
    ApiConfigEntity ollama;
    ollama.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ollama.name = QStringLiteral("Ollama 本地");
    ollama.modelType = GlobalConstants::ModelType::OpenAI;
    ollama.isBuiltin = true;
    ollama.isOpenAICompatible = true;
    ollama.baseDomain = QStringLiteral("http://localhost:11434");
    ollama.apiPath = QStringLiteral("/v1/chat/completions");
    ollama.availableModels = { "llama3", "qwen2", "mistral" };
    ollama.timeout = 120000;
    ollama.isActive = false;
    ollama.createdAt = QDateTime::currentDateTime();
    ollama.updatedAt = QDateTime::currentDateTime();
    m_dao->insert(ollama);

}

bool ApiConfigService::isBuiltinConfig(const QString &id)
{
    ApiConfigEntity config = getConfigById(id);
    return config.isBuiltin;
}