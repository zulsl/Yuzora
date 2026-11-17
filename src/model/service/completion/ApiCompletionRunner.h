#ifndef APICOMPLETIONRUNNER_H
#define APICOMPLETIONRUNNER_H

#include "ChatCompletionRunner.h"

class BaseApiClient;

// 生产实现：按当前激活的 API 配置建客户端，把流式/完成/失败信号转成 runner 信号。
// 原来这些代码长在 ChatService::startGeneration / onStreamChunkReceived 里。
class ApiCompletionRunner : public ChatCompletionRunner {
    Q_OBJECT
public:
    explicit ApiCompletionRunner(QObject* parent = nullptr);
    ~ApiCompletionRunner() override;

    void start(const QList<ChatMessage>& messages) override;
    void cancel() override;
    bool isRunning() const override;

private:
    void releaseClient();
    void onChunk(const QString& chunk);
    void onFinished(const QString& fullText);
    void onFailed(const QString& error, const QString& detail);

    BaseApiClient* m_client = nullptr;
};

#endif // APICOMPLETIONRUNNER_H
