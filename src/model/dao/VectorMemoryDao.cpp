#include "VectorMemoryDao.h"
#include "DatabaseManager.h"
#include "Utils.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

VectorMemoryDao::VectorMemoryDao(QObject* parent)
    : QObject(parent)
{
}

QList<VectorMemoryEntity> VectorMemoryDao::selectByCharacterId(const QString& characterId)
{
    QList<VectorMemoryEntity> result;
    QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
    QSqlQuery query(db);

    const QString sql = QStringLiteral(R"(
        SELECT id, character_id, content, timestamp, importance, embedding_file_path, source_session_id
        FROM vector_memories
        WHERE character_id = :characterId
        ORDER BY timestamp DESC
    )");

    query.prepare(sql);
    query.bindValue(":characterId", characterId);

    if (!query.exec()) {
        qWarning() << "[VectorMemoryDao] selectByCharacterId error:" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        VectorMemoryEntity entity;
        entity.id = query.value("id").toString();
        entity.characterId = query.value("character_id").toString();
        entity.content = query.value("content").toString();
        entity.timestamp = QDateTime::fromString(query.value("timestamp").toString(), Qt::ISODate);
        entity.importance = query.value("importance").toDouble();
        entity.embeddingFilePath = query.value("embedding_file_path").toString();
        entity.sourceSessionId = query.value("source_session_id").toString();
        result.append(entity);
    }

    return result;
}

QString VectorMemoryDao::insert(const VectorMemoryEntity& memory)
{
    QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
    QSqlQuery query(db);

    QString newId = memory.id.isEmpty() ? Utils::generateUuid() : memory.id;

    query.prepare(R"(
        INSERT INTO vector_memories (
            id, character_id, content, timestamp, importance,
            embedding_file_path, source_session_id
        ) VALUES (
            :id, :characterId, :content, :timestamp, :importance,
            :embeddingFilePath, :sourceSessionId
        )
    )");

    query.bindValue(":id", newId);
    query.bindValue(":characterId", memory.characterId);
    query.bindValue(":content", memory.content);
    query.bindValue(":timestamp", memory.timestamp.isValid() ? memory.timestamp.toString(Qt::ISODate) : QDateTime::currentDateTime().toString(Qt::ISODate));
    query.bindValue(":importance", memory.importance);
    query.bindValue(":embeddingFilePath", memory.embeddingFilePath);
    query.bindValue(":sourceSessionId", memory.sourceSessionId);

    if (!query.exec()) {
        qWarning() << "[VectorMemoryDao] insert error:" << query.lastError().text();
        return QString();
    }

    return newId;
}

bool VectorMemoryDao::remove(const QString& id)
{
    QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
    QSqlQuery query(db);

    query.prepare("DELETE FROM vector_memories WHERE id = :id");
    query.bindValue(":id", id);

    return query.exec();
}

bool VectorMemoryDao::clearByCharacterId(const QString& characterId)
{
    QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
    QSqlQuery query(db);

    query.prepare("DELETE FROM vector_memories WHERE character_id = :characterId");
    query.bindValue(":characterId", characterId);

    return query.exec();
}