#include "SettingsDao.h"
#include "DatabaseManager.h"
#include "Utils.h"
#include <QSqlQuery>
#include <QSqlError>

SettingsDao::SettingsDao(QObject* parent)
    : QObject(parent)
{
}

QString SettingsDao::tableName() const {
    return "settings";
}

QString SettingsDao::primaryKey() const {
    return "setting_key";
}

QStringList SettingsDao::columnNames() const {
    return {
        "setting_key", "setting_value", "value_type",
        "description", "updated_at"
    };
}

void SettingsDao::bindValues(QSqlQuery& query, const SettingsEntity& entity) const {
    query.addBindValue(entity.key);
    query.addBindValue(entity.value.toString());
    query.addBindValue(entity.valueType);
    query.addBindValue(entity.description);
    query.addBindValue(entity.updatedAt.toString(Qt::ISODate));
}

SettingsEntity SettingsDao::parseFromQuery(const QSqlQuery& query) const {
    SettingsEntity entity;
    entity.key = query.value("setting_key").toString();
    entity.value = query.value("setting_value");
    entity.valueType = query.value("value_type").toInt();
    entity.description = query.value("description").toString();
    entity.updatedAt = QDateTime::fromString(query.value("updated_at").toString(), Qt::ISODate);
    return entity;
}

SettingsEntity SettingsDao::selectByKey(const QString& key) {
    QSqlQuery query(DatabaseManager::instance().getThreadConnection());
    query.prepare(QString("SELECT * FROM %1 WHERE setting_key = :key").arg(tableName()));
    query.bindValue(":key", key);
    if (!query.exec() || !query.next()) {
        return {};
    }
    return parseFromQuery(query);
}

bool SettingsDao::insertOrUpdate(const SettingsEntity& entity) {
    QSqlQuery query(DatabaseManager::instance().getThreadConnection());
    query.prepare(QString(
        "INSERT OR REPLACE INTO %1 "
        "(setting_key, setting_value, value_type, description, updated_at) "
        "VALUES (:key, :value, :type, :desc, :time)"
    ).arg(tableName()));
    bindValues(query, entity);
    return query.exec();
}

QList<SettingsEntity> SettingsDao::selectAll() {
    QList<SettingsEntity> result;
    QSqlQuery query(DatabaseManager::instance().getThreadConnection());
    query.exec(QString("SELECT * FROM %1").arg(tableName()));
    while (query.next()) {
        result.append(parseFromQuery(query));
    }
    return result;
}

bool SettingsDao::removeByKey(const QString &key)
{
    return false;
}

bool SettingsDao::clearAll()
{
    return false;
}