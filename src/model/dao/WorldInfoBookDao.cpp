#include "WorldInfoBookDao.h"
#include "DatabaseManager.h"
#include "Utils.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

WorldInfoBookDao::WorldInfoBookDao(QObject* parent)
    : QObject(parent)
{
}

QString WorldInfoBookDao::tableName() const
{
    return "world_info_books";
}

QString WorldInfoBookDao::primaryKey() const
{
    return "id";
}

QStringList WorldInfoBookDao::columnNames() const
{
    return {
        "id", "name", "is_enabled",
        "created_at", "updated_at"
    };
}

void WorldInfoBookDao::bindValues(QSqlQuery &query, const WorldInfoBookEntity &entity) const
{
    //严格顺序
    query.addBindValue(entity.id);
    query.addBindValue(entity.name);
    query.addBindValue(entity.isEnabled?1:0);
    query.addBindValue(entity.createdAt.toString(Qt::ISODate));
    query.addBindValue(entity.updatedAt.toString(Qt::ISODate));
}

WorldInfoBookEntity WorldInfoBookDao::parseFromQuery(const QSqlQuery &query) const
{
    WorldInfoBookEntity entity;
    entity.id = query.value("id").toString();
    entity.name = query.value("name").toString();
    entity.isEnabled = query.value("is_enabled").toBool();
    entity.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
    entity.updatedAt = QDateTime::fromString(query.value("updated_at").toString(), Qt::ISODate);
    return entity;
}