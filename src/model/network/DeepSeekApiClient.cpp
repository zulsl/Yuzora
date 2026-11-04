// #include "DeepSeekApiClient.h"
// #include <QNetworkRequest>
// #include <QNetworkReply>
// #include <QJsonDocument>
// #include <QJsonObject>
// #include <QJsonArray>
// #include <QUrl>
// #include <QUrlQuery>
// #include <QThread>

// DeepSeekApiClient::DeepSeekApiClient(QObject *parent)
// {

// }

// void DeepSeekApiClient::sendChatCompletion(const QJsonArray &messages)
// {
//     //取消请求
//     if (m_currentReply) cancelAllRequests();

//     //放入配置
//     m_fullResponse.clear();
//     //构建请求头
//     QNetworkRequest request=buildBaseRequestHeader();

//     //构建请求体
//     QJsonObject requestBody = buildRequestBody(messages);
//     QByteArray payload = QJsonDocument(requestBody).toJson();//转换类型
//     qDebug() << "发送请求至:" << m_apiConfig.baseDomain;
//     qDebug() << "请求负载:" << payload;
//     //接收post响应
//     m_currentReply = m_networkManager->post(request, payload);

//     //连接信号
//     connect(m_currentReply, &QNetworkReply::readyRead, this, [this]() {
//         QByteArray data = m_currentReply->readAll();
//         handleStreamData(data);
//     });

//     connect(m_currentReply, &QNetworkReply::finished,this, &DeepSeekApiClient::handleFinished);

//     connect(m_currentReply, &QNetworkReply::errorOccurred,this, &DeepSeekApiClient::handleError);

// }

// void DeepSeekApiClient::fetchModelList()
// {
//     //取消请求
//     if (m_currentReply) cancelAllRequests();

//     //封装URL
//     QUrl url(m_apiConfig.baseDomain + "/models");
//     QNetworkRequest request(url);
//     //封装请求头
//     //设置授权
//     if (!m_apiConfig.apiKey.isEmpty()) {
//         request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiConfig.apiKey).toUtf8());
//     }else{
//         qDebug()<<"apiKey为空";
//         return;
//     }
//     request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
//     m_currentReply = m_networkManager->get(request);
//     //读取响应
//     connect(m_currentReply, &QNetworkReply::finished, this, [this](){
//         //无错误
//         if (m_currentReply->error() == QNetworkReply::NoError)
//         {
//             //读取并保存模型列表
//             QStringList modelNames = parseModelList(m_currentReply->readAll());
//             emit modelListFetched(modelNames);
//         }
//         else
//         {
//             emit requestFailed(m_currentReply->errorString());
//         }
//         m_currentReply->deleteLater();
//         m_currentReply = nullptr;
//     });
// }

// QNetworkRequest DeepSeekApiClient::buildBaseRequestHeader() const
// {
//     QString url=m_apiConfig.baseDomain+m_apiConfig.apiPath;
//     // //url处理(废弃)
//     // // 确保 URL 以 /chat/completions 结尾
//     // QString base = config.baseUrl.isEmpty() ? config.baseUrl : config.baseUrl;
//     // if (!base.endsWith("/chat/completions")) {
//     //     if (!base.endsWith('/')) {
//     //         base += '/';
//     //     }
//     //     base += "chat/completions";
//     // }

//     QNetworkRequest request;
//     //设置url
//     request.setUrl(QUrl(url));
//     //设置请求体格式
//     request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

//     // 注入 API 密钥鉴权头
//     QString keyTemp =m_apiConfig.apiKey;
//     if (!keyTemp.isEmpty()) {
//         request.setRawHeader("Authorization", QString("Bearer %1").arg(keyTemp).toUtf8());
//     }

//     // 注入自定义额外请求头(未完成)
//     // QVariantMap::const_iterator it;
//     // for (it = m_apiConfig.extraHeaders.constBegin(); it != m_apiConfig.extraHeaders.constEnd(); ++it) {
//     //     request.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
//     // }


//     return request;
// }

// QJsonObject DeepSeekApiClient::buildRequestBody(const QJsonArray &messages) const
// {
//     QJsonObject body;
//     // body["model"] = config.modelName;
//     // body["temperature"] = config.temperature;
//     // body["top_p"] = config.topP;
//     // body["max_tokens"] = config.maxResponseToken;
//     // body["stream"] = config.stream;
//     // body["frequency_penalty"] = config.repetitionPenalty - 1.0;

//     body["messages"] = messages;

//     // // 扩展参数合并
//     // if (!config.extraParams.isEmpty()) {
//     //     for (auto it = config.extraParams.constBegin(); it != config.extraParams.constEnd(); ++it) {
//     //         body[it.key()] = QJsonValue::fromVariant(it.value());
//     //     }
//     // }

//     return body;
// }

// void DeepSeekApiClient::handleStreamData(const QByteArray &data)
// {
//     if (m_preset.streamEnabled) {
//         // 流式模式走 SSE 逐事件解析
//         BaseApiClient::handleStreamData(data);
//     } else {
//         // 非流式模式直接缓存全部响应，结束后统一解析
//         m_streamBuffer.append(data);
//     }
// }

// void DeepSeekApiClient::handleFinished()
// {
//     // 非流式模式：统一解析完整响应体
//     if (!m_preset.streamEnabled) {
//         QByteArray rawData = m_streamBuffer;
//         m_streamBuffer.clear();

//         QJsonParseError parseErr;
//         QJsonDocument doc = QJsonDocument::fromJson(rawData, &parseErr);
//         if (parseErr.error != QJsonParseError::NoError) {
//             emit requestFailed(QStringLiteral("响应解析失败：%1").arg(parseErr.errorString()));
//             return;
//         }
//         if (!doc.isObject()) {
//             emit requestFailed(QStringLiteral("响应格式错误，非标准JSON对象"));
//             return;
//         }

//         QJsonObject root = doc.object();

//         // 业务错误拦截
//         if (root.contains("error")) {
//             QJsonObject errorObj = root.value("error").toObject();
//             QString msg = errorObj.value("message").toString(QStringLiteral("未知接口错误"));
//             emit requestFailed(msg);
//             return;
//         }

//         // 提取完整回复内容
//         QJsonArray choices = root.value("choices").toArray();
//         if (choices.isEmpty()) {
//             emit requestFailed(QStringLiteral("响应中无有效回复内容"));
//             return;
//         }

//         QJsonObject choice = choices.first().toObject();
//         QJsonObject message = choice.value("message").toObject();
//         m_fullResponse = message.value("content").toString();

//         // 上报Token用量
//         if (root.contains("usage")) {
//             QJsonObject usage = root.value("usage").toObject();
//             int promptTokens = usage.value("prompt_tokens").toInt();
//             int completionTokens = usage.value("completion_tokens").toInt();
//             int totalTokens = usage.value("total_tokens").toInt();
//             emit tokenUsageUpdated(promptTokens, completionTokens, totalTokens);
//         }
//     }

//     // 调用基类完成收尾，触发 completionFinished 信号
//     BaseApiClient::handleFinished();
// }

// QStringList DeepSeekApiClient::parseModelList(const QByteArray &data)
// {
//     // 复用基类标准解析
//     return BaseApiClient::parseModelList(data);
// }

// void DeepSeekApiClient::processSseBuffer()
// {
//     // SSE 协议规范：事件以空行分隔，每行格式为 field: value
//     while (true) {
//         // 查找换行符，兼容 \n 与 \r\n 两种换行格式
//         int newlineIdx = m_streamBuffer.indexOf('\n');
//         if (newlineIdx == -1) {
//             // 无完整行，等待下一批数据
//             break;
//         }

//         // 提取单行，去除末尾回车符
//         QByteArray line = m_streamBuffer.left(newlineIdx).trimmed();
//         // 移除缓冲区中已处理的行（含换行符）
//         m_streamBuffer = m_streamBuffer.mid(newlineIdx + 1);

//         // 空行为事件分隔符，跳过
//         if (line.isEmpty()) {
//             continue;
//         }

//         // 仅处理 data 字段
//         if (!line.startsWith("data: ")) {
//             continue;
//         }

//         // 提取 data 载荷
//         QByteArray payload = line.mid(6).trimmed();

//         // 流结束标记
//         if (payload == "[DONE]") {
//             return;
//         }

//         // 解析事件 JSON
//         QJsonParseError parseErr;
//         QJsonDocument doc = QJsonDocument::fromJson(payload, &parseErr);
//         if (parseErr.error != QJsonParseError::NoError) {
//             continue; // 非法JSON跳过，不中断整体流
//         }
//         if (!doc.isObject()) {
//             continue;
//         }

//         QJsonObject root = doc.object();

//         // 上报Token用量（部分接口在流末尾返回）
//         if (root.contains("usage")) {
//             QJsonObject usage = root.value("usage").toObject();
//             int promptTokens = usage.value("prompt_tokens").toInt();
//             int completionTokens = usage.value("completion_tokens").toInt();
//             int totalTokens = usage.value("total_tokens").toInt();
//             emit tokenUsageUpdated(promptTokens, completionTokens, totalTokens);
//         }

//         // 提取文本增量
//         QJsonArray choices = root.value("choices").toArray();
//         if (choices.isEmpty()) {
//             continue;
//         }

//         QJsonObject choice = choices.first().toObject();
//         QJsonObject delta = choice.value("delta").toObject();

//         if (delta.contains("content")) {
//             QString chunk = delta.value("content").toString();
//             if (!chunk.isEmpty()) {
//                 m_fullResponse.append(chunk);
//                 emit streamChunkReceived(chunk);
//             }
//         }
//     }

// }

