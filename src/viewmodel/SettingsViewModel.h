#ifndef SETTINGSVIEWMODEL_H
#define SETTINGSVIEWMODEL_H

#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>
#include "GlobalConstants.h"

class QJSEngine;
class SettingsService;
class DeviceHelper;
class ExceptionHandler;

/**
 * @brief 系统设置视图模型
 * 职责：外观 / 通用 / 网络 / 存储四组设置的读写转发与变更通知。
 * 本类以 QML 单例形式暴露给 QML（类型名 SettingsViewModel）。
 * 注意：themeMode 的取值沿用 Material 语义（0=Light 1=Dark 2=System），
 * 与 GlobalConstants::ThemeType 的枚举序号并不一致，不要混用。
 */
class SettingsViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // QML 单例工厂（供 QML 使用）：qmltyperegistrar 只有检测到 create() 才会把本类注册成
    // "可实例化的单例"；只依赖默认构造会被标记 isCreatable=false，QML 侧会报
    // ReferenceError: SettingsViewModel is not defined。
    static SettingsViewModel* create(QQmlEngine*, QJSEngine*)
    {
        static SettingsViewModel instance;
        // 生命周期归 C++：静态实例必须由我们销毁。否则引擎析构时会先 delete 一次，
        // 进程退出时静态析构再销毁一次，表现为"关窗后卡死/崩溃"。
        QQmlEngine::setObjectOwnership(&instance, QQmlEngine::CppOwnership);
        return &instance;
    }

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
    SettingsService* m_service = nullptr;
    DeviceHelper* m_deviceHelper = nullptr;
    ExceptionHandler* m_exceptionHandler = nullptr;

    // 构造函数必须 private：QML 单例只有在"无法默认构造"时才会调用 create()。
    // 若保持 public，引擎会自己 new 一个实例，C++ 侧的静态实例就与 QML 拿到的
    // 不是同一个对象（会出现"数据写进 A、界面读的是 B"这类极难定位的问题）。
    explicit SettingsViewModel(QObject* parent = nullptr);
};

#endif // SETTINGSVIEWMODEL_H