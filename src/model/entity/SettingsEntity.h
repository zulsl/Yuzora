#ifndef SETTINGSENTITY_H
#define SETTINGSENTITY_H

#include <QString>
#include <QVariant>
#include <QDateTime>

//设置数据实体

struct SettingsEntity {
    QString id;           //保留键,未使用
    QString key;          // 设置项核心键(索引)
    QVariant value;       // 设置值（自动适配字符串/数字/布尔）
    int valueType = 0;    // 值类型：0-string 1-int 2-bool 3-double
    QString description;  // 设置项描述,保留
    QDateTime updatedAt;  // 最后修改时间
};

#endif // SETTINGSENTITY_H
