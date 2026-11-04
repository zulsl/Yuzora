#include "Net_test_VM.h"
#include "ExceptionHandler.h"
#include <QJsonArray>
#include <QJsonObject>

Net_test_VM::Net_test_VM(QObject *parent)
    : QObject(parent),
    m_curModelType(GlobalConstants::ModelType::DeepSeek),
    m_isLoading(false)
{
    // 绑定模型工厂全局信号
    // connect(&ModelFactory::instance(), &ModelFactory::modelListReady,
    //         this, &Net_test_VM::onModelListReady);
    // connect(&ModelFactory::instance(), &ModelFactory::modelListLoadFailed,
    //         this, &Net_test_VM::onModelListFailed);
}

GlobalConstants::ModelType Net_test_VM::currentModelType() const
{
    return m_curModelType;
}

void Net_test_VM::setCurrentModelType(GlobalConstants::ModelType type)
{
//设置当前子模型
    if (m_curModelType == type) {
        return;
    }
    m_curModelType = type;
    m_curModelList=QStringList();
    emit modelListChanged();
    emit currentModelTypeChanged();

    // 切换模型类型自动重新拉取模型列表
    // loadModelList();
}

QStringList Net_test_VM::modelList() const
{
    return m_curModelList;
}

QString Net_test_VM::requestLog() const
{
    return m_requestLog;
}

QString Net_test_VM::receiveContent() const
{
    return m_receiveContent;
}

bool Net_test_VM::isLoading() const
{
    return m_isLoading;
}

//(核心完成)
void Net_test_VM::loadModelList()
{
    if(!m_modelList[m_curModelType].isEmpty()){
        m_curModelList=m_modelList[m_curModelType];
        return;
    }


    //打印信息
    m_requestLog = "开始拉取模型列表...";
    emit requestLogChanged();

    //清空当前模型列表
    m_curModelList.clear();
    m_receiveContent.clear();
    emit modelListChanged();

    // 创建网络客户端
    m_tempClient = ModelFactory::instance().createClient(m_curModelType);
    if (!m_tempClient)
    {
        m_requestLog = "创建网络客户端失败";
        m_isLoading = false;
        emit requestLogChanged();
        emit isLoadingChanged();
        return;
    }

    // 绑定响应信号
    connect(m_tempClient, &BaseApiClient::modelListFetched,[=](QStringList modelNames){
        onModelListReady(m_curModelType,modelNames);});
    connect(m_tempClient, &BaseApiClient::requestFailed, this, &Net_test_VM::onRequestError);
    //发起请求
    m_tempClient->fetchModelList();
    // 临时客户端，执行完即可释放（也可根据业务改为常驻）
    // m_tempClient->deleteLater();
}

//(核心完成)
void Net_test_VM::sendChatRequest(const QString &userInput, const QString &selectModel)
{
    //判断是否为空
    if (m_isLoading || userInput.isEmpty() || selectModel.isEmpty())
    {
        m_requestLog = "输入内容为空或请求正在进行中";
        emit requestLogChanged();
        return;
    }

    // 清理旧请求
    cancelAllRequest();

    m_isLoading = true;
    m_receiveContent.clear();
    emit isLoadingChanged();
    emit receiveContentChanged();

    m_requestLog = QString("发起对话请求，模型：%1，输入：%2").arg(selectModel, userInput);
    emit requestLogChanged();

    // 创建网络客户端
    m_tempClient = ModelFactory::instance().createClient(m_curModelType);
    if (!m_tempClient)
    {
        m_requestLog = "创建网络客户端失败";
        m_isLoading = false;
        emit requestLogChanged();
        emit isLoadingChanged();
        return;
    }

    // 绑定网络信号
    connect(m_tempClient, &BaseApiClient::streamChunkReceived, this, &Net_test_VM::onStreamChunk);
    connect(m_tempClient, &BaseApiClient::completionFinished, this, &Net_test_VM::onRequestFinish);
    connect(m_tempClient, &BaseApiClient::requestFailed, this, &Net_test_VM::onRequestError);

    // 组装对话消息
    QJsonArray messages;
    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userInput;
    messages.append(userMsg);

    // 模型参数
    // ModelConfig config;
    // config.modelName = selectModel;
    // config.stream = true;

    // // 发起请求
    // m_tempClient->sendChatCompletion(messages, config);

}

void Net_test_VM::cancelAllRequest()
{
    if (m_tempClient)
    {
        m_tempClient->cancelAllRequests();
        m_tempClient->deleteLater();
        m_tempClient = nullptr;
    }

    if (m_isLoading)
    {
        m_requestLog = "请求已手动取消";
        m_isLoading = false;
        emit requestLogChanged();
        emit isLoadingChanged();
    }
}

void Net_test_VM::onModelListReady(GlobalConstants::ModelType type, const QStringList &models)
{
    // if (type != m_curModelType) return;

    m_modelList[type]=models;
    m_curModelList = models;
    m_requestLog = QString("模型列表拉取成功，共 %1 个模型").arg(models.size());
    emit modelListChanged();
    emit requestLogChanged();
}

// void Net_test_VM::onModelListFailed(const QString &err)
// {
//     // if (type != m_curModelType) return;

//     // 拉取失败，使用兜底静态模型
//     // m_modelList = ModelFactory::instance().getFallbackModelNames(type);
//     m_requestLog = QString("远端连接失败");
//     emit modelListChanged();
//     emit requestLogChanged();
// }

void Net_test_VM::onStreamChunk(const QString &chunk)
{
    m_receiveContent += chunk;
    emit receiveContentChanged();
}

void Net_test_VM::onRequestFinish(const QString &fullText)
{
    m_isLoading = false;
    m_tempClient->deleteLater();
    m_tempClient = nullptr;

    m_requestLog += "\n对话请求完成";
    emit isLoadingChanged();
    emit requestLogChanged();
}

void Net_test_VM::onRequestError(const QString &err)
{
    m_isLoading = false;
    m_tempClient->deleteLater();
    m_tempClient = nullptr;

    m_requestLog += QString("\n请求异常：%1").arg(err);
    emit isLoadingChanged();
    emit requestLogChanged();
}

// BaseApiClient* Net_test_VM::getClient(GlobalConstants::ModelType ModelType)
// {
//     auto curPtr = m_modelTypeList.value(ModelType);

//     if (!curPtr)
//     {
//         // 为空，新建
//         curPtr = ModelFactory::instance().createClient(m_curModelType);
//         // 存入map缓存
//         m_modelTypeList.insert(ModelType, curPtr);
//     }

//     return curPtr;
// }
