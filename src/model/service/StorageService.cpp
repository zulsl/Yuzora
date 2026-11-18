#include "StorageService.h"

#include "AppException.h"
#include "DatabaseManager.h"
#include "DeviceHelper.h"
#include "Utils.h"

#include <QDir>
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

    // 先断开数据库，否则 Windows 会因 .db 仍被占用而删除失败。
    DatabaseManager::instance().closeAllResources();

    QStringList failedPaths;
    const QStringList paths = {
        device.getAppDataPath(),
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
    };
    for (const QString& path : paths) {
        if (path.isEmpty()) {
            continue;
        }
        if (!device.removeDataDir(path) && QDir(path).exists()) {
            failedPaths.append(path);
        }
    }

    if (!failedPaths.isEmpty()) {
        THROW(FileIOError, QStringLiteral("清除数据失败，以下目录无法删除：%1")
                                .arg(failedPaths.join(QStringLiteral("；"))));
    }

    QSettings settings;
    settings.clear();
    settings.sync();

    device.restartApplication();
}
