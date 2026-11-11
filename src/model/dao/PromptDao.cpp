#include "PromptDao.h"


PromptDao::PromptDao(QObject *parent)
    : QObject{parent}
{

}

QList<PromptEntity> PromptDao::selectPromptsByPresetId(const QString &presetId)
{
    QList<PromptEntity> result;
    QSqlDatabase db = connection();
    QSqlQuery query(db);

    QString sql = QString("SELECT * FROM %1 WHERE preset_id = ? ").arg(tableName());
    query.prepare(sql);
    query.addBindValue(presetId);

    if (!query.exec()) {
        DatabaseError("selectPromptsByPresetId", query.lastError(),sql,query);
        return result;
    }

    while (query.next()) {
        result.append(parseFromQuery(query));
    }
    return result;
}

bool PromptDao::createPrompts(const QList<PromptEntity> & prompts)
{
    if (prompts.isEmpty())
        return true;

    QSqlDatabase db = connection();
    QSqlQuery query(db);

    QStringList cols = columnNames();
    QStringList placeholders(cols.size(), "?");
    QString sql = QString("INSERT INTO %1 (%2) VALUES (%3)")
        .arg(tableName(), cols.join(","), placeholders.join(","));

    // 开启事务
    if (!db.transaction()) {
        DatabaseError("transaction", db.lastError(), sql, query);
        return false;
    }

    //准备语句
    if (!query.prepare(sql)) {
        DatabaseError("insert", query.lastError(), sql, query);
        return false;
    }

    //绑定参数
    for (const PromptEntity & entity : prompts) {
        bindValues(query, entity);
        if (!query.exec()) {
            DatabaseError("insert", query.lastError(), sql, query);
            db.rollback();                  // 失败回滚
            return false;
        }
    }

    if (!db.commit()) {
        DatabaseError("commit", db.lastError(), sql, query);
        db.rollback();
        return false;
    }

    return true;
}

QString PromptDao::tableName() const
{
    return "completion_presets_prompts";
}

QString PromptDao::primaryKey() const
{
    return "id";
}

QStringList PromptDao::columnNames() const
{
    return {
        "id","identifier","preset_id","name","is_active","role","content","system_prompt","marker",
        "injection_position","injection_depth","injection_order","forbid_overrides"
    };
}

void PromptDao::bindValues(QSqlQuery &query, const PromptEntity &entity) const
{
    //严格顺序
    query.addBindValue(entity.id);
    query.addBindValue(entity.identifier);
    query.addBindValue(entity.presetId);
    query.addBindValue(entity.name);
    query.addBindValue(entity.isActive?1:0);
    query.addBindValue(entity.role);
    query.addBindValue(entity.content);
    query.addBindValue(entity.system_prompt?1:0);
    query.addBindValue(entity.marker?1:0);
    query.addBindValue(entity.injection_position);
    query.addBindValue(entity.injection_depth);
    query.addBindValue(entity.injection_order);
    query.addBindValue(entity.forbid_overrides?1:0);

}

PromptEntity PromptDao::parseFromQuery(const QSqlQuery &query) const
{
    PromptEntity entity;
    entity.id = query.value("id").toString();
    entity.identifier = query.value("identifier").toString();
    entity.presetId = query.value("preset_id").toString();
    entity.name = query.value("name").toString();
    entity.isActive = query.value("is_active").toBool();
    entity.role = static_cast<GlobalConstants::MessageType>(query.value("role").toInt());
    entity.content = query.value("content").toString();
    entity.system_prompt = query.value("system_prompt").toBool();
    entity.marker = query.value("marker").toBool();
    entity.injection_position = query.value("injection_position").toInt();
    entity.injection_depth = query.value("injection_depth").toInt();
    entity.injection_order = query.value("injection_order").toInt();
    entity.forbid_overrides = query.value("forbid_overrides").toBool();
    return entity;
}
