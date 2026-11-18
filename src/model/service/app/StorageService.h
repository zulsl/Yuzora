#ifndef STORAGESERVICE_H
#define STORAGESERVICE_H

#include <QString>

/**
 * @brief 应用存储目录与数据清理
 *
 * 从 SettingsViewModel 下沉：ViewModel 不再直接操作 DeviceHelper/QSettings。
 */
class StorageService
{
public:
    static StorageService& instance();

    QString dataPath() const;
    // 路径为空或切换失败时抛 AppException。
    void changeAppDataPath(const QString& path);
    // 删除应用数据目录、默认 AppData 目录与 QSettings，然后重启应用。
    void clearAllData();

private:
    StorageService() = default;
};

#endif // STORAGESERVICE_H
