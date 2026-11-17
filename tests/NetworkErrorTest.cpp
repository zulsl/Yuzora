#include <QtTest>
#include <QNetworkReply>
#include <QString>

#include "NetworkError.h"

// 网络错误分类的回归测试：不碰真实网络，只验证「错误码 + HTTP 状态 + 响应体」
// 会被翻译成什么结论/建议，以及技术细节里有没有关键线索。

namespace {

const QString kEndpoint = QStringLiteral("https://api.example.com/v1/chat/completions");

NetworkError::Info makeInfo(QNetworkReply::NetworkError error, int status,
                            const QByteArray& body, const QString& errorString = QString())
{
    return NetworkError::classify(static_cast<int>(error), status, errorString, body, kEndpoint);
}

} // namespace

class NetworkErrorTest : public QObject {
    Q_OBJECT

private slots:
    void dnsFailure();
    void connectionRefused();
    void timeoutIsRetryable();
    void tlsHandshake();
    void canceledHasNoHint();

    void unauthorizedExtractsProviderMessage();
    void forbiddenKeepsServerMessage();
    void rateLimitIsRetryable();
    void notFoundMentionsPath();
    void serverErrorWithHtmlBody();
    void plainTextErrorBody();

    void businessErrorOnHttp200();
    void arrayStyleErrorBody();

    void parseFailurePointsAtHtml();
    void emptyResponse();
    void detailContainsEndpointAndStatus();
};

void NetworkErrorTest::dnsFailure()
{
    const NetworkError::Info info = makeInfo(QNetworkReply::HostNotFoundError, 0, {},
                                             QStringLiteral("Host not found"));
    QVERIFY(info.kind == NetworkError::Kind::Dns);
    QCOMPARE(info.host, QStringLiteral("api.example.com"));
    QVERIFY(info.userMessage().contains(QStringLiteral("域名")));
    QVERIFY(info.detail().contains(QStringLiteral("请求：https://api.example.com/v1/chat/completions")));
}

void NetworkErrorTest::connectionRefused()
{
    const NetworkError::Info info = makeInfo(QNetworkReply::ConnectionRefusedError, 0, {},
                                             QStringLiteral("Connection refused"));
    QVERIFY(info.kind == NetworkError::Kind::ConnectionRefused);
    QVERIFY(info.userMessage().contains(QStringLiteral("拒绝连接")));
    QVERIFY(info.userMessage().contains(QStringLiteral("建议：")));
}

void NetworkErrorTest::timeoutIsRetryable()
{
    const NetworkError::Info info = makeInfo(QNetworkReply::TimeoutError, 0, {},
                                             QStringLiteral("Operation timed out"));
    QVERIFY(info.kind == NetworkError::Kind::Timeout);
    QVERIFY(info.retryable);
    QVERIFY(info.userMessage().contains(QStringLiteral("超时")));
}

void NetworkErrorTest::tlsHandshake()
{
    // Qt 有时只报 UnknownNetworkError，错误文本里才有 SSL 线索
    const NetworkError::Info info = makeInfo(QNetworkReply::UnknownNetworkError, 0,
                                             {}, QStringLiteral("SSL handshake failed"));
    QVERIFY(info.kind == NetworkError::Kind::Tls);
    QVERIFY(info.userMessage().contains(QStringLiteral("证书")));
}

void NetworkErrorTest::canceledHasNoHint()
{
    const NetworkError::Info info = makeInfo(QNetworkReply::OperationCanceledError, 0, {});
    QVERIFY(info.kind == NetworkError::Kind::Canceled);
    QVERIFY(info.hint.isEmpty());
}

void NetworkErrorTest::unauthorizedExtractsProviderMessage()
{
    const QByteArray body = R"({"error":{"message":"Incorrect API key provided","type":"invalid_request_error"}})";
    const NetworkError::Info info = makeInfo(QNetworkReply::ContentAccessDenied, 401, body);

    QVERIFY(info.kind == NetworkError::Kind::Auth);
    QCOMPARE(info.providerMessage, QStringLiteral("Incorrect API key provided"));
    QVERIFY(info.userMessage().contains(QStringLiteral("401")));
    QVERIFY(info.userMessage().contains(QStringLiteral("Incorrect API key provided")));
    QVERIFY(info.detail().contains(QStringLiteral("HTTP：401")));
}

void NetworkErrorTest::forbiddenKeepsServerMessage()
{
    const QByteArray body = R"({"error":{"message":"country not supported"}})";
    const NetworkError::Info info = makeInfo(QNetworkReply::ContentAccessDenied, 403, body);

    QVERIFY(info.kind == NetworkError::Kind::Permission);
    QCOMPARE(info.providerMessage, QStringLiteral("country not supported"));
    QVERIFY(info.userMessage().contains(QStringLiteral("country not supported")));
    QVERIFY(info.userMessage().contains(QStringLiteral("建议：")));
}

void NetworkErrorTest::rateLimitIsRetryable()
{
    const QByteArray body = R"({"error":{"message":"Rate limit reached"}})";
    const NetworkError::Info info = makeInfo(QNetworkReply::ContentAccessDenied, 429, body);

    QVERIFY(info.kind == NetworkError::Kind::RateLimit);
    QVERIFY(info.retryable);
    QVERIFY(info.userMessage().contains(QStringLiteral("429")));
}

void NetworkErrorTest::notFoundMentionsPath()
{
    const NetworkError::Info info = makeInfo(QNetworkReply::ContentNotFoundError, 404, {});
    QVERIFY(info.kind == NetworkError::Kind::NotFound);
    QVERIFY(info.hint.contains(QStringLiteral("/v1")));
}

void NetworkErrorTest::serverErrorWithHtmlBody()
{
    const QByteArray body = "<!DOCTYPE html>\n<html><body>502 Bad Gateway</body></html>";
    const NetworkError::Info info = makeInfo(QNetworkReply::InternalServerError, 500, body);

    QVERIFY(info.kind == NetworkError::Kind::Server);
    QVERIFY(info.retryable);
    // 网关/代理返回网页时要明确提示"这不是 API 数据"
    QVERIFY(info.hint.contains(QStringLiteral("网页")));
    QVERIFY(info.bodySnippet.startsWith(QStringLiteral("<!DOCTYPE html>")));
}

void NetworkErrorTest::plainTextErrorBody()
{
    const QByteArray body = R"({"message":"bad parameter"})";
    const NetworkError::Info info = makeInfo(QNetworkReply::ProtocolInvalidOperationError, 400, body);

    QVERIFY(info.kind == NetworkError::Kind::BadRequest);
    QCOMPARE(info.providerMessage, QStringLiteral("bad parameter"));
}

void NetworkErrorTest::businessErrorOnHttp200()
{
    const QByteArray body = R"({"error":{"message":"model gpt-5 not found"}})";
    const NetworkError::Info info = makeInfo(QNetworkReply::NoError, 200, body);

    QVERIFY(info.kind == NetworkError::Kind::BadRequest);
    QCOMPARE(info.summary, QStringLiteral("服务返回错误"));
    QVERIFY(info.userMessage().contains(QStringLiteral("model gpt-5 not found")));
}

void NetworkErrorTest::arrayStyleErrorBody()
{
    // Gemini 的错误体是数组：[{"error":{"code":400,"message":"API key not valid"}}]
    const QByteArray body = R"([{"error":{"code":400,"message":"API key not valid"}}])";
    const NetworkError::Info info = makeInfo(QNetworkReply::ProtocolInvalidOperationError, 400, body);

    QVERIFY(info.kind == NetworkError::Kind::BadRequest);
    QCOMPARE(info.providerMessage, QStringLiteral("API key not valid"));
}

void NetworkErrorTest::parseFailurePointsAtHtml()
{
    const NetworkError::Info info = NetworkError::parseFailure(
        QStringLiteral("illegal value"), "<html>login</html>", kEndpoint);

    QVERIFY(info.kind == NetworkError::Kind::Parse);
    QVERIFY(info.userMessage().contains(QStringLiteral("JSON")));
    QVERIFY(info.hint.contains(QStringLiteral("网页")));
    QVERIFY(info.detail().contains(QStringLiteral("分类：响应解析失败")));
}

void NetworkErrorTest::emptyResponse()
{
    const NetworkError::Info info = NetworkError::emptyResponse(kEndpoint);
    QVERIFY(info.kind == NetworkError::Kind::Empty);
    QVERIFY(info.userMessage().contains(QStringLiteral("空内容")));
}

void NetworkErrorTest::detailContainsEndpointAndStatus()
{
    const NetworkError::Info info = makeInfo(QNetworkReply::ContentAccessDenied, 401,
                                             "{\"error\":{\"message\":\"nope\"}}");
    const QString detail = info.detail();
    QVERIFY(detail.contains(QStringLiteral("请求：https://api.example.com/v1/chat/completions")));
    QVERIFY(detail.contains(QStringLiteral("HTTP：401")));
    QVERIFY(detail.contains(QStringLiteral("响应片段：")));
}

QTEST_MAIN(NetworkErrorTest)
#include "NetworkErrorTest.moc"
