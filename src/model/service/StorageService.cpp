#include "StorageService.h"

#include "AppException.h"
#include "DeviceHelper.h"
#include "Utils.h"

#include <QSettings>
#include <QStandardPaths>

StorageService& StorageService::instance()
{
    static StorageService instance;
    return instance;
}

QString StorageService::dataPath() const
{
    return Utils::getAppDataPath();
}

void StorageService::changeAppDataPath(const QString& path)
{
    if (path.isEmpty()) {
        THROW(FileIOError, "文件路径不能为空");
    }
    DeviceHelper::instance().reSetAppDataPath(path);
}

void StorageService::clearAllData()
{
    DeviceHelper& device = DeviceHelper::instance();
    device.removeDataDir(device.getAppDataPath());
    device.removeDataDir(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

    QSettings settings;
    settings.clear();

    device.restartApplication();
}
