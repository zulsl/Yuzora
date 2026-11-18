#include "AboutViewModel.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QVersionNumber>
#include <QDebug>

namespace {
constexpr auto kReleasesPage = "https://github.com/zulsl/Yuzora/releases";
constexpr auto kLatestReleaseApi = "https://api.github.com/repos/zulsl/Yuzora/releases/latest";
constexpr int kRequestTimeoutMs = 5000;
}

AboutViewModel::AboutViewModel(QObject* parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_timeout(new QTimer(this))
{
    m_timeout->setSingleShot(true);
    m_timeout->setInterval(kRequestTimeoutMs);
    connect(m_timeout, &QTimer::timeout, this, [this] {
        if (!m_reply) return;
        m_timedOut = true;
        m_reply->abort();
    });
}

AboutViewModel::~AboutViewModel()
{
    if (m_reply) {
        disconnect(m_reply, nullptr, this, nullptr);
        m_reply->abort();
        m_reply->deleteLater();
    }
}

QString AboutViewModel::currentVersion() const
{
    const QString version = QCoreApplication::applicationVersion();
    if (!version.isEmpty())
        return version;
#ifdef APP_VERSION
    return QStringLiteral(APP_VERSION);
#else
    return {};
#endif
}

bool AboutViewModel::showReleaseButton() const
{
    return m_status == UpdateAvailable
        || m_status == NoRelease
        || m_status == Failed;
}

QString AboutViewModel::statusText() const
{
    switch (m_status) {
    case Idle:
        return QStringLiteral("尚未检查更新");
    case Checking:
        return QStringLiteral("正在检查更新…");
    case UpToDate:
        return QStringLiteral("已是最新版本");
    case UpdateAvailable:
        return QStringLiteral("发现新版本 v%1").arg(m_latestVersion);
    case NoRelease:
        return QStringLiteral("暂无发布版本");
    case Failed:
        return QStringLiteral("检查失败，请稍后重试");
    }
    return {};
}

QString AboutViewModel::normalizeTag(const QString& tag)
{
    QString normalized = tag.trimmed();
    if (normalized.startsWith(QStringLiteral("refs/tags/")))
        normalized.remove(0, 10);
    if (!normalized.isEmpty()
        && (normalized.front() == QLatin1Char('v')
            || normalized.front() == QLatin1Char('V'))) {
        normalized.remove(0, 1);
    }
    return normalized.trimmed();
}

bool AboutViewModel::isNewerVersion(const QString& current, const QString& candidate)
{
    const QVersionNumber currentVersion = QVersionNumber::fromString(normalizeTag(current));
    const QVersionNumber candidateVersion = QVersionNumber::fromString(normalizeTag(candidate));
    if (currentVersion.isNull() || candidateVersion.isNull())
        return false;
    return candidateVersion > currentVersion;
}

void AboutViewModel::checkForUpdates()
{
    if (m_status == Checking)
        return;

    if (m_reply) {
        disconnect(m_reply, nullptr, this, nullptr);
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    m_latestVersion.clear();
    m_releaseUrl = QString::fromLatin1(kReleasesPage);
    m_timedOut = false;

    QNetworkRequest request(QUrl(QString::fromLatin1(kLatestReleaseApi)));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("Yuzora/%1").arg(currentVersion()));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");

    setStatus(Checking);
    m_reply = m_network->get(request);
    connect(m_reply, &QNetworkReply::finished,
            this, &AboutViewModel::onReplyFinished);
    m_timeout->start();
}

void AboutViewModel::onReplyFinished()
{
    if (!m_reply) return;

    QNetworkReply* reply = m_reply;
    m_reply = nullptr;
    m_timeout->stop();

    const bool timedOut = m_timedOut;
    m_timedOut = false;

    const QNetworkReply::NetworkError networkError = reply->error();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray payload = reply->readAll();
    reply->deleteLater();

    if (timedOut || networkError == QNetworkReply::OperationCanceledError) {
        fail();
        return;
    }

    // 404 = 仓库还没有 Release，不当作网络错误。
    if (httpStatus == 404) {
        setStatus(NoRelease);
        return;
    }
    if (networkError != QNetworkReply::NoError || httpStatus != 200) {
        fail();
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        qWarning() << "[AboutViewModel] GitHub release JSON 解析失败:" << parseError.errorString();
        fail();
        return;
    }

    const QJsonObject release = document.object();
    if (release.value(QStringLiteral("draft")).toBool()
        || release.value(QStringLiteral("prerelease")).toBool()) {
        setStatus(NoRelease);
        return;
    }

    const QString tag = release.value(QStringLiteral("tag_name")).toString();
    const QString normalized = normalizeTag(tag);
    if (normalized.isEmpty()) {
        qWarning() << "[AboutViewModel] GitHub release 缺少 tag_name";
        fail();
        return;
    }

    m_latestVersion = normalized;
    const QString htmlUrl = release.value(QStringLiteral("html_url")).toString();
    if (!htmlUrl.isEmpty())
        m_releaseUrl = htmlUrl;

    setStatus(isNewerVersion(currentVersion(), m_latestVersion)
                  ? UpdateAvailable
                  : UpToDate);
}

void AboutViewModel::fail()
{
    m_latestVersion.clear();
    setStatus(Failed);
}

void AboutViewModel::setStatus(Status status)
{
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged();
}

void AboutViewModel::copyGroupNumber()
{
    QGuiApplication::clipboard()->setText(groupNumber());
}
