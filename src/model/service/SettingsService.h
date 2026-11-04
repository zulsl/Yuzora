#ifndef SETTINGSSERVICE_H
#define SETTINGSSERVICE_H

#include <QObject>
#include <QVariantMap>
#include "SettingsDao.h"
#include "GlobalConstants.h"

class SettingsService : public QObject {
    Q_OBJECT
public:
    static SettingsService& instance();
    ~SettingsService() override;

    SettingsService(const SettingsService&) = delete;
    SettingsService& operator=(const SettingsService&) = delete;
    SettingsService(SettingsService&&) = delete;
    SettingsService& operator=(SettingsService&&) = delete;

    // ========== 类型安全的读取接口 ==========
    QString getString(const QString& key, const QString& defaultValue = QString());
    int getInt(const QString& key, int defaultValue = 0);
    bool getBool(const QString& key, bool defaultValue = false);
    double getDouble(const QString& key, double defaultValue = 0.0);

    // ========== 通用写入接口 ==========
    Q_INVOKABLE void setValue(const QString& key, const QVariant& value);

    // ========== 批量操作 ==========
    void resetToDefault(const QString& key);
    void resetAll();
    void reloadFromDatabase();

signals:
    // 通用变更信号，供各模块监听自定义设置
    void settingChanged(const QString& key, const QVariant& value);

    // 常用设置专属变更信号，方便UI绑定
    void themeChanged(GlobalConstants::ThemeType theme);
    void fontSizeChanged(int size);
    void proxySettingsChanged();

private:
    explicit SettingsService(QObject* parent = nullptr);
    void loadAllToCache();
    void initDefaultValues();


    QVariant getDefaultValue(const QString& key) const;

    SettingsDao* m_dao = nullptr;
    QVariantMap m_cache;       // 内存缓存，读写优先走这里
    QVariantMap m_defaults;    // 默认值表
};

#endif // SETTINGSSERVICE_H