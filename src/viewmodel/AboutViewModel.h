#ifndef ABOUTVIEWMODEL_H
#define ABOUTVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

// 关于页的更新检查。
//
// 只做“手动检查 GitHub Releases 最新版”，不轮询、不自动下载。网络请求放在
// ViewModel 而不是 QML，是为了让超时、HTTP 状态码和版本比较都能用 C++ 测试覆盖。
// 版本号唯一来源仍是 CMake 的 PROJECT_VERSION（main.cpp 写进 QCoreApplication）。
class AboutViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum Status {
        Idle,
        Checking,
        UpToDate,
        UpdateAvailable,
        NoRelease,
        Failed
    };
    Q_ENUM(Status)

private:
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT FINAL)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged FINAL)
    Q_PROPERTY(bool checking READ checking NOTIFY statusChanged FINAL)
    Q_PROPERTY(bool hasUpdate READ hasUpdate NOTIFY statusChanged FINAL)
    Q_PROPERTY(bool showReleaseButton READ showReleaseButton NOTIFY statusChanged FINAL)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY statusChanged FINAL)
    Q_PROPERTY(QString releaseUrl READ releaseUrl NOTIFY statusChanged FINAL)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged FINAL)
    Q_PROPERTY(QString groupNumber READ groupNumber CONSTANT FINAL)

public:
    // QML 单例工厂：静态实例由 C++ 持有，避免引擎析构时 delete 静态对象。
    static AboutViewModel* create(QQmlEngine*, QJSEngine*)
    {
        static AboutViewModel instance;
        QQmlEngine::setObjectOwnership(&instance, QQmlEngine::CppOwnership);
        return &instance;
    }

    ~AboutViewModel() override;

    QString currentVersion() const;
    Status status() const { return m_status; }
    bool checking() const { return m_status == Checking; }
    bool hasUpdate() const { return m_status == UpdateAvailable; }
    bool showReleaseButton() const;
    QString latestVersion() const { return m_latestVersion; }
    QString releaseUrl() const { return m_releaseUrl; }
    QString statusText() const;
    QString groupNumber() const { return QStringLiteral("767201901"); }

    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void copyGroupNumber();

    // 纯函数：供更新检查和单测共用。候选版本可带前缀 v/V。
    static bool isNewerVersion(const QString& current, const QString& candidate);
    static QString normalizeTag(const QString& tag);

signals:
    void statusChanged();

private:
    explicit AboutViewModel(QObject* parent = nullptr);

    void onReplyFinished();
    void fail();
    void setStatus(Status status);

    QNetworkAccessManager* m_network = nullptr;
    QNetworkReply* m_reply = nullptr;
    QTimer* m_timeout = nullptr;
    Status m_status = Idle;
    QString m_latestVersion;
    QString m_releaseUrl = QStringLiteral("https://github.com/zulsl/Yuzora/releases");
    bool m_timedOut = false;
};

#endif // ABOUTVIEWMODEL_H
