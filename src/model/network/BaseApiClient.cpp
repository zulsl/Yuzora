// BaseApiClient.cpp
#include "BaseApiClient.h"
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QThread>

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
    qDebug().noquote() << "请求负载:" << payload;

    //接收post响应
    m_currentReply = m_networkManager->post(request, payload);

    // 连接信号
    connect(m_currentReply, &QNetworkReply::readyRead, this, &BaseApiClient::onReadyRead);
    connect(m_currentReply, &QNetworkReply::finished, this, &BaseApiClient::onFinished);
    // 错误信号仅用于日志，实际处理在 finished 中
    connect(m_currentReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
        this, [this](QNetworkReply::NetworkError err) {
            Q_UNUSED(err);
            qDebug() << "发生网络错误（将在完成时处理）:" << m_currentReply->errorString();
            emit requestFailed(m_currentReply->errorString());
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
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiConfig.apiKey).toUtf8());
    }else Utils::writeLog("ERROR","BaseApiClient","apiKey为空");

    m_currentReply = m_networkManager->get(request);

    //读取响应
    connect(m_currentReply, &QNetworkReply::finished, this, [this](){
        //无错误
        if (m_currentReply->error() == QNetworkReply::NoError)
        {
            //读取并保存模型列表
            QStringList modelNames = parseModelList(m_currentReply->readAll());
            emit modelListFetched(modelNames);
        }
        else
        {
            QNetworkReply::NetworkError err = m_currentReply->error();
            QString errMsg = m_currentReply->errorString();
            qDebug()<<err;
            qDebug()<<errMsg;
            emit requestFailed(QString(errMsg));
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
    return message.value("content").toString();

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
    qDebug()<<"当前m_finishedHandled:"<<m_finishedHandled;
    qDebug()<<"当前m_cancelled:"<<m_finishedHandled;

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
        // 尝试读取错误响应体
        QByteArray errorBody = reply->readAll();
        QString errorMsg = reply->errorString();
        if (!errorBody.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(errorBody);
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.contains("error")) {
                    QJsonObject errObj = obj.value("error").toObject();
                    errorMsg = errObj.value("message").toString(errorMsg);
                }
            }
        }
        // cancelAllRequests();
        emit requestFailed(errorMsg);
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

        QJsonParseError parseErr;
        QJsonDocument doc = QJsonDocument::fromJson(rawData, &parseErr);
        if (parseErr.error != QJsonParseError::NoError) {
            // cancelAllRequests();
            emit requestFailed(QStringLiteral("响应解析失败：%1").arg(parseErr.errorString()));
            return;
        }
        if (!doc.isObject()) {
            // cancelAllRequests();
            emit requestFailed(QStringLiteral("响应格式错误，非JSON对象"));
            return;
        }

        QJsonObject root = doc.object();
        // 检查业务错误
        if (root.contains("error")) {
            QJsonObject errObj = root.value("error").toObject();
            QString msg = errObj.value("message").toString(QStringLiteral("未知接口错误"));
            // cancelAllRequests();
            emit requestFailed(msg);
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

void BaseApiClient::handleError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);
    if (!m_currentReply) return;

    QString errorString = m_currentReply->errorString();

    // 尝试解析响应体中的业务错误信息（如 OpenAI 的 error.message）
    QByteArray errorBody = m_currentReply->readAll();
    if (!errorBody.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(errorBody);
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            if (root.contains("error")) {
                QJsonObject errorObj = root.value("error").toObject();
                QString msg = errorObj.value("message").toString();
                if (!msg.isEmpty()) {
                    errorString = msg;
                }
            }
        }
    }

    emit requestFailed(errorString);
}

