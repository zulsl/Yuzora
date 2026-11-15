#ifndef CHATCOMPLETIONRUNNER_H
#define CHATCOMPLETIONRUNNER_H

#include <QObject>
#include <QList>
#include <QString>

#include "ChatMessage.h"

// 跑一次聊天补全请求的出口：ChatService 只负责"什么时候该补全、结果怎么落库"，
// "取哪份 API 配置、用哪个客户端、怎么发请求"统统藏在实现里。
//
// 抽这层的目的：
//   1. ChatService 不再直接引用 ApiConfigService / ModelFactory / BaseApiClient，
//      单测里换一个假实现就能把"发送 → 完成 → 自动改标题"整条链路跑完；
//   2. 以后接本地模型、换请求协议，只动实现，不动会话/消息逻辑。
//
// 约定：同一时间只允许一个进行中的请求；start() 里能立刻判定的错误
// （没配 API、模型不支持）也通过 failed() 报出来，和网络错误走同一条路。
class ChatCompletionRunner : public QObject {
    Q_OBJECT
public:
    explicit ChatCompletionRunner(QObject* parent = nullptr) : QObject(parent) {}
    ~ChatCompletionRunner() override = default;

    virtual void start(const QList<ChatMessage>& messages) = 0;
    virtual void cancel() = 0;
    virtual bool isRunning() const = 0;

signals:
    void chunkReceived(const QString& chunk);  // 流式增量
    void finished(const QString& fullText);    // 完成（流式/非流式都会给完整文本）
    void failed(const QString& error);         // 失败（含"未配置 API"这类前置错误）
};

#endif // CHATCOMPLETIONRUNNER_H
