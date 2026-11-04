#ifndef PROMPTENTITY_H
#define PROMPTENTITY_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QtQml>
#include "GlobalConstants.h"

struct PromptEntity {
    Q_GADGET
    QML_VALUE_TYPE(promptEntity)

public:
    QString id;                    //唯一id
    QString identifier;           // 兼容id
    QString presetId;             //所属预设id
    QString name;                 // 显示名称
    bool isActive = true;         //是否激活
    GlobalConstants::MessageType role = GlobalConstants::MessageType::System;   // 0 system / 1 user / 2 assistant
    QString content;              // 原始内容（含变量占位符）
    bool system_prompt = false;   // 是否为系统内置提示词
    bool marker = false;          // 是否为标记（如 charDescription, worldInfoBefore）
    int injection_position = 0;   // 注入位置
        //0 表示相对位置（即放在系统区，与其他系统提示按顺序排列）
        //1 表示绝对注入（即插入到聊天历史中）（In-Chat）
    int injection_depth = 4;      // 注入深度（仅在 absolute 时有效）
    int injection_order = 100;    // 注入优先级
    bool forbid_overrides = false;// 是否禁止覆盖
    QStringList injection_trigger;// 触发类型（如 "continue", "impersonate"）()


    //其他杂项
    QString extraPosition;      //额外位置(提示在提示列表中的位置。)
    bool extension;//扩展标记,提示是通过扩展添加的。
    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QString identifier MEMBER identifier)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(bool isActive MEMBER isActive)
    Q_PROPERTY(GlobalConstants::MessageType role MEMBER role)
    Q_PROPERTY(QString content MEMBER content)
    Q_PROPERTY(int injection_position MEMBER injection_position)

};
Q_DECLARE_METATYPE(PromptEntity)

#endif
