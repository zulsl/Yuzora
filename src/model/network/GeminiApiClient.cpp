#include "GeminiApiClient.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>

GeminiApiClient::GeminiApiClient(QObject* parent)
    : BaseApiClient(parent)
{
}

// ---------- 协议适配 ----------
QNetworkRequest GeminiApiClient::buildRequestHeader() const
{
    // Gemini 推荐将 API Key 作为查询参数
    // URL 格式：https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent?key=YOUR_API_KEY
    QString base = m_apiConfig.baseDomain;
    if (base.isEmpty()) {
        base = "https://generativelanguage.googleapis.com";
    }
    QString apiPath = m_apiConfig.apiPath;
    if (apiPath.isEmpty()) {
        apiPath = "/v1beta/models/" + m_apiConfig.currentModel + ":generateContent";
    } else {
        // 如果配置了完整路径，直接使用，但需确保包含模型占位符？此处简化处理
        // 实际使用时建议在配置中提供完整路径
    }

    QUrl url(base + apiPath);
    QUrlQuery query;
    if (!m_apiConfig.apiKey.isEmpty()) {
        query.addQueryItem("key", m_apiConfig.apiKey);
    }
    url.setQuery(query);

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Gemini 也支持 x-goog-api-key 头，但查询参数方式更通用，此处不额外添加

    return request;
}

QJsonObject GeminiApiClient::buildRequestBody(const QList<ChatMessage>& messages) const
{
    QJsonObject body;

    // 1. contents 数组
    body["contents"] = buildContents(messages);

    // 2. generationConfig
    QJsonObject generationConfig;
    generationConfig["temperature"] = m_preset.temperature;
    generationConfig["topP"] = m_preset.topP;
    generationConfig["maxOutputTokens"] = m_preset.maxResponseToken;
    // // 可选：停止序列
    // if (!m_preset.stopSequences.isEmpty()) {
    //     QJsonArray stopArr;
    //     for (const QString& stop : m_preset.stopSequences) {
    //         stopArr.append(stop);
    //     }
    //     generationConfig["stopSequences"] = stopArr;
    // }
    body["generationConfig"] = generationConfig;

    // 3. 流式标志
    body["stream"] = m_preset.streamEnabled;

    // 4. safetySettings 等可选，暂不添加

    return body;
}

// ---------- 解析钩子重写 ----------
QString GeminiApiClient::parseStreamChunk(const QJsonObject& chunkJson) const
{
    // Gemini 流式响应结构：
    // {
    //   "candidates": [
    //     {
    //       "content": {
    //         "parts": [ { "text": "..." } ],
    //         "role": "model"
    //       }
    //     }
    //   ]
    // }
    QJsonArray candidates = chunkJson.value("candidates").toArray();
    if (candidates.isEmpty()) {
        return QString();
    }
    QJsonObject candidate = candidates.first().toObject();
    QJsonObject content = candidate.value("content").toObject();
    QJsonArray parts = content.value("parts").toArray();
    if (parts.isEmpty()) {
        return QString();
    }
    QJsonObject part = parts.first().toObject();
    return part.value("text").toString();
}

QString GeminiApiClient::parseCompletionResponse(const QJsonObject& responseJson) const
{
    // 非流式响应结构与流式类似，但包含完整内容
    QJsonArray candidates = responseJson.value("candidates").toArray();
    if (candidates.isEmpty()) {
        return QString();
    }
    QJsonObject candidate = candidates.first().toObject();
    QJsonObject content = candidate.value("content").toObject();
    QJsonArray parts = content.value("parts").toArray();
    if (parts.isEmpty()) {
        return QString();
    }
    QJsonObject part = parts.first().toObject();
    return part.value("text").toString();
}

void GeminiApiClient::parseTokenUsage(const QJsonObject& responseJson, int& promptTokens,
                                      int& completionTokens, int& totalTokens) const
{
    // Gemini 的用量在 usageMetadata 中：
    // "usageMetadata": {
    //   "promptTokenCount": 10,
    //   "candidatesTokenCount": 20,
    //   "totalTokenCount": 30
    // }
    QJsonObject usage = responseJson.value("usageMetadata").toObject();
    promptTokens = usage.value("promptTokenCount").toInt();
    completionTokens = usage.value("candidatesTokenCount").toInt();
    totalTokens = usage.value("totalTokenCount").toInt();
}

QStringList GeminiApiClient::parseModelList(const QByteArray& data) const
{
    // Gemini 模型列表响应：
    // {
    //   "models": [
    //     { "name": "models/gemini-pro", "displayName": "Gemini Pro", ... },
    //     ...
    //   ]
    // }
    QStringList result;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return result;

    QJsonObject root = doc.object();
    QJsonArray models = root.value("models").toArray();
    for (const auto& item : models) {
        QJsonObject obj = item.toObject();
        QString name = obj.value("name").toString();
        // 名称通常为 "models/gemini-pro"，提取最后一部分作为模型ID
        if (name.startsWith("models/")) {
            name = name.mid(7); // 去掉 "models/"
        }
        result << name;
    }
    return result;
}

// ---------- 辅助函数 ----------
QJsonArray GeminiApiClient::buildContents(const QList<ChatMessage>& messages) const
{
    QJsonArray contents;

    // for (const ChatMessage& msg : messages) {
    //     QJsonObject content;
    //     // 角色映射：User -> "user", Assistant -> "model"
    //     // Gemini 不支持 system 角色，通常将系统指令作为第一条 user 消息处理，或使用 system_instruction 字段（需额外支持）
    //     switch (msg.role) {
    //     case GlobalConstants::MessageType::System:
    //         // 为了简化，将系统消息作为 user 消息，但加上特定前缀？或者忽略？这里按普通 user 处理，但需用户自行处理
    //         // 更规范的用法是使用 system_instruction 字段，我们可以在 buildRequestBody 中单独处理第一个 system 消息
    //         // 但为保持通用，直接将 system 当作 user，内容前面加 "[System] " ？但更好的是在外部处理，我们仅做映射
    //         content["role"] = "user";
    //         break;
    //     case GlobalConstants::MessageType::User:
    //         content["role"] = "user";
    //         break;
    //     case GlobalConstants::MessageType::Assistant:
    //         content["role"] = "model";
    //         break;
    //     default:
    //         content["role"] = "user"; // fallback
    //     }

    //     // 构建 parts 数组
    //     QJsonArray parts;
    //     for (const ChatMessagePart& part : msg.parts) {
    //         QJsonObject partObj;
    //         if (part.type == ChatMessagePart::Type::Text) {
    //             partObj["text"] = part.data;
    //         } else if (part.type == ChatMessagePart::Type::ImageUrl) {
    //             // Gemini 支持 inlineData 或 fileData，此处简化，仅做示例
    //             // 假设 data 是 base64 图片数据或 URL，Gemini 需使用 inlineData
    //             // 此处省略具体实现，仅做示意
    //             partObj["inlineData"] = QJsonObject{{"mimeType", "image/jpeg"}, {"data", part.data}};
    //         }
    //         parts.append(partObj);
    //     }
    //     content["parts"] = parts;

    //     // 如果有多条消息，且角色相同，Gemini 允许合并，但为简单，我们每条独立
    //     contents.append(content);
    // }

    return contents;
}