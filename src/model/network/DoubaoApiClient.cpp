

//以下废弃
// #include "BaseApiClient_t.h"
// #include "Utils.h"
// #include <QJsonDocument>
// #include <QJsonObject>
// #include <QTimer>
// #include <QMetaEnum>

// // ==================== 辅助数据结构实现 ====================

// ChatMessage ChatMessage::fromText(GlobalConstants::MessageType role, const QString& text)
// {
//     ChatMessage msg;
//     msg.role = role;
//     ChatMessageContent content;
//     content.type = ChatMessageContent::ContentType::Text;
//     content.text = text;
//     msg.contents.append(content);
//     return msg;
// }

// // ==================== 基类构造与析构 ====================

// BaseApiClient::BaseApiClient(QObject* parent)
//     : QObject(parent)
// {
//     qRegisterMetaType<ApiError>("ApiError");
//     qRegisterMetaType<ChatMessage>("ChatMessage");
//     qRegisterMetaType<ModelConfig>("ModelConfig");
// }

// BaseApiClient::~BaseApiClient()
// {
//     cancelAllRequests();
// }

// // ==================== 对外核心接口 ====================

// void BaseApiClient::sendChatCompletion(const QString& requestId,
//                                        const QList<ChatMessage>& messages,
//                                        const ModelConfig& config)
// {
//     if (requestId.isEmpty() || messages.isEmpty()) {
//         ApiError error;
//         error.type = ApiError::ErrorType::ParameterError;
//         error.message = tr("Invalid request parameters");
//         error.isRetryable = false;
//         emit requestFailed(requestId, error);
//         return;
//     }

//     // 重复ID先清理旧请求
//     if (m_activeRequests.contains(requestId)) {
//         cancelRequest(requestId);
//     }

//     executeRequest(requestId, messages, config);
// }

// void BaseApiClient::cancelRequest(const QString& requestId)
// {
//     if (!m_activeRequests.contains(requestId))
//         return;

//     RequestContext* ctx = m_activeRequests.value(requestId);
//     if (ctx->reply) {
//         // 断开所有信号，避免取消后回调干扰
//         ctx->reply->disconnect(this);
//         if (ctx->reply->isRunning()) {
//             ctx->reply->abort();
//         }
//     }
//     if (ctx->timeoutTimer) {
//         ctx->timeoutTimer->stop();
//     }

//     // 主动取消不触发错误信号，直接清理
//     cleanupRequest(requestId);
// }

// void BaseApiClient::cancelAllRequests()
// {
//     const QStringList ids = m_activeRequests.keys();
//     for (const QString& id : ids) {
//         cancelRequest(id);
//     }
// }

// // ==================== 内部请求执行流程 ====================

// void BaseApiClient::executeRequest(const QString& requestId,
//                                    const QList<ChatMessage>& messages,
//                                    const ModelConfig& config)
// {
//     RequestContext* ctx = new RequestContext();
//     ctx->requestId = requestId;
//     ctx->messages = messages;
//     ctx->config = config;

//     // 构建网络请求
//     QNetworkRequest request;
//     setupNetworkRequest(request, config);

//     // 构建请求体
//     QJsonObject body = buildRequestBody(messages, config);
//     beforeRequest(request, body);

//     QByteArray postData = QJsonDocument(body).toJson(QJsonDocument::Compact);

//     // 发起请求
//     ctx->reply = NetworkManager::instance().getAccessManager()->post(request, postData);

//     // 连接信号
//     connect(ctx->reply, &QNetworkReply::readyRead, this, &BaseApiClient::onReplyReadyRead);
//     connect(ctx->reply, &QNetworkReply::finished, this, &BaseApiClient::onReplyFinished);
//     connect(ctx->reply, &QNetworkReply::errorOccurred, this, &BaseApiClient::onReplyError);

//     // 初始化超时定时器
//     ctx->timeoutTimer = new QTimer(this);
//     ctx->timeoutTimer->setSingleShot(true);
//     int timeout = config.timeoutMs > 0 ? config.timeoutMs : NetworkManager::instance().getGlobalTimeout();
//     ctx->timeoutTimer->setInterval(timeout);
//     connect(ctx->timeoutTimer, &QTimer::timeout, this, &BaseApiClient::onRequestTimeout);
//     ctx->timeoutTimer->start();

//     m_activeRequests.insert(requestId, ctx);
// }

// void BaseApiClient::setupNetworkRequest(QNetworkRequest& request, const ModelConfig& config)
// {
//     // 设置URL
//     QString url = config.baseUrl.isEmpty() ? m_defaultBaseUrl : config.baseUrl;
//     request.setUrl(QUrl(buildRequestUrl(config)));

//     // 通用请求头
//     request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
//     request.setSslConfiguration(NetworkManager::instance().getGlobalSslConfig());

//     // 转移编码：流式响应禁用自动解压，避免部分服务端兼容问题
//     request.setRawHeader("Accept", "text/event-stream");
// }

// // ==================== 流式响应核心：SSE 粘包拆包解析 ====================

// void BaseApiClient::onReplyReadyRead()
// {
//     QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
//     if (!reply) return;

//     // 查找对应请求上下文
//     RequestContext* ctx = nullptr;
//     for (auto it = m_activeRequests.constBegin(); it != m_activeRequests.constEnd(); ++it) {
//         if (it.value()->reply == reply) {
//             ctx = it.value();
//             break;
//         }
//     }
//     if (!ctx) return;

//     // 追加数据到缓冲区
//     ctx->streamBuffer.append(reply->readAll());

//     // 解析完整事件
//     parseSseEvents(ctx);
// }

// void BaseApiClient::parseSseEvents(RequestContext* ctx)
// {
//     // SSE标准事件分隔符为 \n\n
//     const QByteArray separator = "\n\n";

//     while (ctx->streamBuffer.contains(separator)) {
//         int sepIndex = ctx->streamBuffer.indexOf(separator);
//         QByteArray eventBlock = ctx->streamBuffer.left(sepIndex);
//         ctx->streamBuffer = ctx->streamBuffer.mid(sepIndex + separator.size());

//         // 解析事件块，提取data字段
//         QByteArrayList lines = eventBlock.split('\n');
//         QByteArray eventData;

//         for (const QByteArray& line : lines) {
//             if (line.startsWith("data:")) {
//                 QByteArray data = line.mid(5).trimmed();
//                 if (!data.isEmpty()) {
//                     if (!eventData.isEmpty()) eventData += "\n";
//                     eventData += data;
//                 }
//             }
//             // 忽略 event:、id:、注释行等
//         }

//         if (eventData.isEmpty()) continue;

//         // 标准流结束标记
//         if (eventData == "[DONE]") {
//             continue;
//         }

//         // 解析单条流式内容
//         QString chunk = parseStreamEventData(eventData);
//         if (!chunk.isEmpty()) {
//             ctx->fullResponseText += chunk;
//             emit streamChunkReceived(ctx->requestId, chunk);
//         }
//     }
// }

// // ==================== 请求完成与错误处理 ====================

// void BaseApiClient::onReplyFinished()
// {
//     QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
//     if (!reply) return;

//     // 查找上下文
//     QString requestId;
//     RequestContext* ctx = nullptr;
//     for (auto it = m_activeRequests.constBegin(); it != m_activeRequests.constEnd(); ++it) {
//         if (it.value()->reply == reply) {
//             requestId = it.key();
//             ctx = it.value();
//             break;
//         }
//     }
//     if (!ctx) return;

//     // 停止超时定时器
//     if (ctx->timeoutTimer) {
//         ctx->timeoutTimer->stop();
//     }

//     // 处理剩余缓冲区数据
//     if (!ctx->streamBuffer.isEmpty()) {
//         parseSseEvents(ctx);
//     }

//     // 判断请求结果
//     if (reply->error() == QNetworkReply::NoError) {
//         // 非流式请求：一次性解析完整响应
//         if (!ctx->config.stream) {
//             QByteArray responseData = reply->readAll();
//             QJsonDocument doc = QJsonDocument::fromJson(responseData);
//             if (doc.isObject()) {
//                 QJsonObject obj = doc.object();
//                 afterResponse(obj);

//                 int promptTokens = 0, completionTokens = 0;
//                 parseFinalResponse(obj, ctx->fullResponseText, promptTokens, completionTokens);

//                 // 兜底Token计算
//                 if (promptTokens == 0) {
//                     promptTokens = calculateMessagesToken(ctx->messages);
//                 }
//                 if (completionTokens == 0) {
//                     completionTokens = calculateTextToken(ctx->fullResponseText);
//                 }

//                 emit completionFinished(requestId, ctx->fullResponseText,
//                                         promptTokens, completionTokens,
//                                         promptTokens + completionTokens);
//             }
//         } else {
//             // 流式请求：计算最终Token
//             int promptTokens = calculateMessagesToken(ctx->messages);
//             int completionTokens = calculateTextToken(ctx->fullResponseText);
//             emit completionFinished(requestId, ctx->fullResponseText,
//                                     promptTokens, completionTokens,
//                                     promptTokens + completionTokens);
//         }

//         cleanupRequest(requestId);
//     } else {
//         // 错误已在 onReplyError 处理，这里仅做兜底清理
//         if (!m_activeRequests.contains(requestId)) return;

//         ApiError error = buildErrorFromReply(reply);

//         // 判断是否可重试
//         if (shouldRetry(error, ctx->currentRetryCount, ctx->config.maxRetryCount)) {
//             retryRequest(ctx, error);
//         } else {
//             emit requestFailed(requestId, error);
//             cleanupRequest(requestId);
//         }
//     }
// }

// void BaseApiClient::onReplyError(QNetworkReply::NetworkError error)
// {
//     Q_UNUSED(error);
//     // 错误统一在 finished 中处理，避免重复触发
// }

// void BaseApiClient::onRequestTimeout()
// {
//     QTimer* timer = qobject_cast<QTimer*>(sender());
//     if (!timer) return;

//     // 查找对应请求
//     RequestContext* ctx = nullptr;
//     for (auto it = m_activeRequests.constBegin(); it != m_activeRequests.constEnd(); ++it) {
//         if (it.value()->timeoutTimer == timer) {
//             ctx = it.value();
//             break;
//         }
//     }
//     if (!ctx) return;

//     // 中止请求，后续走finished错误流程
//     if (ctx->reply && ctx->reply->isRunning()) {
//         ctx->reply->abort();
//     }
// }

// // ==================== 错误构建与重试机制 ====================

// ApiError BaseApiClient::buildErrorFromReply(QNetworkReply* reply) const
// {
//     ApiError error;
//     error.httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
//     error.rawDetail = reply->errorString();
//     error.message = reply->errorString();

//     // 按HTTP状态码分类错误
//     int status = error.httpStatusCode;
//     if (status == 401 || status == 403) {
//         error.type = ApiError::ErrorType::AuthError;
//         error.isRetryable = false;
//         error.message = tr("Authentication failed, please check API key");
//     } else if (status == 429) {
//         error.type = ApiError::ErrorType::RateLimitError;
//         error.isRetryable = true;
//         error.message = tr("Rate limit exceeded, please try again later");
//     } else if (status >= 400 && status < 500) {
//         error.type = ApiError::ErrorType::ParameterError;
//         error.isRetryable = false;
//     } else if (status >= 500) {
//         error.type = ApiError::ErrorType::ServerError;
//         error.isRetryable = true;
//     } else if (reply->error() == QNetworkReply::TimeoutError
//                || reply->error() == QNetworkReply::OperationCanceledError) {
//         error.type = ApiError::ErrorType::TimeoutError;
//         error.isRetryable = true;
//         error.message = tr("Request timed out");
//     } else {
//         error.type = ApiError::ErrorType::NetworkError;
//         error.isRetryable = true;
//     }

//     return error;
// }

// bool BaseApiClient::shouldRetry(const ApiError& error, int currentRetry, int maxRetry) const
// {
//     return error.isRetryable && currentRetry < maxRetry;
// }

// void BaseApiClient::retryRequest(RequestContext* ctx, const ApiError& error)
// {
//     Q_UNUSED(error);
//     ctx->currentRetryCount++;

//     // 指数退避：1s → 2s → 4s
//     int delayMs = 1000 * qPow(2, ctx->currentRetryCount - 1);

//     // 清理旧请求资源
//     if (ctx->reply) {
//         ctx->reply->disconnect(this);
//         ctx->reply->deleteLater();
//         ctx->reply = nullptr;
//     }
//     if (ctx->timeoutTimer) {
//         ctx->timeoutTimer->stop();
//     }

//     // 重置缓冲区
//     ctx->streamBuffer.clear();
//     ctx->fullResponseText.clear();

//     // 延迟后重试
//     QTimer::singleShot(delayMs, this, [this, ctx]() {
//         // 二次校验：请求未被取消
//         if (!m_activeRequests.contains(ctx->requestId)) return;
//         executeRequest(ctx->requestId, ctx->messages, ctx->config);
//     });
// }

// // ==================== 资源清理 ====================

// void BaseApiClient::cleanupRequest(const QString& requestId)
// {
//     if (!m_activeRequests.contains(requestId))
//         return;

//     RequestContext* ctx = m_activeRequests.take(requestId);

//     if (ctx->reply) {
//         ctx->reply->disconnect(this);
//         ctx->reply->deleteLater();
//     }
//     if (ctx->timeoutTimer) {
//         ctx->timeoutTimer->stop();
//         ctx->timeoutTimer->deleteLater();
//     }

//     delete ctx;
// }

// // ==================== 拦截钩子默认实现 ====================

// void BaseApiClient::beforeRequest(QNetworkRequest& request, QJsonObject& requestBody)
// {
//     Q_UNUSED(request);
//     Q_UNUSED(requestBody);
// }

// void BaseApiClient::afterResponse(QJsonObject& response)
// {
//     Q_UNUSED(response);
// }

// // ==================== Token计算工具 ====================

// int BaseApiClient::calculateTextToken(const QString& text) const
// {
//     // 调用全局工具类计算，可替换为具体模型分词器
//     return Utils::calculateToken(text, GlobalConstants::ModelType::OpenAI);
// }

// int BaseApiClient::calculateMessagesToken(const QList<ChatMessage>& messages) const
// {
//     int total = 0;
//     for (const ChatMessage& msg : messages) {
//         for (const ChatMessageContent& content : msg.contents) {
//             if (content.type == ChatMessageContent::ContentType::Text) {
//                 total += calculateTextToken(content.text);
//             }
//         }
//     }
//     return total;
// }