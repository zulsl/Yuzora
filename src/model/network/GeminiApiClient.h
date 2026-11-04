#ifndef GEMINIAPICLIENT_H
#define GEMINIAPICLIENT_H

#include "BaseApiClient.h"

/**
 * @brief Google Gemini API 客户端实现
 *
 * 适配 Gemini API 的请求结构、响应解析及 SSE 流式格式。
 * API 文档：https://ai.google.dev/gemini-api/docs
 */
class GeminiApiClient : public BaseApiClient {
    Q_OBJECT
public:
    explicit GeminiApiClient(QObject* parent = nullptr);

protected:
    // ---------- 必须实现的协议适配 ----------
    QNetworkRequest buildRequestHeader() const override;
    QJsonObject buildRequestBody(const QList<ChatMessage>& messages) const override;

    // ---------- 重写解析钩子，适配 Gemini 响应结构 ----------
    QString parseStreamChunk(const QJsonObject& chunkJson) const override;
    QString parseCompletionResponse(const QJsonObject& responseJson) const override;
    void parseTokenUsage(const QJsonObject& responseJson, int& promptTokens,
                         int& completionTokens, int& totalTokens) const override;
    QStringList parseModelList(const QByteArray& data) const override;

private:
    // 辅助函数：构建 Gemini 的 content 数组
    QJsonArray buildContents(const QList<ChatMessage>& messages) const;
};

#endif // GEMINIAPICLIENT_H