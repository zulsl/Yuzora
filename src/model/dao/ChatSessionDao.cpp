#include "ChatSessionDao.h"
#include "DatabaseManager.h"
#include "Utils.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QDebug>

ChatSessionDao::ChatSessionDao(QObject* parent)
    : QObject(parent)
{
}

QList<ChatSessionEntity> ChatSessionDao::selectByCharacterId(const QString &characterId)
{
    QList<ChatSessionEntity> result;
    QSqlDatabase db = connection();
    QSqlQuery query(db);

    QString sql = QString("SELECT * FROM %1 WHERE character_id = ? ").arg(tableName());
    query.prepare(sql);
    query.addBindValue(characterId);

    if (!query.exec()) {
        DatabaseError("selectByCharacterId", query.lastError(),sql,query);
        return result;
    }

    while (query.next()) {
        result.append(parseFromQuery(query));
    }
    return result;
}

bool ChatSessionDao::updateLastModified(const QString& id, const QDateTime& time)
{
    QSqlDatabase db = connection();
    QSqlQuery query(db);

    query.prepare("UPDATE chat_sessions SET last_modified = :time WHERE id = :id");
    query.bindValue(":id", id);
    query.bindValue(":time", time.toString(Qt::ISODate));

    return query.exec();
}

bool ChatSessionDao::removeByCharacterId(const QString &characterId)
{
    QSqlDatabase db = connection();
    QSqlQuery query(db);

    QString sql = QString("DELETE FROM %1 WHERE character_id = ? ").arg(tableName());
    query.prepare(sql);
    query.addBindValue(characterId);

    if (!query.exec()) {
        DatabaseError("removeByCharacterId", query.lastError(),sql,query);
        return false;
    }
    return query.numRowsAffected() > 0;
}

QString ChatSessionDao::tableName() const
{
    return "chat_sessions";
}

QString ChatSessionDao::primaryKey() const
{
    return "id";
}

QStringList ChatSessionDao::columnNames() const
{
    return {
        "id", "character_id", "title", "created_at",
        "last_modified"
    };
}

void ChatSessionDao::bindValues(QSqlQuery &query, const ChatSessionEntity &entity) const
{
    //严格顺序
    query.addBindValue(entity.id);
    query.addBindValue(entity.characterId);
    query.addBindValue(entity.title);
    query.addBindValue(entity.createdAt.toString(Qt::ISODate));
    query.addBindValue(entity.lastModified.toString(Qt::ISODate));
}

ChatSessionEntity ChatSessionDao::parseFromQuery(const QSqlQuery &query) const
{
    ChatSessionEntity entity;
    entity.id = query.value("id").toString();
    entity.characterId = query.value("character_id").toString();
    entity.title = query.value("title").toString();
    entity.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
    entity.lastModified = QDateTime::fromString(query.value("last_modified").toString(), Qt::ISODate);
    return entity;
}