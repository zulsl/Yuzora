// #ifndef DEEPSEEKAPICLIENT_H
// #define DEEPSEEKAPICLIENT_H

// #include "BaseApiClient_t.h"

// //DeepSeek模型

// class DeepSeekApiClient : public BaseApiClient {
//     Q_OBJECT
// public:
//     explicit DeepSeekApiClient(QObject* parent = nullptr);

// //核心请求
//     //发送聊天
//     virtual void sendChatCompletion(const QJsonArray& messages, const ModelConfig& config) override;
//     //(拉取)子模型列表
//     virtual void fetchModelList() override;


//     // //是否支持
//     // virtual bool isSupported(GlobalConstants::ModelType type) override;
// protected:
//     //构建请求头
//     virtual QString buildRequestUrl(const ModelConfig& config)const override;
//     //构建请求体
//     virtual QJsonObject buildRequestBody(
//         const QJsonArray& messages, const ModelConfig& config)const override;
//     //处理流式数据
//     virtual void handleStreamData(const QByteArray& data) override;
//     //解析模型列表
//     virtual QStringList parseModelList(const QByteArray& data) override;


// private:
//     //构建请求体
//     QString m_streamBuffer;
// };
// #endif