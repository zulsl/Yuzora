#ifndef NETWORKERROR_H
#define NETWORKERROR_H

#include <QByteArray>
#include <QString>

class QNetworkReply;

// 网络错误的"翻译层"：把 QNetworkReply 的错误码 / HTTP 状态码 / 服务端响应体
// 归类成一句人能看懂的结论 + 一条可执行的建议 + 一段技术细节。
//
// 之所以单独抽出来：
//   1. 以前界面上直接显示 Qt 的 errorString（"Connection refused"）或
//      "HTTP 401：xxx"，用户既不知道该改什么，也分不清是网络、Key 还是模型名的问题；
//   2. 分类逻辑是纯函数，可以脱离真实网络做单测（见 tests/NetworkErrorTest.cpp）。
namespace NetworkError {

enum class Kind {
    None,
    Canceled,
    Offline,              // 本机网络不可用
    Dns,                  // 域名解析失败
    ConnectionRefused,    // 目标拒绝连接
    ConnectionClosed,     // 连接被中断
    Timeout,              // 超时
    Tls,                  // 证书/TLS
    Proxy,                // 代理
    Auth,                 // 401 / Key 无效
    Permission,           // 403 / 无权限
    RateLimit,            // 429 / 限流、额度
    NotFound,             // 404 / 接口或模型不存在
    PayloadTooLarge,      // 413
    BadRequest,           // 其它 4xx / 参数错误
    Server,               // 5xx
    Parse,                // 200 但不是合法 JSON
    Empty,                // 200 但内容为空
    Unknown
};

struct Info {
    Kind kind = Kind::Unknown;
    int httpStatus = 0;         // 0 表示没拿到 HTTP 状态（连接阶段的错误）
    QString host;               // 从 endpoint 里解析出的域名，便于快速判断"发到哪了"
    QString endpoint;           // 完整请求地址
    QString summary;            // 一句话结论
    QString hint;               // 建议用户做什么
    QString providerMessage;    // 服务端/网关返回的原始错误消息
    QString rawError;           // QNetworkReply::errorString() 原文
    QString bodySnippet;        // 响应体开头（截断），HTML/网关页一眼能看出来
    bool retryable = false;     // 稍后重试是否可能成功

    // 给界面用：结论 + 建议（两行以内）
    QString userMessage() const;
    // 给日志/详情面板用：分类、地址、状态码、原始错误、响应片段
    QString detail() const;
};

// networkError 传 QNetworkReply::NetworkError 的整数值（避免头文件依赖 QtNetwork）。
Info classify(int networkError, int httpStatus, const QString& errorString,
              const QByteArray& body, const QString& endpoint);

Info fromReply(QNetworkReply* reply, const QByteArray& body);

Info parseFailure(const QString& parseError, const QByteArray& body,
                  const QString& endpoint, int httpStatus = 0);

Info emptyResponse(const QString& endpoint, const QString& summary = QString(),
                   const QString& hint = QString());

QString kindLabel(Kind kind);

} // namespace NetworkError

#endif // NETWORKERROR_H
