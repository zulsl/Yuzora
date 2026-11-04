#ifndef OPENAICOMPATiBLEAPICLIENT_H
#define OPENAICOMPATiBLEAPICLIENT_H

#include "BaseApiClient.h"

//openAi兼容模型

class OpenAICompatibleApiClient : public BaseApiClient {
    Q_OBJECT
public:
    explicit OpenAICompatibleApiClient(QObject* parent = nullptr);

protected:
    //构建请求头
    virtual QNetworkRequest buildRequestHeader()const override;
    //构建请求体
    virtual QJsonObject buildRequestBody(const QList<ChatMessage>& messages)const override;


    // 如有特殊需求可重写解析函数，但基类默认已兼容 OpenAI，故可不重写
    // QString parseStreamChunk(const QJsonObject&) const override;
    // QString parseCompletionResponse(const QJsonObject&) const override;
    // QStringList parseModelList(const QByteArray&) const override;


};
#endif