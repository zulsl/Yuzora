#include "ApiCompletionRunner.h"

#include "ApiConfigEntity.h"
#include "ApiConfigService.h"
#include "BaseApiClient.h"
#include "CompletionPresetService.h"
#include "ModelFactory.h"

ApiCompletionRunner::ApiCompletionRunner(QObject* parent)
    : ChatCompletionRunner(parent)
{
}

ApiCompletionRunner::~ApiCompletionRunner()
{
    releaseClient();
}

void ApiCompletionRunner::start(const QList<ChatMessage>& messages)
{
    // 前置校验：没有激活的 API 配置也走 failed()，和网络错误同一条处理路径
    const ApiConfigEntity apiConfig = ApiConfigService::instance().getActiveConfig();
    if (apiConfig.id.isEmpty()) {
        emit failed(QStringLiteral("请先配置并激活API连接"));
        return;
    }

    releaseClient();
    m_client = ModelFactory::instance().createClient(apiConfig.modelType);
    if (!m_client) {
        emit failed(QStringLiteral("不支持的模型类型"));
        return;
    }

    connect(m_client, &BaseApiClient::streamChunkReceived,
            this, &ApiCompletionRunner::onChunk);
    connect(m_client, &BaseApiClient::completionFinished,
            this, &ApiCompletionRunner::onFinished);
    connect(m_client, &BaseApiClient::requestFailed,
            this, &ApiCompletionRunner::onFailed);

    m_client->loadConfig(apiConfig);
    m_client->loadPreset(CompletionPresetService::instance().getActivePreset());
    m_client->sendChatCompletion(messages);
}

void ApiCompletionRunner::cancel()
{
    releaseClient();
}

bool ApiCompletionRunner::isRunning() const
{
    return m_client != nullptr;
}

void ApiCompletionRunner::onChunk(const QString& chunk)
{
    emit chunkReceived(chunk);
}

void ApiCompletionRunner::onFinished(const QString& fullText)
{
    // 必须先复制：这个引用指向 BaseApiClient::m_fullResponse，
    // 而 releaseClient() -> cancelAllRequests() 会把那个成员 clear() 掉，
    // 之后 emit 出去的就是空字符串（表现为"回复内容为空、写入失败"）。
    const QString text = fullText;

    releaseClient();
    emit finished(text);
}

void ApiCompletionRunner::onFailed(const QString& error)
{
    releaseClient();
    emit failed(error);
}

void ApiCompletionRunner::releaseClient()
{
    if (!m_client) return;

    m_client->cancelAllRequests();
    disconnect(m_client, nullptr, this, nullptr);
    m_client->deleteLater();
    m_client = nullptr;
}
