// #ifndef DEEPSEEKAPICLIENT_H
// #define DEEPSEEKAPICLIENT_H

// #include "BaseApiClient.h"

// //DeepSeek模型

// class DeepSeekApiClient : public BaseApiClient {
//     Q_OBJECT
// public:
//     explicit DeepSeekApiClient(QObject* parent = nullptr);

// //核心请求
//     //发送聊天
//     virtual void sendChatCompletion(const QJsonArray& messages) override;
//     //(拉取)子模型列表
//     virtual void fetchModelList() override;

// protected:
//     //构建请求头
//     virtual QNetworkRequest buildBaseRequestHeader()const override;
//     //构建请求体
//     virtual QJsonObject buildRequestBody(const QJsonArray& messages)const override;
//     //处理流式数据
//     virtual void handleStreamData(const QByteArray& data) override;
//     //处理完成
//     virtual void handleFinished() override;
//     //解析模型列表
//     virtual QStringList parseModelList(const QByteArray& data) override;

//     virtual void processSseBuffer() override;


// };
// #endif