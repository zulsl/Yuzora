#include "ApiConfigDao.h"
#include "ApiConfigEntity.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

ApiConfigDao::ApiConfigDao(QObject *parent)
    : QObject(parent)
{
}

// ==================== 激活状态管理 ====================
ApiConfigEntity ApiConfigDao::selectActive()
{
    ApiConfigEntity result;
    QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
    QSqlQuery query(db);

    QString sql = QStringLiteral("SELECT * FROM api_configs WHERE is_active = 1 LIMIT 1");

    if (!query.exec(sql)) {
        qWarning() << "查询激活API配置失败:" << query.lastError().text();
        return result;
    }

    if (query.next()) {
        result = parseFromQuery(query);
    }
    return result;
}

bool ApiConfigDao::setActive(const QString &id)
{
    QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
    QSqlQuery query(db);

    QString sql = QStringLiteral("UPDATE api_configs SET is_active = 1 WHERE id = :id");
    query.prepare(sql);
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "设置激活配置失败:" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool ApiConfigDao::clearAllActive()
{
    QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
    QSqlQuery query(db);

    QString sql = QStringLiteral("UPDATE api_configs SET is_active = 0");

    if (!query.exec(sql)) {
        qWarning() << "清除激活状态失败:" << query.lastError().text();
        return false;
    }
    return true;
}



bool ApiConfigDao::rename(const QString &id, const QString &newName)
{
    return false;
}


QString ApiConfigDao::tableName() const
{
    return "api_configs";
}

QString ApiConfigDao::primaryKey() const
{
    return "id";
}

QStringList ApiConfigDao::columnNames() const
{
    return {
        "id", "name", "model_type", "is_builtin", "is_openai_compatible",
        "base_domain", "api_path", "api_key", "available_models",
        "current_model",
        "extra_headers", "extra_params", "timeout", "is_active",
        "created_at", "updated_at"
    };

}

void ApiConfigDao::bindValues(QSqlQuery &query, const ApiConfigEntity &entity) const
{
    //严格顺序
    query.addBindValue(entity.id);
    query.addBindValue(entity.name);
    query.addBindValue(static_cast<int>(entity.modelType));
    query.addBindValue(entity.isBuiltin ? 1 : 0);
    query.addBindValue(entity.isOpenAICompatible ? 1 : 0);
    query.addBindValue(entity.baseDomain);
    query.addBindValue(entity.apiPath);
    query.addBindValue(entity.apiKey);
    query.addBindValue(Utils::stringListToJson(entity.availableModels));
    query.addBindValue(entity.currentModel);
    query.addBindValue(Utils::variantMapToJson(entity.extraHeaders));
    query.addBindValue(Utils::variantMapToJson(entity.extraParams));
    query.addBindValue(entity.timeout);
    query.addBindValue(entity.isActive ? 1 : 0);
    query.addBindValue(entity.createdAt.toString(Qt::ISODate));
    query.addBindValue(entity.updatedAt.toString(Qt::ISODate));
}

ApiConfigEntity ApiConfigDao::parseFromQuery(const QSqlQuery &query) const
{
    ApiConfigEntity entity;
    entity.id = query.value("id").toString();
    entity.name = query.value("name").toString();
    entity.modelType = static_cast<GlobalConstants::ModelType>(query.value("model_type").toInt());
    entity.isBuiltin = query.value("is_builtin").toBool();
    entity.isOpenAICompatible = query.value("is_openai_compatible").toBool();
    entity.baseDomain = query.value("base_domain").toString();
    entity.apiPath = query.value("api_path").toString();
    entity.apiKey = query.value("api_key").toString();
    entity.availableModels = Utils::jsonToStringList(query.value("available_models"));
    entity.currentModel = query.value("current_model").toString();
    entity.extraHeaders = Utils::jsonToVariantMap(query.value("extra_headers"));
    entity.extraParams = Utils::jsonToVariantMap(query.value("extra_params"));
    entity.timeout = query.value("timeout").toInt();
    entity.isActive = query.value("is_active").toBool();
    entity.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
    entity.updatedAt = QDateTime::fromString(query.value("updated_at").toString(), Qt::ISODate);
    return entity;
}





//以下废弃
// // ==================== 查询方法 ====================

// QList<ApiConfigEntity> ApiConfigDao::selectAll()
// {
//     QList<ApiConfigEntity> result;
//     QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
//     QSqlQuery query(db);

//     //R"() C++11 原始字符串字面量（Raw String）
//     //QStringLiteral 静态QString,性能更好
//     QString sql = QStringLiteral(R"(
//         SELECT * FROM api_configs
//         ORDER BY is_builtin DESC, created_at ASC
//     )");

//     if (!query.exec(sql)) {
//         qWarning() << "查询API配置列表失败:" << query.lastError().text();
//         return result;
//     }

//     while (query.next()) {
//         result.append(parseFromQuery(query));
//     }
//     return result;
// }

// ApiConfigEntity ApiConfigDao::selectById(const QString &id)
// {
//     ApiConfigEntity result;
//     QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
//     QSqlQuery query(db);

//     QString sql = QStringLiteral("SELECT * FROM api_configs WHERE id = :id");
//     query.prepare(sql);
//     query.bindValue(":id", id);

//     if (!query.exec()) {
//         qWarning() << "查询API配置失败:" << query.lastError().text();
//         return result;
//     }

//     if (query.next()) {
//         result = parseFromQuery(query);
//     }
//     return result;
// }



// // ==================== 写入方法 ====================

// QString ApiConfigDao::insert(const ApiConfigEntity &config)
// {
//     QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
//     QSqlQuery query(db);

//     QString sql = QStringLiteral(R"(
//         INSERT INTO api_configs (
//             id, name, model_type, is_builtin, is_openai_compatible,
//             base_domain, api_path, api_key, available_models,
//             extra_headers, extra_params, timeout, is_active,
//             created_at, updated_at
//         ) VALUES (
//             :id, :name, :model_type, :is_builtin, :is_openai_compatible,
//             :base_domain, :api_path, :api_key, :available_models,
//             :extra_headers, :extra_params, :timeout, :is_active,
//             :created_at, :updated_at
//         )
//     )");

//     query.prepare(sql);
//     query.bindValue(":id", config.id);
//     query.bindValue(":name", config.name);
//     query.bindValue(":model_type", static_cast<int>(config.modelType));
//     query.bindValue(":is_builtin", config.isBuiltin ? 1 : 0);
//     query.bindValue(":is_openai_compatible", config.isOpenAICompatible ? 1 : 0);
//     query.bindValue(":base_domain", config.baseDomain);
//     query.bindValue(":api_path", config.apiPath);
//     query.bindValue(":api_key", config.apiKey);

//     // 序列化可用模型列表为JSON数组
//     QJsonArray modelsArray;
//     for (const QString& model : config.availableModels) {
//         modelsArray.append(model);
//     }
//     query.bindValue(":available_models", QJsonDocument(modelsArray).toJson(QJsonDocument::Compact));

//     // 序列化额外请求头为JSON对象
//     QJsonObject headersObj = QJsonObject::fromVariantMap(config.extraHeaders);
//     query.bindValue(":extra_headers", QJsonDocument(headersObj).toJson(QJsonDocument::Compact));

//     // 序列化额外参数为JSON对象
//     QJsonObject paramsObj = QJsonObject::fromVariantMap(config.extraParams);
//     query.bindValue(":extra_params", QJsonDocument(paramsObj).toJson(QJsonDocument::Compact));

//     query.bindValue(":timeout", config.timeout);
//     query.bindValue(":is_active", config.isActive ? 1 : 0);
//     query.bindValue(":created_at", config.createdAt.toString(Qt::ISODate));
//     query.bindValue(":updated_at", config.updatedAt.toString(Qt::ISODate));

//     if (!query.exec()) {
//         qWarning() << "插入API配置失败:" << query.lastError().text();
//         qWarning().noquote() << "SQL语句：" << sql;
//         qWarning() << "占位符总数：" << query.boundValues().size();
//         qWarning().noquote() << "绑定参数详情：" << query.boundValues();
//         return QString();
//     }
//     return config.id;
// }

// bool ApiConfigDao::update(const ApiConfigEntity &config)
// {
//     QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
//     QSqlQuery query(db);

//     QString sql = QStringLiteral(R"(
//         UPDATE api_configs SET
//             name = :name,
//             model_type = :model_type,
//             is_openai_compatible = :is_openai_compatible,
//             base_domain = :base_domain,
//             api_path = :api_path,
//             api_key = :api_key,
//             available_models = :available_models,
//             extra_headers = :extra_headers,
//             extra_params = :extra_params,
//             timeout = :timeout,
//             is_active = :is_active,
//             updated_at = :updated_at
//         WHERE id = :id
//     )");

//     query.prepare(sql);
//     query.bindValue(":id", config.id);
//     query.bindValue(":name", config.name);
//     query.bindValue(":model_type", static_cast<int>(config.modelType));
//     query.bindValue(":is_openai_compatible", config.isOpenAICompatible ? 1 : 0);
//     query.bindValue(":base_domain", config.baseDomain);
//     query.bindValue(":api_path", config.apiPath);
//     query.bindValue(":api_key", config.apiKey);

//     // 序列化可用模型
//     QJsonArray modelsArray;
//     for (const QString& model : config.availableModels) {
//         modelsArray.append(model);
//     }
//     query.bindValue(":available_models", QJsonDocument(modelsArray).toJson(QJsonDocument::Compact));

//     // 序列化额外请求头
//     QJsonObject headersObj = QJsonObject::fromVariantMap(config.extraHeaders);
//     query.bindValue(":extra_headers", QJsonDocument(headersObj).toJson(QJsonDocument::Compact));

//     // 序列化额外参数
//     QJsonObject paramsObj = QJsonObject::fromVariantMap(config.extraParams);
//     query.bindValue(":extra_params", QJsonDocument(paramsObj).toJson(QJsonDocument::Compact));

//     query.bindValue(":timeout", config.timeout);
//     query.bindValue(":is_active", config.isActive ? 1 : 0);
//     query.bindValue(":updated_at", config.updatedAt.toString(Qt::ISODate));

//     if (!query.exec()) {
//         qWarning() << "更新API配置失败:" << query.lastError().text();
//         return false;
//     }
//     return query.numRowsAffected() > 0;
// }

// bool ApiConfigDao::remove(const QString &id)
// {
//     QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
//     QSqlQuery query(db);

//     QString sql = QStringLiteral("DELETE FROM api_configs WHERE id = :id");
//     query.prepare(sql);
//     query.bindValue(":id", id);

//     if (!query.exec()) {
//         qWarning() << "删除API配置失败:" << query.lastError().text();
//         return false;
//     }
//     return query.numRowsAffected() > 0;
// }


// // ==================== 私有工具方法 ====================

// ApiConfigEntity ApiConfigDao::parseFromQuery(const QSqlQuery &query)
// {
//     ApiConfigEntity entity;
//     // QSqlRecord record = query.record();//可选

//     entity.id = query.value("id").toString();
//     entity.name = query.value("name").toString();
//     entity.modelType = static_cast<GlobalConstants::ModelType>(query.value("model_type").toInt());
//     entity.isBuiltin = query.value("is_builtin").toBool();
//     entity.isOpenAICompatible = query.value("is_openai_compatible").toBool();
//     entity.baseDomain = query.value("base_domain").toString();
//     entity.apiPath = query.value("api_path").toString();
//     entity.apiKey = query.value("api_key").toString();
//     entity.timeout = query.value("timeout").toInt();
//     entity.isActive = query.value("is_active").toBool();
//     entity.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
//     entity.updatedAt = QDateTime::fromString(query.value("updated_at").toString(), Qt::ISODate);

//     // 反序列化可用模型列表
//     QString modelsJson = query.value("available_models").toString();
//     if (!modelsJson.isEmpty()) {
//         QJsonDocument doc = QJsonDocument::fromJson(modelsJson.toUtf8());
//         if (doc.isArray()) {
//             QJsonArray array = doc.array();
//             for (const auto& val : array) {
//                 entity.availableModels.append(val.toString());
//             }
//         }
//     }

//     // 反序列化额外请求头
//     QString headersJson = query.value("extra_headers").toString();
//     if (!headersJson.isEmpty()) {
//         QJsonDocument doc = QJsonDocument::fromJson(headersJson.toUtf8());
//         if (doc.isObject()) {
//             entity.extraHeaders = doc.object().toVariantMap();
//         }
//     }

//     // 反序列化额外参数
//     QString paramsJson = query.value("extra_params").toString();
//     if (!paramsJson.isEmpty()) {
//         QJsonDocument doc = QJsonDocument::fromJson(paramsJson.toUtf8());
//         if (doc.isObject()) {
//             entity.extraParams = doc.object().toVariantMap();
//         }
//     }

//     return entity;
// }
