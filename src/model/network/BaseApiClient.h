#ifndef BASEAPICLIENT_H
#define BASEAPICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>

#include "ApiConfigEntity.h"
#include "CompletionPresetEntity.h"
#include "GlobalConstants.h"
#include "ChatMessage.h"

/**
 * @brief 抽象网络客户端基类
 */
class BaseApiClient : public QObject {
    Q_OBJECT
public:
    explicit BaseApiClient(QObject* parent = nullptr);
    ~BaseApiClient() override;


// ======================================
// 核心接口
// ======================================
    //发送聊天补全请求
    virtual void sendChatCompletion(const QList<ChatMessage>& messages);
    //GET获取模型列表
    virtual void fetchModelList();
    //取消全部请求
    void cancelAllRequests();


// ======================================
// 通用配置外部接口
// ======================================
    //加载配置
    void loadConfig(const ApiConfigEntity& config);
    //加载预设
    void loadPreset(const CompletionPresetEntity& preset);
    //设置代理(未完成)
    void setProxy(const QString& host, int port);


signals:
//==========公有信号==========
    void streamChunkReceived(const QString& chunk);// 流式模式下，每收到一个数据块信号
    void completionFinished(const QString& fullText);// 非流式模式下，收到完整响应信号
    // message：给用户看的结论 + 建议；detail：分类、地址、状态码、原始错误、响应片段
    void requestFailed(const QString& message, const QString& detail);// 请求错误信号
    void cancelled();// 请求被用户取消
    void tokenUsageUpdated(int promptTokens, int completionTokens,int totalTokens);//令牌使用情况已更新(未完成)
    void modelListFetched(const QStringList& modelNames);//拉取模型列表槽,返回模型名列表

protected:
// ========== 子类必须实现的格式适配接口 ==========
    //构建通用请求头
    virtual QNetworkRequest buildRequestHeader() const = 0;
    //构建请求体
    virtual QJsonObject buildRequestBody(const QList<ChatMessage>& messages) const = 0;


// ========== 可重写函数（默认提供 OpenAI 兼容实现） ==========
    //解析单帧流式响应JSON，提取文本内容
    virtual QString parseStreamChunk(const QJsonObject& chunkJson) const;
    //解析完整非流式响应JSON，提取完整文本
    virtual QString parseCompletionResponse(const QJsonObject& responseJson) const;
    //解析Token用量信息（默认不处理）
    virtual void parseTokenUsage(const QJsonObject& responseJson, int& promptTokens,
        int& completionTokens, int& totalTokens) const;
    //解析模型列表
    virtual QStringList parseModelList(const QByteArray& data) const;

    ApiConfigEntity m_apiConfig;                        //完整API配置
    CompletionPresetEntity m_preset;                    //预设

private slots:
    // 网络事件处理
    void onReadyRead();//可读
    void onFinished();//已完成

private:
    void processSseBuffer();//处理sse


    QNetworkAccessManager* m_networkManager = nullptr;  //网络管理器
    QNetworkReply* m_currentReply = nullptr;            //当前请求响应
    QByteArray m_streamBuffer;                          //SSE数据缓冲区
    QString m_fullResponse;                             //累积的完整响应

    bool m_isStreaming = false;                // 当前请求是否为流式
    bool m_finishedHandled = false;            // 防止重复处理finished

};

#endif // BASEAPICLIENT_H
