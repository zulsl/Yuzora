#include "OpenAICompatibleApiClient.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QThread>
#include <limits>

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
    qDebug()<<"当前请求地址:"<<url;
    //设置请求体格式
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        // 注入 API 密钥鉴权头
    if (!m_apiConfig.apiKey.isEmpty()) {
        request.setRawHeader(
            "Authorization", QString("Bearer %1").arg(m_apiConfig.apiKey).toUtf8());
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
    body["stream"] = m_preset.streamEnabled;
    // body["stream_options"] = m_preset.stream_options;
    // body["thinking"] = m_preset.thinking;
    // body["reasoning_effort"] = m_preset.reasoning_effort;
    body["max_tokens"] = m_preset.maxResponseToken;
    // body["response_format"] = m_preset.response_format;
    // body["stop"] = m_preset.stop;
    body["temperature"] = m_preset.temperature;

    // top_p 的合法区间是 (0, 1.0]。预设里的 0 表示"不启用 top_p"，
    // 直接发 0 会被服务端判成参数非法（DeepSeek 实测：
    //   HTTP 400 "Invalid top_p value, the valid range of top_p is (0, 1.0]"）。
    // ST 对 DeepSeek 就是这么处理的（openai.js:3015-3018）：
    //   generate_data.top_p = generate_data.top_p || Number.EPSILON;
    double topP = m_preset.topP;
    if (topP <= 0.0) topP = std::numeric_limits<double>::epsilon();
    body["top_p"] = topP;


    // 消息数组转换
    QJsonArray msgArray;
    for (const ChatMessage& msg : messages) {
        QJsonObject obj;
        obj = msg.toJson();
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







