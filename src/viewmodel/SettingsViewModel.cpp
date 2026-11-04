#include "SettingsViewModel.h"
#include "SettingsService.h"
#include "GlobalConstants.h"
#include "ExceptionHandler.h"
#include "DeviceHelper.h"

SettingsViewModel::SettingsViewModel(QObject* parent)
    : QObject(parent)
{
}

// ========== 外观设置 ==========
int SettingsViewModel::themeMode() const {
    return SettingsService::instance().getInt(GlobalConstants::THEME_MODE);
}
void SettingsViewModel::setThemeMode(int mode) {
    SettingsService::instance().setValue(GlobalConstants::THEME_MODE, mode);
    emit themeModeChanged();
}

int SettingsViewModel::baseFontSize() const {
    return SettingsService::instance().getInt(GlobalConstants::FONT_SIZE);
}
void SettingsViewModel::setBaseFontSize(int size) {
    SettingsService::instance().setValue(GlobalConstants::FONT_SIZE, size);
    emit baseFontSizeChanged();
}

QString SettingsViewModel::themeColor() const {
    return SettingsService::instance().getString(GlobalConstants::THEME_COLOR);
}
void SettingsViewModel::setThemeColor(QString color) {
    SettingsService::instance().setValue(GlobalConstants::THEME_COLOR, color);
    emit themeColorChanged();
}

// ========== 通用设置 ==========
bool SettingsViewModel::sendWithEnter() const {
    return SettingsService::instance().getBool(GlobalConstants::SEND_WITH_ENTER);
}
void SettingsViewModel::setSendWithEnter(bool enable) {
    SettingsService::instance().setValue(GlobalConstants::SEND_WITH_ENTER, enable);
    emit sendWithEnterChanged();
}

bool SettingsViewModel::showTimeStamp() const {
    return SettingsService::instance().getBool(GlobalConstants::SHOW_TIME_STAMP);
}
void SettingsViewModel::setShowTimeStamp(bool show) {
    SettingsService::instance().setValue(GlobalConstants::SHOW_TIME_STAMP, show);
    emit showTimeStampChanged();
}

bool SettingsViewModel::autoScroll() const {
    return SettingsService::instance().getBool(GlobalConstants::AUTO_SCROLL_TO_BOTTOM);
}
void SettingsViewModel::setAutoScroll(bool enable) {
    SettingsService::instance().setValue(GlobalConstants::AUTO_SCROLL_TO_BOTTOM, enable);
    emit autoScrollChanged();
}

int SettingsViewModel::autoSaveInterval() const {
    return SettingsService::instance().getInt(GlobalConstants::AUTO_SAVE_INTERVAL);
}
void SettingsViewModel::setAutoSaveInterval(int seconds) {
    SettingsService::instance().setValue(GlobalConstants::AUTO_SAVE_INTERVAL, seconds);
    emit autoSaveIntervalChanged();
}

// ========== 网络设置 ==========
bool SettingsViewModel::enableProxy() const {
    return SettingsService::instance().getBool(GlobalConstants::ENABLE_PROXY);
}
void SettingsViewModel::setEnableProxy(bool enable) {
    SettingsService::instance().setValue(GlobalConstants::ENABLE_PROXY, enable);
    emit enableProxyChanged();
}

QString SettingsViewModel::proxyType() const {
    return SettingsService::instance().getString(GlobalConstants::PROXY_TYPE);
}
void SettingsViewModel::setProxyType(const QString& type) {
    SettingsService::instance().setValue(GlobalConstants::PROXY_TYPE, type);
    emit proxyTypeChanged();
}

QString SettingsViewModel::proxyHost() const {
    return SettingsService::instance().getString(GlobalConstants::PROXY_HOST);
}
void SettingsViewModel::setProxyHost(const QString& host) {
    SettingsService::instance().setValue(GlobalConstants::PROXY_HOST, host);
    emit proxyHostChanged();
}

int SettingsViewModel::proxyPort() const {
    return SettingsService::instance().getInt(GlobalConstants::PROXY_PORT);
}
void SettingsViewModel::setProxyPort(int port) {
    SettingsService::instance().setValue(GlobalConstants::PROXY_PORT, port);
    emit proxyPortChanged();
}

int SettingsViewModel::requestTimeout() const {
    return SettingsService::instance().getInt(GlobalConstants::REQUEST_TIMEOUT);
}
void SettingsViewModel::setRequestTimeout(int seconds) {
    SettingsService::instance().setValue(GlobalConstants::REQUEST_TIMEOUT, seconds);
    emit requestTimeoutChanged();
}

// ========== 存储设置 ==========

QString SettingsViewModel::dataPath() const {
    return Utils::getAppDataPath();
}

void SettingsViewModel::selectDataPath(const QString& path) {
    if (path.isEmpty()) return;
    SettingsService::instance().setValue(GlobalConstants::DATA_STORAGE_PATH, path);
    emit dataPathChanged();
}

void SettingsViewModel::customAppPath(const QUrl & fileUrl)
{
    QString path=fileUrl.toLocalFile();
    if (path.isEmpty()) {
        emit ExceptionHandler::instance().errorOccurred("文件路径不能为空");
        return;
    }

    try{
        DeviceHelper::instance().reSetAppDataPath(path);
        emit ExceptionHandler::instance().tipOccurred("路径修改成功");
    }
    catch (const AppException& e) {ExceptionHandler::instance().handleException(e);}
    catch (const std::exception& e) {ExceptionHandler::instance().handleException(e);}
    catch (...) {ExceptionHandler::instance().handleException();}

}

void SettingsViewModel::clearAllData()
{
    DeviceHelper::instance().removeDataDir(DeviceHelper::instance().getAppDataPath());
    DeviceHelper::instance().removeDataDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QSettings settings;
    settings.clear();// 删除所有键
    DeviceHelper::instance().restartApplication();
}

QString SettingsViewModel::getUserName() const
{
    QString userName=SettingsService::instance().getString(GlobalConstants::USER_NAME);
    if(userName.isEmpty()) return "用户名";
    return userName;
}

void SettingsViewModel::setUserName(const QString & newUserName)
{
    try{
        if (newUserName.isEmpty()) THROW(OperationError, "用户名不能为空");
        SettingsService::instance().setValue(GlobalConstants::USER_NAME, newUserName);
        emit userNameChanged();
    }
    catch (const AppException& e) {ExceptionHandler::instance().handleException(e);}
    catch (const std::exception& e) {ExceptionHandler::instance().handleException(e);}
    catch (...) {ExceptionHandler::instance().handleException();}

}


QString SettingsViewModel::getUserAvatar() const
{
    return Utils::avatarPathToUrl(SettingsService::instance().getString(GlobalConstants::USER_AVATAR));
}
void SettingsViewModel::setUserAvatar(const QUrl& fileUrl)
{
    if (fileUrl.isEmpty()) return;
    try{
        QString filePath = fileUrl.toLocalFile();
        QImage image(filePath);
        if (image.isNull()) {
            THROW(FileIOError, QString("无法读取图片：%1").arg(filePath));
        }
        QString fileName = Utils::generateUuid() + ".png";
        QString fullPath = QDir(Utils::getAvatarDirPath()).filePath(fileName);
        if (!image.save(fullPath, "PNG")) {
            THROW(FileIOError, "头像保存失败：无法写入文件");
        }
        SettingsService::instance().setValue(GlobalConstants::USER_AVATAR, fileName);
        emit userAvatarChanged();
    }
    catch (const AppException& e) {ExceptionHandler::instance().handleException(e);}
    catch (const std::exception& e) {ExceptionHandler::instance().handleException(e);}
    catch (...) {ExceptionHandler::instance().handleException();}
}



// ========== 批量操作 ==========
void SettingsViewModel::resetAllSettings() {
    SettingsService::instance().resetAll();
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