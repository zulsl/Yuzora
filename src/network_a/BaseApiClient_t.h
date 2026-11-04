// #ifndef BASEAPICLIENT_T_H
// #define BASEAPICLIENT_T_H

// #include <QObject>
// #include <QNetworkAccessManager>
// #include <QNetworkRequest>
// #include <QNetworkReply>
// #include <QJsonObject>
// #include <QJsonArray>

// #include "GlobalConstants.h"

// //抽象网络客户端基类


// //模型配置
// struct ModelConfig {
//     QString modelName;              //模型名称
//     int maxContextToken = 4096;     //最大上下文token
//     int maxResponseToken = 1024;    //最大回复token
//     double temperature = 0.7;       //温度(创造力,越低越死板,越高越放飞自我.写作推荐1.0,问答0.3~0.7)
//     double topP = 0.9;              //词汇多样性(0.1:只选最肯定的词,0.9:选概率前 90% 的词,写作推荐0.95)
//     double topK = 40;               //候选词数量(topK=1:永远选概率最高的(最死板),topK=40:从前40个词里随机选(自然),写作推荐60)
//     double repetitionPenalty = 1.1; //重复惩罚(1.0:不惩罚,1.1:轻微惩罚(默认)1.5~2.0:强惩罚.写作推荐1.05)
//     bool stream = true;             //是否流式
//     QVariantMap extraParams;        //额外参数

//     // 连接级参数
//     QString baseUrl;//基础URL
//     QString apiKey;
// };

// class BaseApiClient : public QObject {
//     Q_OBJECT
// public:
//     explicit BaseApiClient(QObject* parent = nullptr);
//     ~BaseApiClient() override;

//     //基类统一
//     //发送聊天
//     //void sendChatCompletion(const QJsonArray& messages,const ModelConfig& config);

// // ======================================
// // 核心接口
// // ======================================
//     //发送聊天补全请求
//     virtual void sendChatCompletion(const QJsonArray& messages,const ModelConfig& config) = 0;
//     //GET获取模型列表
//     virtual void fetchModelList() = 0;
//     //取消全部请求
//     void cancelAllRequests();


//     // //是否支持
//     // virtual bool isSupported(GlobalConstants::ModelType type) = 0;

// // ======================================
// // 通用配置外部接口
// // ======================================
//     //设置Api密钥
//     void setApiKey(const QString& key) { m_apiKey = key; }
//     //设置基本Url
//     void setBaseUrl(const QString& url);
//     //设置代理(未完成)
//     void setProxy(const QString& host, int port);

//     //设置超时时间
//     //void setTimeout(int timeoutMs) { m_timeoutMs = timeoutMs; }

//     // 全局共享网络管理器（所有客户端复用，连接池复用）
//     //static QNetworkAccessManager* globalNetworkManager();

// signals:
//     //收到流块
//     void streamChunkReceived(const QString& chunk);
//     //回复完成
//     void completionFinished(const QString& fullText);
//     //网络请求错误信号
//     void requestFailed(const QString& error);
//     //令牌使用情况已更新(未完成)
//     void tokenUsageUpdated(int promptTokens, int completionTokens,int totalTokens);
//     //拉取模型列表槽,返回模型名列表
//     void modelListFetched(const QStringList& modelNames);

//     // // 拉取失败
//     // void modelListFetchFailed(const QString& error);
//     // 开始请求
//     //void modelListFetchStarted();


// protected:
//     QNetworkAccessManager* m_networkManager;//网络管理器
//     QString m_apiKey;//apikey
//     QString m_defaultBaseUrl;//基础url
//     ModelConfig m_currentConfig;//当前配置
//     QNetworkReply* m_currentReply = nullptr;//当前网络响应
//     QNetworkReply* m_modelListReply = nullptr;  //模型列表请求句柄
//     QString m_fullResponse;//完整响应

//     //构建请求头
//     virtual QString buildRequestUrl(const ModelConfig& config) const = 0;
//     //构建请求体
//     virtual QJsonObject buildRequestBody(
//         const QJsonArray& messages, const ModelConfig& config) const = 0;
//     //处理流数据
//     virtual void handleStreamData(const QByteArray& data) = 0;

//     //处理完成
//     virtual void handleFinished();
//     //处理错误
//     virtual void handleError(QNetworkReply::NetworkError error);

//     //解析模型列表
//     virtual QStringList parseModelList(const QByteArray& data);

//     //解析单帧流式响应JSON，提取文本内容
//     virtual QString parseStreamChunk(const QJsonObject& chunkJson) = 0;
//     //解析完整非流式响应JSON，提取完整文本
//     virtual QString parseCompletionResponse(const QJsonObject& responseJson) = 0;
//     // 子类可选重写：解析Token用量信息
//     virtual void parseTokenUsage(
//         const QJsonObject& responseJson, int& promptTokens, int& completionTokens);


//     // // 工具方法：构建标准Authorization头
//     // QNetworkRequest buildBaseRequest(const QUrl& url);
//     //废弃
//     //解析端点
//     //QString resolveEndpoint(const QString& path);
//     //处理完整数据
//     // virtual void handleFullResponse(const QByteArray& data);
//     //构建基础请求
//     //QNetworkRequest buildBaseRequest(const QString& endpoint);

// // private:
// //     // 全局静态网络管理器
// //     static QNetworkAccessManager* s_networkManager;
// };

// #endif // BASEAPICLIENT_T_H
