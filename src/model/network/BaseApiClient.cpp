// BaseApiClient.cpp
#include "BaseApiClient.h"
#include <QDir>
#include <QFile>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QThread>
#include <QUrl>

#include "NetworkError.h"
#include "Utils.h"

BaseApiClient::BaseApiClient(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
{
}

BaseApiClient::~BaseApiClient()
{
    cancelAllRequests();
}

void BaseApiClient::sendChatCompletion(const QList<ChatMessage> &messages)
{
    // 重置状态
    // 取消旧请求，并确保清理干净
    if (m_currentReply) cancelAllRequests();

    m_finishedHandled = false;
    m_isStreaming = m_preset.streamEnabled;

    // 构建请求
    QNetworkRequest request = buildRequestHeader();
    QJsonObject bodyJson = buildRequestBody(messages);
    QByteArray payload = QJsonDocument(bodyJson).toJson();
    qDebug() << "发送请求至:" << m_apiConfig.baseDomain;
    qDebug().noquote() << "请求负载(UTF-8):\n" << payload;


    // 请求体是 UTF-8 的 QByteArray，这里**不要**包成 QString 再打：
    // Qt 把 QString 写日志时会先转成本地 8 位编码（中文 Windows 是 GBK），
    // 在 Qt Creator 的 Application Output（按 UTF-8 解码）里就全是乱码。
    // 直接打 QByteArray 才是原始 UTF-8 字节；控制台里要正常看就先 chcp 65001。
    // 想百分之百确认发出去的字节：设 YUZORA_DUMP_REQUEST=1
    // <数据目录>/logs/request_last.json，用 UTF-8 编辑器打开即可。
    if (payload.contains("\xEF\xBF\xBD")) {
        qWarning() << "[BaseApiClient] 请求体里出现 U+FFFD，说明上游文本已损坏（不是编码显示问题）";
    }
    if (qEnvironmentVariableIsSet("YUZORA_DUMP_REQUEST")) {//设 YUZORA_DUMP_REQUEST=1，会落盘到
        // 落在当前工作目录（Qt Creator 里就是构建目录）
        QFile dumpFile(QDir::current().filePath(QStringLiteral("request_last.json")));
        if (dumpFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
            dumpFile.write(payload);
    }


    //接收post响应
    m_currentReply = m_networkManager->post(request, payload);

    // 连接信号
    connect(m_currentReply, &QNetworkReply::readyRead, this, &BaseApiClient::onReadyRead);
    connect(m_currentReply, &QNetworkReply::finished, this, &BaseApiClient::onFinished);
    // 错误信号仅用于日志，实际处理和上报在 onFinished 里做。
    // 以前这里也 emit requestFailed(...)，一次失败会走两遍 error 流程：
    // 第二次进来时 runner 已经 deleteLater + cancelAllRequests()，
    // 于是对已经出错的 reply 再 abort()，Qt 报
    // "QNetworkReplyImplPrivate::error: Internal problem, this method must only be called once."
    connect(m_currentReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
        this, [this](QNetworkReply::NetworkError err) {
            Q_UNUSED(err);
            if (!m_currentReply) return;
            qDebug() << "发生网络错误（将在完成时处理）:"
                     << int(m_currentReply->error()) << m_currentReply->errorString();
        });
}

void BaseApiClient::fetchModelList()
{
    if (m_currentReply) cancelAllRequests();

    m_finishedHandled = false;
    m_isStreaming = false; // 非流式

    //封装URL
    QUrl url(m_apiConfig.baseDomain + "/models");
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_apiConfig.apiKey.isEmpty()) {
        request.setRawHeader(
            "Authorization", QString("Bearer %1").arg(m_apiConfig.apiKey).toUtf8());
    }else Utils::writeLog("ERROR","BaseApiClient","apiKey为空");

    m_currentReply = m_networkManager->get(request);

    //读取响应
    connect(m_currentReply, &QNetworkReply::finished, this, [this](){
        //无错误
        if (m_currentReply->error() == QNetworkReply::NoError)
        {
            //读取并保存模型列表
            const QByteArray body = m_currentReply->readAll();
            const QStringList modelNames = parseModelList(body);

            if (modelNames.isEmpty()) {
                // 200 但没有模型：可能是服务端回了 error（有的网关这么干），
                // 也可能这个接口根本不支持 /models——两种都别报"获取成功"。
                const QString endpoint = m_currentReply->url().toString(QUrl::RemoveQuery);
                NetworkError::Info info =
                    NetworkError::classify(0, 200, QString(), body, endpoint);
                if (info.providerMessage.isEmpty()) {
                    info = NetworkError::emptyResponse(
                        endpoint,
                        QStringLiteral("接口没有返回任何模型"),
                        QStringLiteral("确认 baseDomain 以 /v1 结尾，并且该服务支持 GET /models；"
                                       "有些服务不提供模型列表，需要手动填写模型名。"));
                }
                Utils::writeLog("ERROR", "BaseApiClient",
                                QStringLiteral("模型列表为空：\n%1\n响应体：%2")
                                    .arg(info.detail(), QString::fromUtf8(body.left(2000))));
                emit requestFailed(info.userMessage(), info.detail());
            } else {
                qDebug() << "模型获取成功:" << modelNames;
                emit modelListFetched(modelNames);
            }
        }
        else
        {
            const QByteArray errorBody = m_currentReply->readAll();
            const NetworkError::Info info =
                NetworkError::fromReply(m_currentReply, errorBody);
            Utils::writeLog("ERROR", "BaseApiClient",
                            QStringLiteral("拉取模型列表失败：\n%1\n响应体：%2")
                                .arg(info.detail(), QString::fromUtf8(errorBody.left(2000))));
            emit requestFailed(info.userMessage(), info.detail());
        }
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    });
    connect(m_currentReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
        this, [this](QNetworkReply::NetworkError err) {
            qDebug() << "拉取错误:" << m_currentReply->errorString();
            qDebug() << "QNetworkReply::NetworkError" << err;
        }
    );
}

void BaseApiClient::cancelAllRequests()
{
    if (m_currentReply) {
        // 断开所有连接，避免槽被触发
        disconnect(m_currentReply, nullptr, this, nullptr);
        // 已经结束（尤其是已经出错的 reply）不要再 abort：
        // Qt 会报 "Internal problem, this method must only be called once."
        if (!m_currentReply->isFinished())
            m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        // 清空缓冲区
        m_streamBuffer.clear();
        m_fullResponse.clear();
        emit cancelled();
    }
}

void BaseApiClient::loadConfig(const ApiConfigEntity &config)
{
    m_apiConfig=config;
}

void BaseApiClient::loadPreset(const CompletionPresetEntity &preset)
{
    m_preset=preset;
}


void BaseApiClient::setProxy(const QString& host, int port)
{
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::HttpProxy);
    proxy.setHostName(host);
    proxy.setPort(port);
    m_networkManager->setProxy(proxy);
}

QString BaseApiClient::parseStreamChunk(const QJsonObject &chunkJson) const
{
    //默认兼容openai
    QJsonArray choices = chunkJson.value("choices").toArray();
    if (choices.isEmpty()) return QString();
    QJsonObject choice = choices.first().toObject();
    QJsonObject delta = choice.value("delta").toObject();
    return delta.value("content").toString();
}

QString BaseApiClient::parseCompletionResponse(const QJsonObject &responseJson) const
{
    //非流式文本
    QJsonArray choices = responseJson.value("choices").toArray();
    if (choices.isEmpty()) return QString();
    QJsonObject choice = choices.first().toObject();
    QJsonObject message = choice.value("message").toObject();

    // content 可能是字符串，也可能是多模态数组（[{"type":"text","text":"..."}]）
    const QJsonValue content = message.value("content");
    if (content.isArray()) {
        QStringList texts;
        for (const QJsonValue& item : content.toArray()) {
            const QJsonObject part = item.toObject();
            const QString text = part.value("text").toString();
            if (!text.isEmpty())
                texts.append(text);
        }
        return texts.join(QString());
    }
    return content.toString();

}

void BaseApiClient::parseTokenUsage(const QJsonObject &responseJson, int &promptTokens, int &completionTokens, int &totalTokens) const
{
    QJsonObject usage = responseJson.value("usage").toObject();
    promptTokens = usage.value("prompt_tokens").toInt();
    completionTokens = usage.value("completion_tokens").toInt();
    totalTokens = usage.value("total_tokens").toInt();
}

QStringList BaseApiClient::parseModelList(const QByteArray &data) const
{
    //openai规范
    QStringList result;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return result;

    QJsonObject root = doc.object();
    QJsonArray dataArr = root["data"].toArray();
    for (const auto& item : dataArr)
    {
        QJsonObject obj = item.toObject();
        result << obj["id"].toString();
    }
    return result;
}

void BaseApiClient::onReadyRead()
{
    if (!m_currentReply) return;
    QByteArray data = m_currentReply->readAll();


    // 调用虚函数处理流式数据（基类默认实现 SSE 解析）
    if (m_isStreaming) {
        m_streamBuffer.append(data);
        processSseBuffer();
    } else {
        // 非流式直接缓存
        m_streamBuffer.append(data);
    }

    // qDebug().noquote()<<"当前响应文本块:\n"<<data;
    // qDebug().noquote()<<"当前sse缓冲区:\n"<<m_streamBuffer;

}

void BaseApiClient::onFinished()
{
    qDebug() << "onFinished, 已处理过:" << m_finishedHandled;

    if (m_finishedHandled || !m_currentReply) return;
    m_finishedHandled = true;

    // // 检查是否被取消
    // if (m_cancelled) {
    //     cleanupReply(false); // 已经发射过 cancelled
    //     return;
    // }

    QNetworkReply* reply = m_currentReply;
    QNetworkReply::NetworkError err = reply->error();

    //请求错误
    if (err != QNetworkReply::NoError) {
        // 错误响应体可能已经被 onReadyRead 读进 m_streamBuffer 了，
        // 此时 reply->readAll() 是空的——两个都要看。
        QByteArray errorBody = m_streamBuffer;
        errorBody += reply->readAll();
        m_streamBuffer.clear();

        // 统一翻译成「结论 + 建议 + 技术细节」；原始 body 只进日志，不进界面
        const NetworkError::Info info = NetworkError::fromReply(reply, errorBody);
        Utils::writeLog("ERROR", "BaseApiClient",
                        QStringLiteral("请求失败：\n%1\n响应体：%2")
                            .arg(info.detail(), QString::fromUtf8(errorBody.left(2000))));
        qWarning().noquote() << "请求失败:" << info.userMessage();
        emit requestFailed(info.userMessage(), info.detail());
        return;
    }


    // 成功：处理响应数据
    if (m_isStreaming) {
        // 流式模式下，可能还有未处理的缓冲区数据
        processSseBuffer(); // 处理剩余数据
        // 流式已完成，发射完整文本
        emit completionFinished(m_fullResponse);
    } else {
        // 非流式模式：解析完整响应
        QByteArray rawData = m_streamBuffer;
        m_streamBuffer.clear();

        const QString endpoint = reply->url().toString(QUrl::RemoveQuery);
        const QVariant statusVar = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        const int status = statusVar.isValid() ? statusVar.toInt() : 0;

        QJsonParseError parseErr;
        QJsonDocument doc = QJsonDocument::fromJson(rawData, &parseErr);
        if (parseErr.error != QJsonParseError::NoError) {
            const NetworkError::Info info =
                NetworkError::parseFailure(parseErr.errorString(), rawData, endpoint, status);
            Utils::writeLog("ERROR", "BaseApiClient",
                            QStringLiteral("响应解析失败：\n%1").arg(info.detail()));
            emit requestFailed(info.userMessage(), info.detail());
            return;
        }
        if (!doc.isObject()) {
            const NetworkError::Info info = NetworkError::parseFailure(
                QStringLiteral("响应是合法 JSON，但不是对象"), rawData, endpoint, status);
            emit requestFailed(info.userMessage(), info.detail());
            return;
        }

        QJsonObject root = doc.object();
        // 检查业务错误（有些服务 HTTP 200 也返回 error）
        if (root.contains("error")) {
            NetworkError::Info info = NetworkError::classify(0, status, QString(), rawData, endpoint);
            if (info.providerMessage.isEmpty()) {
                info.kind = NetworkError::Kind::BadRequest;
                info.summary = QStringLiteral("服务返回错误");
                info.hint = QStringLiteral("服务端给了 error 字段但没给说明，"
                                           "查看详情里的响应片段。");
            }
            Utils::writeLog("ERROR", "BaseApiClient",
                            QStringLiteral("业务错误：\n%1").arg(info.detail()));
            emit requestFailed(info.userMessage(), info.detail());
            return;
        }

        // 提取文本
        m_fullResponse = parseCompletionResponse(root);

        qDebug()<<"完整响应文本:"<<m_fullResponse;

        // 提取Token
        int prompt = 0, completion = 0, total = 0;
        parseTokenUsage(root, prompt, completion, total);
        if (total > 0) {
            emit tokenUsageUpdated(prompt, completion, total);
        }

        emit completionFinished(m_fullResponse);
    }


    // cancelAllRequests();
}


void BaseApiClient::processSseBuffer()
{
    // 标准 SSE 解析：以 \n 分隔行，处理 data: 字段
    while (true) {
        int newlineIdx = m_streamBuffer.indexOf('\n');//查找换行符
        if (newlineIdx == -1) break;//无完整行,结束循环,等待下一批数据

        QByteArray line = m_streamBuffer.left(newlineIdx).trimmed();//提取单行,去除末尾回车符
        m_streamBuffer = m_streamBuffer.mid(newlineIdx + 1);//移除缓冲区中已处理的行（含换行符）

        if (line.isEmpty()) continue;//空行为事件分隔符，跳过
        if (!line.startsWith("data: ")) continue;//仅处理 data 字段

        QByteArray payload = line.mid(6).trimmed();//提取 data 载荷
        if (payload == "[DONE]") {//流结束标记，可忽略
            continue;
        }

        //解析事件 JSON
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            continue;//非法JSON和空JSON跳过，不中断整体流
        }

        QJsonObject obj = doc.object();

    // 提取Token（部分接口在流末尾发送）
        int prompt = 0, completion = 0, total = 0;
        parseTokenUsage(obj, prompt, completion, total);
        if (total > 0) {
            emit tokenUsageUpdated(prompt, completion, total);
        }

        // 提取内容增量
        QString chunk = parseStreamChunk(obj);
        qDebug()<<"当前收到流块:"<<chunk;
        if (!chunk.isEmpty()) {
            m_fullResponse.append(chunk);
            emit streamChunkReceived(chunk);
        }
    }
}

