#include "SettingsViewModel.h"
#include "SettingsService.h"
#include "StorageService.h"
#include "GlobalConstants.h"
#include "ExceptionHandler.h"
#include <QColor>

SettingsViewModel::SettingsViewModel(QObject* parent)
    : QObject(parent)
    , m_service(&SettingsService::instance())
    , m_exceptionHandler(&ExceptionHandler::instance())
{
}

// ========== 外观设置 ==========
int SettingsViewModel::themeMode() const {
    return m_service->getInt(GlobalConstants::THEME_MODE);
}
void SettingsViewModel::setThemeMode(int mode) {
    if (mode < 0 || mode > 2) return;
    m_service->setValue(GlobalConstants::THEME_MODE, mode);
    emit themeModeChanged();
}

int SettingsViewModel::baseFontSize() const {
    return m_service->getInt(GlobalConstants::FONT_SIZE);
}
void SettingsViewModel::setBaseFontSize(int size) {
    if (size < 10 || size > 32) return;
    m_service->setValue(GlobalConstants::FONT_SIZE, size);
    emit baseFontSizeChanged();
}

QString SettingsViewModel::themeColor() const {
    return m_service->getString(GlobalConstants::THEME_COLOR);
}
void SettingsViewModel::setThemeColor(QString color) {
    if (!QColor(color).isValid()) return;
    m_service->setValue(GlobalConstants::THEME_COLOR, color);
    emit themeColorChanged();
}

// ========== 通用设置 ==========
bool SettingsViewModel::sendWithEnter() const {
    return m_service->getBool(GlobalConstants::SEND_WITH_ENTER);
}
void SettingsViewModel::setSendWithEnter(bool enable) {
    m_service->setValue(GlobalConstants::SEND_WITH_ENTER, enable);
    emit sendWithEnterChanged();
}

bool SettingsViewModel::showTimeStamp() const {
    return m_service->getBool(GlobalConstants::SHOW_TIME_STAMP);
}
void SettingsViewModel::setShowTimeStamp(bool show) {
    m_service->setValue(GlobalConstants::SHOW_TIME_STAMP, show);
    emit showTimeStampChanged();
}

bool SettingsViewModel::autoScroll() const {
    return m_service->getBool(GlobalConstants::AUTO_SCROLL_TO_BOTTOM);
}
void SettingsViewModel::setAutoScroll(bool enable) {
    m_service->setValue(GlobalConstants::AUTO_SCROLL_TO_BOTTOM, enable);
    emit autoScrollChanged();
}

int SettingsViewModel::autoSaveInterval() const {
    return m_service->getInt(GlobalConstants::AUTO_SAVE_INTERVAL);
}
void SettingsViewModel::setAutoSaveInterval(int seconds) {
    if (seconds < 0 || seconds > 3600) return;
    m_service->setValue(GlobalConstants::AUTO_SAVE_INTERVAL, seconds);
    emit autoSaveIntervalChanged();
}

// ========== 网络设置 ==========
bool SettingsViewModel::enableProxy() const {
    return m_service->getBool(GlobalConstants::ENABLE_PROXY);
}
void SettingsViewModel::setEnableProxy(bool enable) {
    m_service->setValue(GlobalConstants::ENABLE_PROXY, enable);
    emit enableProxyChanged();
}

QString SettingsViewModel::proxyType() const {
    return m_service->getString(GlobalConstants::PROXY_TYPE);
}
void SettingsViewModel::setProxyType(const QString& type) {
    m_service->setValue(GlobalConstants::PROXY_TYPE, type);
    emit proxyTypeChanged();
}

QString SettingsViewModel::proxyHost() const {
    return m_service->getString(GlobalConstants::PROXY_HOST);
}
void SettingsViewModel::setProxyHost(const QString& host) {
    m_service->setValue(GlobalConstants::PROXY_HOST, host);
    emit proxyHostChanged();
}

int SettingsViewModel::proxyPort() const {
    return m_service->getInt(GlobalConstants::PROXY_PORT);
}
void SettingsViewModel::setProxyPort(int port) {
    if (port < 1 || port > 65535) return;
    m_service->setValue(GlobalConstants::PROXY_PORT, port);
    emit proxyPortChanged();
}

int SettingsViewModel::requestTimeout() const {
    return m_service->getInt(GlobalConstants::REQUEST_TIMEOUT);
}
void SettingsViewModel::setRequestTimeout(int seconds) {
    if (seconds < 1 || seconds > 600) return;
    m_service->setValue(GlobalConstants::REQUEST_TIMEOUT, seconds);
    emit requestTimeoutChanged();
}

// ========== 存储设置 ==========

QString SettingsViewModel::dataPath() const {
    return StorageService::instance().dataPath();
}

void SettingsViewModel::customAppPath(const QUrl & fileUrl)
{
    const QString path = fileUrl.toLocalFile();
    if (path.isEmpty()) {
        emit m_exceptionHandler->errorOccurred("文件路径不能为空");
        return;
    }

    try {
        StorageService::instance().changeAppDataPath(path);
        emit m_exceptionHandler->tipOccurred("路径修改成功");
    }
    catch (const AppException& e) { m_exceptionHandler->handleException(e); }
    catch (const std::exception& e) { m_exceptionHandler->handleException(e); }
    catch (...) { m_exceptionHandler->handleException(); }
}

void SettingsViewModel::clearAllData()
{
    try {
        StorageService::instance().clearAllData();
    }
    catch (const AppException& e) { m_exceptionHandler->handleException(e); }
    catch (const std::exception& e) { m_exceptionHandler->handleException(e); }
    catch (...) { m_exceptionHandler->handleException(); }
}

QString SettingsViewModel::getUserName() const
{
    QString userName=m_service->getString(GlobalConstants::USER_NAME);
    if(userName.isEmpty()) return "用户名";
    return userName;
}

void SettingsViewModel::setUserName(const QString & newUserName)
{
    try{
        const QString trimmed = newUserName.trimmed();
        if (trimmed.isEmpty()) THROW(OperationError, "用户名不能为空");
        m_service->setValue(GlobalConstants::USER_NAME, trimmed);
        emit userNameChanged();
    }
    catch (const AppException& e) {m_exceptionHandler->handleException(e);}
    catch (const std::exception& e) {m_exceptionHandler->handleException(e);}
    catch (...) {m_exceptionHandler->handleException();}

}


QString SettingsViewModel::getUserAvatar() const
{
    return Utils::avatarPathToUrl(m_service->getString(GlobalConstants::USER_AVATAR));
}
void SettingsViewModel::setUserAvatar(const QUrl& fileUrl)
{
    if (fileUrl.isEmpty()) return;
    try {
        m_service->saveUserAvatar(fileUrl.toLocalFile());
        emit userAvatarChanged();
    }
    catch (const AppException& e) { m_exceptionHandler->handleException(e); }
    catch (const std::exception& e) { m_exceptionHandler->handleException(e); }
    catch (...) { m_exceptionHandler->handleException(); }
}



// ========== 批量操作 ==========
void SettingsViewModel::resetAllSettings() {
    m_service->resetAll();
    // 通知所有属性变更
    emit themeModeChanged();
    emit baseFontSizeChanged();
    emit themeColorChanged();
    emit sendWithEnterChanged();
    emit showTimeStampChanged();
    emit autoScrollChanged();
    emit autoSaveIntervalChanged();
    emit enableProxyChanged();
    emit proxyTypeChanged();
    emit proxyHostChanged();
    emit proxyPortChanged();
    emit requestTimeoutChanged();
    emit autoBackupChanged();
    emit dataPathChanged();
}