#include "NetworkError.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QUrl>

namespace {

// 把任意一段文本压成单行、限长，避免把整段 HTML 或超长 JSON 塞进界面
QString compact(const QString& text, int maxLength)
{
    QString out = text.simplified();
    if (out.size() > maxLength)
        out = out.left(maxLength) + QStringLiteral("…");
    return out;
}

bool looksLikeHtml(const QByteArray& body)
{
    const QByteArray head = body.trimmed().left(32).toLower();
    return head.startsWith("<!doctype")
        || head.startsWith("<html")
        || head.startsWith("<?xml")
        || head.startsWith("<head")
        || head.startsWith("<body");
}

QString bodyPreview(const QByteArray& body)
{
    if (body.isEmpty()) return {};
    return compact(QString::fromUtf8(body.left(400)), 300);
}

// 从业务错误体里挖出"服务端到底说了什么"。
// 兼容 OpenAI/DeepSeek/豆包（{"error":{"message":...}}）、Claude（同左）、
// Gemini（[{"error":{...}}]）、以及一些网关直接回 {"message":...} 或纯文本。
QString providerMessageFrom(const QByteArray& body, bool& looksLikeWebPage)
{
    const QByteArray trimmed = body.trimmed();
    if (trimmed.isEmpty()) return {};

    if (looksLikeHtml(trimmed)) {
        looksLikeWebPage = true;
        return {};
    }

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(trimmed, &parseErr);
    if (parseErr.error == QJsonParseError::NoError) {
        QJsonObject root;
        if (doc.isObject()) {
            root = doc.object();
        } else if (doc.isArray() && !doc.array().isEmpty()) {
            root = doc.array().first().toObject();
        }

        if (!root.isEmpty()) {
            const QJsonValue errorValue = root.value(QStringLiteral("error"));
            if (errorValue.isObject()) {
                const QJsonObject errorObj = errorValue.toObject();
                for (const char* key : {"message", "msg", "detail", "error_description"}) {
                    const QString value = errorObj.value(QLatin1String(key)).toString();
                    if (!value.isEmpty())
                        return compact(value, 400);
                }
            } else if (errorValue.isString()) {
                const QString value = errorValue.toString();
                if (!value.isEmpty())
                    return compact(value, 400);
            }

            for (const char* key : {"message", "msg", "detail", "error_description"}) {
                const QString value = root.value(QLatin1String(key)).toString();
                if (!value.isEmpty())
                    return compact(value, 400);
            }
        }
    }

    // 不是 JSON 也不是网页：可能是网关的纯文本说明，原样截一段
    const QString text = compact(QString::fromUtf8(trimmed), 400);
    if (text.isEmpty()) return {};
    if (looksLikeWebPage) return {};
    return text;
}

void applyWebPageHint(NetworkError::Info& info)
{
    if (info.bodySnippet.isEmpty() || !looksLikeHtml(info.bodySnippet.toUtf8()))
        return;
    info.hint = QStringLiteral("响应是网页而不是 API 数据：确认 baseDomain 指向的是接口而不是网站首页，"
                               "也检查是否被代理/网关拦截。");
}

} // namespace

namespace NetworkError {

QString kindLabel(Kind kind)
{
    switch (kind) {
    case Kind::None:              return QStringLiteral("无错误");
    case Kind::Canceled:          return QStringLiteral("请求已取消");
    case Kind::Offline:           return QStringLiteral("本机网络不可用");
    case Kind::Dns:               return QStringLiteral("域名解析失败");
    case Kind::ConnectionRefused: return QStringLiteral("连接被拒绝");
    case Kind::ConnectionClosed:  return QStringLiteral("连接被中断");
    case Kind::Timeout:           return QStringLiteral("请求超时");
    case Kind::Tls:               return QStringLiteral("HTTPS 证书/TLS 失败");
    case Kind::Proxy:             return QStringLiteral("代理连接失败");
    case Kind::Auth:              return QStringLiteral("鉴权失败");
    case Kind::Permission:        return QStringLiteral("没有访问权限");
    case Kind::RateLimit:         return QStringLiteral("限流或额度用尽");
    case Kind::NotFound:          return QStringLiteral("接口或模型不存在");
    case Kind::PayloadTooLarge:   return QStringLiteral("请求内容过大");
    case Kind::BadRequest:        return QStringLiteral("请求被服务端拒绝");
    case Kind::Server:            return QStringLiteral("服务端错误");
    case Kind::Parse:             return QStringLiteral("响应解析失败");
    case Kind::Empty:             return QStringLiteral("响应为空");
    case Kind::Unknown:           return QStringLiteral("未知网络错误");
    }
    return QStringLiteral("未知网络错误");
}

QString Info::userMessage() const
{
    QString message = summary;
    if (!providerMessage.isEmpty()) {
        // 界面只放得下短消息；完整原文在 detail() 里
        const QString shown = providerMessage.size() > 180
            ? providerMessage.left(180) + QStringLiteral("…")
            : providerMessage;
        message += QStringLiteral("\n服务端：") + shown;
    }
    if (!hint.isEmpty())
        message += QStringLiteral("\n建议：") + hint;
    return message;
}

QString Info::detail() const
{
    QStringList lines;
    lines << QStringLiteral("分类：") + kindLabel(kind);
    if (!endpoint.isEmpty())
        lines << QStringLiteral("请求：") + endpoint;
    if (!host.isEmpty() && host != endpoint)
        lines << QStringLiteral("域名：") + host;
    if (httpStatus > 0)
        lines << QStringLiteral("HTTP：") + QString::number(httpStatus);
    if (!providerMessage.isEmpty())
        lines << QStringLiteral("服务端消息：") + providerMessage;
    if (!rawError.isEmpty())
        lines << QStringLiteral("Qt 错误：") + rawError;
    if (!bodySnippet.isEmpty())
        lines << QStringLiteral("响应片段：") + bodySnippet;
    return lines.join(QLatin1Char('\n'));
}

Info classify(int networkError, int httpStatus, const QString& errorString,
              const QByteArray& body, const QString& endpoint)
{
    Info info;
    info.httpStatus = httpStatus;
    info.endpoint = endpoint;
    info.rawError = compact(errorString, 200);
    info.bodySnippet = bodyPreview(body);

    const QUrl url(endpoint);
    info.host = url.host();

    bool looksLikeWebPage = false;
    info.providerMessage = providerMessageFrom(body, looksLikeWebPage);

    if (httpStatus >= 400) {
        switch (httpStatus) {
        case 400:
        case 422:
            info.kind = Kind::BadRequest;
            info.summary = QStringLiteral("请求被拒绝（HTTP %1）").arg(httpStatus);
            info.hint = QStringLiteral("按服务端提示检查模型名、上下文长度或请求参数；"
                                       "部分服务拒绝空消息或非法角色。");
            break;
        case 401:
            info.kind = Kind::Auth;
            info.summary = QStringLiteral("API Key 无效或未授权（HTTP 401）");
            info.hint = QStringLiteral("检查当前配置里的 API Key 是否填对、是否过期，"
                                       "以及是否选对了服务商/组织。");
            break;
        case 403:
            info.kind = Kind::Permission;
            info.summary = QStringLiteral("没有访问权限（HTTP 403）");
            info.hint = QStringLiteral("Key 有效但无权访问：确认模型是否已开通、"
                                       "账号是否受地区限制，或服务端是否要求额外权限。");
            break;
        case 404:
            info.kind = Kind::NotFound;
            info.summary = QStringLiteral("接口或模型不存在（HTTP 404）");
            info.hint = QStringLiteral("检查 baseDomain 是否包含正确的路径（常见是少了 /v1），"
                                       "以及模型名是否在服务商列表中。");
            break;
        case 408:
            info.kind = Kind::Timeout;
            info.summary = QStringLiteral("服务器等待超时（HTTP 408）");
            info.hint = QStringLiteral("稍后重试；持续出现时检查网络和代理。");
            info.retryable = true;
            break;
        case 413:
            info.kind = Kind::PayloadTooLarge;
            info.summary = QStringLiteral("请求内容过大（HTTP 413）");
            info.hint = QStringLiteral("缩短上下文/提示词，或换一个上下文更大的模型。");
            break;
        case 429:
            info.kind = Kind::RateLimit;
            info.summary = QStringLiteral("请求过于频繁或额度用尽（HTTP 429）");
            info.hint = QStringLiteral("稍等一会儿再试；持续出现时检查账号余额、并发限制或用量。");
            info.retryable = true;
            break;
        default:
            if (httpStatus >= 500) {
                info.kind = Kind::Server;
                info.summary = httpStatus == 502 ? QStringLiteral("网关错误（HTTP 502）")
                             : httpStatus == 503 ? QStringLiteral("服务暂时不可用（HTTP 503）")
                             : httpStatus == 504 ? QStringLiteral("网关超时（HTTP 504）")
                                                 : QStringLiteral("服务端内部错误（HTTP %1）").arg(httpStatus);
                info.hint = QStringLiteral("稍后重试；如果一直出现，检查 API 域名是否正确，"
                                           "或到服务商状态页确认服务是否正常。");
                info.retryable = true;
            } else {
                info.kind = Kind::BadRequest;
                info.summary = QStringLiteral("请求被拒绝（HTTP %1）").arg(httpStatus);
                info.hint = QStringLiteral("按服务端提示检查请求参数。");
            }
            break;
        }

        if (looksLikeWebPage) {
            info.hint = QStringLiteral("响应是网页而不是 API 数据：确认 baseDomain 指向的是接口"
                                       "而不是网站首页，也检查是否被代理/网关拦截。");
        } else {
            applyWebPageHint(info);
        }
        return info;
    }

    // 没有 HTTP 状态码（连接阶段就失败了）；偶尔也有 200 但业务错误的响应
    switch (static_cast<QNetworkReply::NetworkError>(networkError)) {
    case QNetworkReply::NoError:
        break;
    case QNetworkReply::HostNotFoundError:
        info.kind = Kind::Dns;
        info.summary = QStringLiteral("无法解析服务器地址");
        info.hint = QStringLiteral("检查 API 域名是否拼写正确、网络是否正常；"
                                   "用了代理/VPN 的话确认 DNS 能解析该域名。");
        break;
    case QNetworkReply::TimeoutError:
        info.kind = Kind::Timeout;
        info.summary = QStringLiteral("连接或等待响应超时");
        info.hint = QStringLiteral("检查网络、代理或防火墙；服务器繁忙时可稍后重试。");
        info.retryable = true;
        break;
    case QNetworkReply::ConnectionRefusedError:
        info.kind = Kind::ConnectionRefused;
        info.summary = QStringLiteral("服务器拒绝连接");
        info.hint = QStringLiteral("确认域名和端口写对（baseDomain 是否少了/多了路径）、"
                                   "本机代理或本地模型服务是否在运行。");
        break;
    case QNetworkReply::RemoteHostClosedError:
        info.kind = Kind::ConnectionClosed;
        info.summary = QStringLiteral("连接被服务器中断");
        info.hint = QStringLiteral("可能是服务器主动断开或网络不稳定，稍后重试。");
        info.retryable = true;
        break;
    case QNetworkReply::SslHandshakeFailedError:
        info.kind = Kind::Tls;
        info.summary = QStringLiteral("HTTPS 证书校验失败");
        info.hint = QStringLiteral("检查系统时间是否正确；代理/抓包工具是否劫持了 HTTPS；"
                                   "这个域名是否支持 HTTPS。");
        break;
    case QNetworkReply::ProxyConnectionRefusedError:
    case QNetworkReply::ProxyConnectionClosedError:
    case QNetworkReply::ProxyNotFoundError:
    case QNetworkReply::ProxyTimeoutError:
    case QNetworkReply::ProxyAuthenticationRequiredError:
    case QNetworkReply::UnknownProxyError:
        info.kind = Kind::Proxy;
        info.summary = QStringLiteral("代理连接失败");
        info.hint = QStringLiteral("检查系统或应用里的代理设置（地址、端口、账号密码）。");
        break;
    case QNetworkReply::NetworkSessionFailedError:
    case QNetworkReply::TemporaryNetworkFailureError:
        info.kind = Kind::Offline;
        info.summary = QStringLiteral("本机网络不可用");
        info.hint = QStringLiteral("检查网线/Wi-Fi、系统代理和 VPN 是否正常。");
        info.retryable = true;
        break;
    case QNetworkReply::OperationCanceledError:
        info.kind = Kind::Canceled;
        info.summary = QStringLiteral("请求已取消");
        info.hint.clear();
        break;
    case QNetworkReply::ProtocolUnknownError:
    case QNetworkReply::ProtocolInvalidOperationError:
    case QNetworkReply::ProtocolFailure:
        info.kind = Kind::BadRequest;
        info.summary = QStringLiteral("服务器返回了无法识别的协议响应");
        info.hint = QStringLiteral("baseDomain 可能指向了网页或非 API 地址，检查接口路径。");
        break;
    case QNetworkReply::InternalServerError:
    case QNetworkReply::OperationNotImplementedError:
    case QNetworkReply::ServiceUnavailableError:
        info.kind = Kind::Server;
        info.summary = QStringLiteral("服务端错误");
        info.hint = QStringLiteral("稍后重试；持续出现请检查 API 域名或服务商状态。");
        info.retryable = true;
        break;
    case QNetworkReply::ContentAccessDenied:
        info.kind = Kind::Auth;
        info.summary = QStringLiteral("访问被拒绝");
        info.hint = QStringLiteral("检查 API Key 和接口权限。");
        break;
    case QNetworkReply::ContentNotFoundError:
        info.kind = Kind::NotFound;
        info.summary = QStringLiteral("接口不存在");
        info.hint = QStringLiteral("检查 baseDomain 和路径是否正确。");
        break;
    default: {
        const QString lowered = info.rawError.toLower();
        if (lowered.contains(QStringLiteral("ssl")) || lowered.contains(QStringLiteral("tls"))
            || lowered.contains(QStringLiteral("handshake"))) {
            info.kind = Kind::Tls;
            info.summary = QStringLiteral("HTTPS 证书校验失败");
            info.hint = QStringLiteral("检查系统时间、代理/抓包工具和域名证书。");
        } else if (lowered.contains(QStringLiteral("proxy"))) {
            info.kind = Kind::Proxy;
            info.summary = QStringLiteral("代理连接失败");
            info.hint = QStringLiteral("检查系统或应用里的代理设置。");
        } else {
            info.kind = Kind::Unknown;
            info.summary = QStringLiteral("网络请求失败");
            info.hint = QStringLiteral("检查网络和 API 配置；详情里的原始错误可以帮你定位。");
        }
        break;
    }
    }

    if (info.kind == Kind::Unknown && !info.providerMessage.isEmpty()) {
        info.kind = Kind::BadRequest;
        info.summary = QStringLiteral("服务返回错误");
        info.hint = QStringLiteral("按服务端提示检查模型名、请求参数或账号额度。");
    }

    if (info.kind == Kind::Unknown && info.rawError.isEmpty() && body.isEmpty()) {
        info.kind = Kind::Empty;
        info.summary = QStringLiteral("接口返回了空内容");
        info.hint = QStringLiteral("连接正常但没有数据：检查模型是否可用、是否被内容过滤。");
    }

    applyWebPageHint(info);
    return info;
}

Info fromReply(QNetworkReply* reply, const QByteArray& body)
{
    Info info;
    if (!reply) {
        info.kind = Kind::Unknown;
        info.summary = QStringLiteral("请求失败");
        info.hint = QStringLiteral("没有拿到响应对象，请查看日志。");
        return info;
    }

    const QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    const int httpStatus = status.isValid() ? status.toInt() : 0;
    const QString endpoint = reply->url().toString(QUrl::RemoveQuery);

    return classify(static_cast<int>(reply->error()), httpStatus,
                    reply->errorString(), body, endpoint);
}

Info parseFailure(const QString& parseError, const QByteArray& body,
                  const QString& endpoint, int httpStatus)
{
    Info info;
    info.kind = Kind::Parse;
    info.httpStatus = httpStatus;
    info.endpoint = endpoint;
    info.host = QUrl(endpoint).host();
    info.rawError = compact(parseError, 200);
    info.bodySnippet = bodyPreview(body);
    info.summary = QStringLiteral("响应解析失败：返回的不是有效 JSON");
    info.hint = QStringLiteral("可能是代理/网关返回了网页，或 baseDomain 指向了非 API 地址；"
                               "也确认接口类型（OpenAI/Claude/Gemini）和地址是否匹配。");
    applyWebPageHint(info);
    return info;
}

Info emptyResponse(const QString& endpoint, const QString& summary, const QString& hint)
{
    Info info;
    info.kind = Kind::Empty;
    info.endpoint = endpoint;
    info.host = QUrl(endpoint).host();
    info.summary = summary.isEmpty() ? QStringLiteral("接口返回了空内容") : summary;
    info.hint = hint.isEmpty()
        ? QStringLiteral("连接正常但没有回复文本：检查模型是否可用、是否被内容过滤，"
                         "或换一个模型试试。")
        : hint;
    return info;
}

} // namespace NetworkError
