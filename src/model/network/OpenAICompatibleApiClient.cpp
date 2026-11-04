#include "OpenAICompatibleApiClient.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QThread>

OpenAICompatibleApiClient::OpenAICompatibleApiClient(QObject *parent)
{

}


QNetworkRequest OpenAICompatibleApiClient::buildRequestHeader() const
{
    QString url=m_apiConfig.baseDomain+m_apiConfig.apiPath;
    // //url处理(废弃)
    // // 确保 URL 以 /chat/completions 结尾
    // QString base = config.baseUrl.isEmpty() ? config.baseUrl : config.baseUrl;
    // if (!base.endsWith("/chat/completions")) {
    //     if (!base.endsWith('/')) {
    //         base += '/';
    //     }
    //     base += "chat/completions";
    // }

    QNetworkRequest request;
    //设置url
    request.setUrl(QUrl(url));
    //设置请求体格式
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        // 注入 API 密钥鉴权头
    if (!m_apiConfig.apiKey.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiConfig.apiKey).toUtf8());
    }

    // 注入自定义额外请求头(未完成)
    // QVariantMap::const_iterator it;
    // for (it = m_apiConfig.extraHeaders.constBegin(); it != m_apiConfig.extraHeaders.constEnd(); ++it) {
    //     request.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
    // }


    return request;
}

QJsonObject OpenAICompatibleApiClient::buildRequestBody(const QList<ChatMessage>& messages) const
{
    QJsonObject body;
    body["model"] = m_apiConfig.currentModel;
    body["temperature"] = m_preset.temperature;
    body["top_p"] = m_preset.topP;
    body["max_tokens"] = m_preset.maxResponseToken;
    body["stream"] = m_preset.streamEnabled;
    body["frequency_penalty"] = m_preset.repetitionPenalty;

    // 消息数组转换
    QJsonArray msgArray;
    for (const ChatMessage& msg : messages) {
        QJsonObject obj;

        // 角色映射
        switch (msg.role) {
        case GlobalConstants::MessageType::System:    obj["role"] = "system"; break;
        case GlobalConstants::MessageType::User:      obj["role"] = "user"; break;
        case GlobalConstants::MessageType::Assistant: obj["role"] = "assistant"; break;
        case GlobalConstants::MessageType::ToolCall:      obj["role"] = "tool"; break;
        }

        // 单文本片段 → 字符串 content
        if (msg.parts.size() == 1 && msg.parts[0].type == ChatMessagePart::Type::Text) {
            obj["content"] = msg.parts[0].data;
        }
        // 多片段 → 数组 content（多模态）
        else {
            QJsonArray partsArray;
            for (const ChatMessagePart& part : msg.parts) {
                QJsonObject partObj;
                if (part.type == ChatMessagePart::Type::Text) {
                    partObj["type"] = "text";
                    partObj["text"] = part.data;
                } else if (part.type == ChatMessagePart::Type::ImageUrl) {
                    partObj["type"] = "image_url";
                    QJsonObject urlObj;
                    urlObj["url"] = part.data;
                    partObj["image_url"] = urlObj;
                }
                partsArray.append(partObj);
            }
            obj["content"] = partsArray;
        }

        //废弃
        // if (!msg.senderName.isEmpty()) {
        //     obj["name"] = msg.senderName;
        // }

        msgArray.append(obj);
    }

    body["messages"] = msgArray;

    // // 额外参数合并
    // if (!m_preset.extraSettings.isEmpty()) {
    //     QJsonObject extra = Utils::variantMapToJsonObject(m_preset.extraSettings);
    //     for (auto it = extra.begin(); it != extra.end(); ++it) {
    //         body[it.key()] = it.value();
    //     }
    // }

    return body;
}







