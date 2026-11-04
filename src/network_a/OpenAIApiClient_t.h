// #ifndef OPENAIAPICLIENT_T_H
// #define OPENAIAPICLIENT_T_H

// #include "BaseApiClient_t.h"

// //openAI模型

// class OpenAIApiClient : public BaseApiClient {
//     Q_OBJECT
// public:
//     explicit OpenAIApiClient(QObject* parent = nullptr);

//     //发送聊天完成
//     virtual void sendChatCompletion(const QJsonArray& messages, const ModelConfig& config) override;
//     // //是否支持
//     //virtual bool isSupported(GlobalConstants::ModelType type) override;
//     //获取子模型列表
//     virtual void fetchModelList() override;

// protected:
//     //处理流式数据
//     void handleStreamData(const QByteArray& data) override;
//     //解析模型列表
//     QStringList parseModelList(const QByteArray& data) override;

// private:
//     //构建请求
//     QJsonObject buildRequest(const QJsonArray& messages, const ModelConfig& config);
//     QString m_streamBuffer;
// };
// #endif
