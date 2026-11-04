#ifndef SETTINGSVIEWMODEL_H
#define SETTINGSVIEWMODEL_H

#include <QObject>
#include "GlobalConstants.h"

class SettingsViewModel : public QObject {
    Q_OBJECT

//---主界面设置---
    Q_PROPERTY(QString userAvatar READ getUserAvatar WRITE setUserAvatar NOTIFY userAvatarChanged FINAL)
    Q_PROPERTY(QString userName READ getUserName WRITE setUserName NOTIFY userNameChanged FINAL)

//---系统设置---
    // 外观设置
    Q_PROPERTY(int themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged)
    Q_PROPERTY(int baseFontSize READ baseFontSize WRITE setBaseFontSize NOTIFY baseFontSizeChanged)
    Q_PROPERTY(QString themeColor READ themeColor WRITE setThemeColor NOTIFY themeColorChanged)
    // 通用设置
    Q_PROPERTY(bool sendWithEnter READ sendWithEnter WRITE setSendWithEnter NOTIFY sendWithEnterChanged)
    Q_PROPERTY(bool showTimeStamp READ showTimeStamp WRITE setShowTimeStamp NOTIFY showTimeStampChanged)
    Q_PROPERTY(bool autoScroll READ autoScroll WRITE setAutoScroll NOTIFY autoScrollChanged)
    Q_PROPERTY(int autoSaveInterval READ autoSaveInterval WRITE setAutoSaveInterval NOTIFY autoSaveIntervalChanged)
    // 网络设置
    Q_PROPERTY(bool enableProxy READ enableProxy WRITE setEnableProxy NOTIFY enableProxyChanged)
    Q_PROPERTY(QString proxyType READ proxyType WRITE setProxyType NOTIFY proxyTypeChanged)
    Q_PROPERTY(QString proxyHost READ proxyHost WRITE setProxyHost NOTIFY proxyHostChanged)
    Q_PROPERTY(int proxyPort READ proxyPort WRITE setProxyPort NOTIFY proxyPortChanged)
    Q_PROPERTY(int requestTimeout READ requestTimeout WRITE setRequestTimeout NOTIFY requestTimeoutChanged)
    // 存储设置
    Q_PROPERTY(QString dataPath READ dataPath NOTIFY dataPathChanged)

public:
    explicit SettingsViewModel(QObject* parent = nullptr);
    ~SettingsViewModel() override = default;

    // 属性读写方法
    QString getUserAvatar() const;
    void setUserAvatar(const QUrl& fileUrl);
    QString getUserName() const;
    void setUserName(const QString &newUserName);


    int themeMode() const;
    void setThemeMode(int mode);
    int baseFontSize() const;
    void setBaseFontSize(int size);
    QString themeColor() const;
    void setThemeColor(QString color);

    bool sendWithEnter() const;
    void setSendWithEnter(bool enable);
    bool showTimeStamp() const;
    void setShowTimeStamp(bool show);
    bool autoScroll() const;
    void setAutoScroll(bool enable);
    int autoSaveInterval() const;
    void setAutoSaveInterval(int seconds);

    bool enableProxy() const;
    void setEnableProxy(bool enable);
    QString proxyType() const;
    void setProxyType(const QString& type);
    QString proxyHost() const;
    void setProxyHost(const QString& host);
    int proxyPort() const;
    void setProxyPort(int port);
    int requestTimeout() const;
    void setRequestTimeout(int seconds);

    QString dataPath() const;

    // QML可调用操作
    Q_INVOKABLE void resetAllSettings();
    Q_INVOKABLE void selectDataPath(const QString& path);
    Q_INVOKABLE void customAppPath(const QUrl& fileUrl);
    Q_INVOKABLE void clearAllData();

signals:
    void themeModeChanged();
    void baseFontSizeChanged();
    void themeColorChanged();
    void sendWithEnterChanged();
    void showTimeStampChanged();
    void autoScrollChanged();
    void autoSaveIntervalChanged();
    void enableProxyChanged();
    void proxyTypeChanged();
    void proxyHostChanged();
    void proxyPortChanged();
    void requestTimeoutChanged();
    void autoBackupChanged();
    void dataPathChanged();
    void userAvatarChanged();
    void userNameChanged();

private:
};

#endif // SETTINGSVIEWMODEL_H