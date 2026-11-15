#ifndef APICONFIGENTITY_H
#define APICONFIGENTITY_H

#include <QString>
#include <QVariantMap>
#include <QDateTime>
#include "GlobalConstants.h"

//api配置数据实体

struct ApiConfigEntity {
    QString id;                     // 随机id
    QString name;                   // 配置显示名称
    GlobalConstants::ModelType modelType = GlobalConstants::ModelType::OpenAI;
    bool isBuiltin = false;         // 是否为内置预设
    bool isOpenAICompatible = true; // 是否兼容OpenAI接口规范
    QString baseDomain;             // API域名（如 https://api.openai.com/v1）
    QString apiPath;                // 接口路径（如 /chat/completions）
    QString apiKey;                 // API密钥
    QStringList availableModels;    // 该配置下可用的子模型列表
    QString currentModel;           // 当前选中的子模型
    QVariantMap extraHeaders;       // 额外请求头
    QVariantMap extraParams;        // 额外参数
    int timeout = 60000;            // 超时时间(ms)
    bool isActive = false;          // 是否为当前激活使用的配置
    QDateTime createdAt;
    QDateTime updatedAt;

};

#endif // APICONFIGENTITY_H