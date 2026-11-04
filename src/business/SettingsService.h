#ifndef SETTINGSSERVICE_H
#define SETTINGSSERVICE_H

#include <QObject>
#include <QJsonObject>

#include "GlobalConstants.h"
#include "BaseApiClient.h"

//设置核心业务

class SettingsService : public QObject {
    Q_OBJECT
public:
    static SettingsService& instance();
    ~SettingsService() override = default;

// ======================================
// 全局设置
// ======================================
    //获取全局设置
    QVariant getGlobalSetting(const QString& key, const QVariant& defaultValue = QVariant());
    //设置全局设置
    void setGlobalSetting(const QString& key, const QVariant& value);
    //保存全局设置
    void saveGlobalSettings();
    //加载全局设置,启动时加载所有外存中的配置
    void loadGlobalSettings();

// ======================================
// 角色独立设置
// ======================================
    //获取角色设置
    QVariant getCharacterSetting(const QString& charId,
                const QString& key, const QVariant& defaultValue = QVariant());
    //设置角色设置
    void setCharacterSetting(const QString& charId, const QString& key, const QVariant& value);

// ======================================
// 会话独立设置
// ======================================
    //获取会话设置
    QVariant getSessionSetting(const QString& charId,
                const QString& sessionId, const QString& key,
                const QVariant& defaultValue = QVariant());
    //设置会话设置
    void setSessionSetting(const QString& charId,
                const QString& sessionId, const QString& key, const QVariant& value);

// ======================================
// API连接配置持久化接口
// ======================================
    // //保存API连接配置
    // void saveApiConnectionConfig(GlobalConstants::ModelType type, const ApiConnectionConfig& config);
    // //获取API连接配置
    // ApiConnectionConfig getApiConnectionConfig(GlobalConstants::ModelType type);
    // //会话设置已更改
    // ApiConnectionConfig getDefaultOpenAIConfig();

signals:
    //全局设置已更改
    void globalSettingsChanged();
    //角色设置已更改
    void characterSettingsChanged(const QString& charId);
    //会话设置已更改
    void sessionSettingsChanged(const QString& charId, const QString& sessionId);
    //api配置变更通知信号
    void apiConfigChanged(GlobalConstants::ModelType type);

private:
    SettingsService(QObject* parent = nullptr);

    QJsonObject m_globalSettings;//全局设置
    QMap<QString, QJsonObject> m_characterSettings;//角色设置
    QMap<QString, QMap<QString, QJsonObject>> m_sessionSettings;//会话设置
    //QMap<GlobalConstants::ModelType, ApiConnectionConfig> m_apiConfigs;//API配置内存缓存
    QString m_configDir;//配置目录
};

#endif // SETTINGSSERVICE_H
