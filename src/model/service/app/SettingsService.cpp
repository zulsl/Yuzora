#include "SettingsService.h"
#include "Utils.h"
#include "AppException.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QImage>

SettingsService& SettingsService::instance()
{
    static SettingsService instance;
    return instance;
}

SettingsService::SettingsService(QObject* parent)
    : QObject(parent)
    , m_dao(new SettingsDao(this))
{
    initDefaultValues();
}

SettingsService::SettingsService(SettingsDao* dao, QObject* parent)
    : QObject(parent)
    , m_dao(dao ? dao : new SettingsDao(this))
{
    initDefaultValues();
}

SettingsService::~SettingsService() = default;

// 初始化默认值表
void SettingsService::initDefaultValues()
{
    // m_defaults[GlobalConstants::USER_AVATAR] = "用户";

    m_defaults[GlobalConstants::THEME_MODE] = static_cast<int>(GlobalConstants::ThemeType::Default);
    m_defaults[GlobalConstants::FONT_SIZE] = 14;
    m_defaults[GlobalConstants::MESSAGE_BUBBLE_STYLE] = "rounded";
    m_defaults[GlobalConstants::THEME_COLOR] = "#4169E1";//皇家蓝

    m_defaults[GlobalConstants::AUTO_SAVE_INTERVAL] = 30;
    m_defaults[GlobalConstants::SEND_WITH_ENTER] = true;
    m_defaults[GlobalConstants::SHOW_TIME_STAMP] = true;
    m_defaults[GlobalConstants::AUTO_SCROLL_TO_BOTTOM] = true;

    m_defaults[GlobalConstants::ENABLE_PROXY] = false;
    m_defaults[GlobalConstants::PROXY_TYPE] = "http";
    m_defaults[GlobalConstants::PROXY_HOST] = "127.0.0.1";
    m_defaults[GlobalConstants::PROXY_PORT] = 7890;
    m_defaults[GlobalConstants::REQUEST_TIMEOUT] = 30;

    m_defaults[GlobalConstants::AUTO_CONTINUE_GENERATE] = false;
    m_defaults[GlobalConstants::MAX_HISTORY_LOAD] = 50;
}

// 启动时全量加载到内存
void SettingsService::loadAllToCache()
{
    QList<SettingsEntity> all = m_dao->selectAll();
    for (const SettingsEntity& item : all) {
        m_cache[item.key] = item.value;
    }
}

QVariant SettingsService::getDefaultValue(const QString& key) const
{
    return m_defaults.value(key);
}

// ========== 读取接口 ==========
QString SettingsService::getString(const QString& key, const QString& defaultValue)
{
    if (m_cache.contains(key)) {
        return m_cache[key].toString();
    }
    return defaultValue.isEmpty() ? getDefaultValue(key).toString() : defaultValue;
}

int SettingsService::getInt(const QString& key, int defaultValue)
{
    if (m_cache.contains(key)) {
        return m_cache[key].toInt();
    }
    return defaultValue == 0 ? getDefaultValue(key).toInt() : defaultValue;
}

bool SettingsService::getBool(const QString& key, bool defaultValue)
{
    if (m_cache.contains(key)) {
        return m_cache[key].toBool();
    }
    return !defaultValue ? getDefaultValue(key).toBool() : defaultValue;
}

double SettingsService::getDouble(const QString& key, double defaultValue)
{
    if (m_cache.contains(key)) {
        return m_cache[key].toDouble();
    }
    return qFuzzyCompare(defaultValue, 0.0)
        ? getDefaultValue(key).toDouble() : defaultValue;
}

// ========== 写入接口 ==========
void SettingsService::setValue(const QString& key, const QVariant& value)
{
    if (m_cache.value(key) == value) {
        return; // 值未变化，跳过
    }

    // 1. 更新内存缓存
    m_cache[key] = value;

    // 2. 异步写入数据库
    SettingsEntity entity;
    entity.key = key;
    entity.value = value;
    entity.updatedAt = QDateTime::currentDateTime();
    m_dao->insertOrUpdate(entity);

    // 3. 发送变更信号
    emit settingChanged(key, value);

    // 4. 常用设置专属信号
    if (key == GlobalConstants::THEME_MODE) {
        emit themeChanged(static_cast<GlobalConstants::ThemeType>(value.toInt()));
    } else if (key == GlobalConstants::FONT_SIZE) {
        emit fontSizeChanged(value.toInt());
    } else if (key.startsWith("network.proxy")) {
        emit proxySettingsChanged();
    }
}

void SettingsService::saveUserAvatar(const QString& sourcePath)
{
    if (sourcePath.isEmpty()) {
        THROW(FileIOError, "头像路径不能为空");
    }

    QImage image(sourcePath);
    if (image.isNull()) {
        THROW(FileIOError, QString("无法读取图片：%1").arg(sourcePath));
    }

    const QString fileName = Utils::generateUuid() + ".png";
    const QString fullPath = QDir(Utils::getAvatarDirPath()).filePath(fileName);
    if (!image.save(fullPath, "PNG")) {
        THROW(FileIOError, "头像保存失败：无法写入文件");
    }

    const QString oldAvatar = getString(GlobalConstants::USER_AVATAR);
    setValue(GlobalConstants::USER_AVATAR, fileName);

    // 保存成功后再清理旧头像，避免失败时把仍在使用的文件删掉。
    if (!oldAvatar.isEmpty() && oldAvatar != fileName) {
        QFile::remove(QDir(Utils::getAvatarDirPath()).filePath(oldAvatar));
    }
}

// ========== 重置操作 ==========
void SettingsService::resetToDefault(const QString& key)
{
    QVariant def = getDefaultValue(key);
    if (def.isValid()) {
        setValue(key, def);
    }
}

void SettingsService::resetAll()
{
    m_dao->clearAll();
    m_cache.clear();
    for (auto it = m_defaults.keyValueBegin(); it != m_defaults.keyValueEnd(); ++it) {
        setValue((*it).first, (*it).second);
    }
}

void SettingsService::reloadFromDatabase()
{
    m_cache.clear();
    loadAllToCache();
}