#include "CharacterDao.h"
#include "DatabaseManager.h"
#include "Utils.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

CharacterDao::CharacterDao(QObject* parent)
    : QObject(parent)
{
}

QList<CharacterEntity> CharacterDao::searchByName(const QString &keyword)
{
    QList<CharacterEntity> result;
    QSqlDatabase db = connection();
    QSqlQuery query(db);

    QString sql = "SELECT * FROM characters WHERE name LIKE ? ORDER BY updated_at DESC";
    query.prepare(sql);
    query.addBindValue(QString("%%1%").arg(keyword));

    if (!query.exec()) {
        DatabaseError("searchByName", query.lastError());
        return result;
    }

    while (query.next()) {
        result.append(parseFromQuery(query));
    }
    return result;
}

bool CharacterDao::updateAvatar(const QString &charId, const QString &avatarPath)
{
    QSqlDatabase db = connection();
    QSqlQuery query(db);

    query.prepare("UPDATE characters SET avatar_path = ?, updated_at = ? WHERE id = ?");
    query.addBindValue(avatarPath);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.addBindValue(charId);

    if (!query.exec()) {
        DatabaseError("updateAvatar", query.lastError());
        return false;
    }
    return query.numRowsAffected() > 0;
}

QString CharacterDao::tableName() const
{
    return "characters";
}

QString CharacterDao::primaryKey() const
{
    return "id";
}

QStringList CharacterDao::columnNames() const
{
    return {
        "id","name", "description","first_message","alternate_greetings",
        "creator_notes","creator","personality","scenario","example_dialogue",
        "system_prompt","post_history_instructions","character_version","tags","card_format_version",
        "avatar_path","world_book_ids","last_session_id",
        "is_favorite","extensions","created_at",
        "updated_at"
    };
}

void CharacterDao::bindValues(QSqlQuery &query, const CharacterEntity &entity) const
{
    query.addBindValue(entity.id);
    query.addBindValue(entity.name);
    query.addBindValue(entity.description);
    query.addBindValue(entity.firstMessage);
    query.addBindValue(Utils::stringListToJson(entity.alternateGreetings));
    query.addBindValue(entity.creatorNotes);
    query.addBindValue(entity.creator);
    query.addBindValue(entity.personality);
    query.addBindValue(entity.scenario);
    query.addBindValue(entity.exampleDialogue);
    query.addBindValue(entity.systemPrompt);
    query.addBindValue(entity.postHistoryInstructions);
    query.addBindValue(entity.characterVersion);
    query.addBindValue(Utils::stringListToJson(entity.tags));
    query.addBindValue(entity.cardFormatVersion);
    query.addBindValue(entity.avatarPath);
    query.addBindValue(Utils::stringListToJson(entity.worldBookIds));
    query.addBindValue(entity.lastSessionId);
    query.addBindValue(entity.isFavorite);
    query.addBindValue(Utils::variantMapToJson(entity.extensions));
    query.addBindValue(entity.createdAt.toString(Qt::ISODate));
    query.addBindValue(entity.updatedAt.toString(Qt::ISODate));
}

CharacterEntity CharacterDao::parseFromQuery(const QSqlQuery &query) const
{
    CharacterEntity entity;
    entity.id = query.value("id").toString();
    entity.name = query.value("name").toString();
    entity.description = query.value("description").toString();
    entity.firstMessage = query.value("first_message").toString();
    entity.alternateGreetings=Utils::jsonToStringList(query.value("alternate_greetings"));
    entity.creatorNotes=query.value("creator_notes").toString();
    entity.creator=query.value("creator").toString();
    entity.personality = query.value("personality").toString();
    entity.scenario=query.value("scenario").toString();
    entity.exampleDialogue = query.value("example_dialogue").toString();
    entity.systemPrompt = query.value("system_prompt").toString();
    entity.postHistoryInstructions = query.value("post_history_instructions").toString();
    entity.characterVersion=query.value("character_version").toString();
    entity.tags=Utils::jsonToStringList(query.value("tags"));
    entity.cardFormatVersion=query.value("card_format_version").toInt();
    entity.avatarPath = query.value("avatar_path").toString();
    entity.worldBookIds=Utils::jsonToStringList(query.value("world_book_ids"));
    entity.lastSessionId=query.value("last_session_id").toString();
    entity.isFavorite=query.value("is_favorite").toBool();
    entity.extensions = Utils::jsonToVariantMap(query.value("extensions"));
    entity.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
    entity.updatedAt = QDateTime::fromString(query.value("updated_at").toString(), Qt::ISODate);

    return entity;
}

// QString CharacterDao::entityId(const CharacterEntity &entity) const
// {
//     return entity.id;
// }

// void CharacterDao::setEntityId(CharacterEntity &entity, const QString &id) const
// {
//     entity.id = id;
// }
//以下废弃
// QList<CharacterEntity> CharacterDao::selectAll()
// {
//     QList<CharacterEntity> result;
//     QSqlDatabase db = connection();
//     QSqlQuery query(db);

//     const QString sql = QStringLiteral(R"(
//         SELECT id, name, description, personality, first_message, example_dialogue,
//                system_prompt, creator_notes, extensions, avatar_path, version,
//                created_at, updated_at
//         FROM characters
//         ORDER BY updated_at DESC
//     )");

//     if (!query.exec(sql)) {
//         qWarning() << "[CharacterDao] selectAll error:" << query.lastError().text();
//         return result;
//     }

//     while (query.next()) {
//         CharacterEntity entity;
//         entity.id = query.value("id").toString();
//         entity.name = query.value("name").toString();
//         entity.description = query.value("description").toString();
//         entity.personality = query.value("personality").toString();
//         entity.firstMessage = query.value("first_message").toString();
//         entity.exampleDialogue = query.value("example_dialogue").toString();
//         entity.systemPrompt = query.value("system_prompt").toString();
//         entity.creatorNotes = query.value("creator_notes").toString();

//         QByteArray extJson = query.value("extensions").toByteArray();
//         if (!extJson.isEmpty()) {
//             QJsonDocument doc = QJsonDocument::fromJson(extJson);
//             if (doc.isObject()) {
//                 entity.extensions = doc.object().toVariantMap();
//             }
//         }

//         entity.avatarPath = query.value("avatar_path").toString();
//         entity.version = query.value("version").toInt();
//         entity.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
//         entity.updatedAt = QDateTime::fromString(query.value("updated_at").toString(), Qt::ISODate);

//         result.append(entity);
//     }

//     return result;
// }

// CharacterEntity CharacterDao::selectById(const QString& id)
// {
//     CharacterEntity entity;
//     QSqlDatabase db = connection();
//     QSqlQuery query(db);

//     const QString sql = QStringLiteral(R"(
//         SELECT id, name, description, personality, first_message, example_dialogue,
//                system_prompt, creator_notes, extensions, avatar_path, version,
//                created_at, updated_at
//         FROM characters
//         WHERE id = :id
//     )");

//     query.prepare(sql);
//     query.bindValue(":id", id);

//     if (!query.exec()) {
//         qWarning() << "[CharacterDao] selectById error:" << query.lastError().text();
//         return entity;
//     }

//     if (query.next()) {
//         entity.id = query.value("id").toString();
//         entity.name = query.value("name").toString();
//         entity.description = query.value("description").toString();
//         entity.personality = query.value("personality").toString();
//         entity.firstMessage = query.value("first_message").toString();
//         entity.exampleDialogue = query.value("example_dialogue").toString();
//         entity.systemPrompt = query.value("system_prompt").toString();
//         entity.creatorNotes = query.value("creator_notes").toString();

//         QByteArray extJson = query.value("extensions").toByteArray();
//         if (!extJson.isEmpty()) {
//             QJsonDocument doc = QJsonDocument::fromJson(extJson);
//             if (doc.isObject()) {
//                 entity.extensions = doc.object().toVariantMap();
//             }
//         }

//         entity.avatarPath = query.value("avatar_path").toString();
//         entity.version = query.value("version").toInt();
//         entity.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
//         entity.updatedAt = QDateTime::fromString(query.value("updated_at").toString(), Qt::ISODate);
//     }

//     return entity;
// }

// bool CharacterDao::insert(const CharacterEntity& character)
// {
//     QSqlDatabase db = connection();
//     QSqlQuery query(db);

//     const QString sql = QStringLiteral(R"(
//         INSERT INTO characters (
//             id, name, description, personality, first_message, example_dialogue,
//             system_prompt, creator_notes, extensions, avatar_path, version,
//             created_at, updated_at
//         ) VALUES (
//             :id, :name, :description, :personality, :firstMessage, :exampleDialogue,
//             :systemPrompt, :creatorNotes, :extensions, :avatarPath, :version,
//             :createdAt, :updatedAt
//         )
//     )");

//     query.prepare(sql);
//     query.bindValue(":id", character.id.isEmpty() ? Utils::generateUuid() : character.id);
//     query.bindValue(":name", character.name);
//     query.bindValue(":description", character.description);
//     query.bindValue(":personality", character.personality);
//     query.bindValue(":firstMessage", character.firstMessage);
//     query.bindValue(":exampleDialogue", character.exampleDialogue);
//     query.bindValue(":systemPrompt", character.systemPrompt);
//     query.bindValue(":creatorNotes", character.creatorNotes);

//     QJsonDocument extDoc = QJsonDocument::fromVariant(character.extensions);
//     query.bindValue(":extensions", extDoc.toJson(QJsonDocument::Compact));

//     query.bindValue(":avatarPath", character.avatarPath);
//     query.bindValue(":version", character.version);

//     QDateTime now = QDateTime::currentDateTime();
//     query.bindValue(":createdAt", character.createdAt.isValid() ? character.createdAt.toString(Qt::ISODate) : now.toString(Qt::ISODate));
//     query.bindValue(":updatedAt", character.updatedAt.isValid() ? character.updatedAt.toString(Qt::ISODate) : now.toString(Qt::ISODate));

//     return query.exec();
// }

// bool CharacterDao::update(const CharacterEntity& character)
// {
//     QSqlDatabase db = connection();
//     QSqlQuery query(db);

//     const QString sql = QStringLiteral(R"(
//         UPDATE characters SET
//             name = :name,
//             description = :description,
//             personality = :personality,
//             first_message = :firstMessage,
//             example_dialogue = :exampleDialogue,
//             system_prompt = :systemPrompt,
//             creator_notes = :creatorNotes,
//             extensions = :extensions,
//             avatar_path = :avatarPath,
//             version = :version,
//             updated_at = :updatedAt
//         WHERE id = :id
//     )");

//     query.prepare(sql);
//     query.bindValue(":id", character.id);
//     query.bindValue(":name", character.name);
//     query.bindValue(":description", character.description);
//     query.bindValue(":personality", character.personality);
//     query.bindValue(":firstMessage", character.firstMessage);
//     query.bindValue(":exampleDialogue", character.exampleDialogue);
//     query.bindValue(":systemPrompt", character.systemPrompt);
//     query.bindValue(":creatorNotes", character.creatorNotes);

//     QJsonDocument extDoc = QJsonDocument::fromVariant(character.extensions);
//     query.bindValue(":extensions", extDoc.toJson(QJsonDocument::Compact));

//     query.bindValue(":avatarPath", character.avatarPath);
//     query.bindValue(":version", character.version);
//     query.bindValue(":updatedAt", QDateTime::currentDateTime().toString(Qt::ISODate));

//     return query.exec();
// }

// bool CharacterDao::remove(const QString& id)
// {
//     QSqlDatabase db = connection();
//     QSqlQuery query(db);

//     query.prepare("DELETE FROM characters WHERE id = :id");
//     query.bindValue(":id", id);

//     return query.exec();
// }

// bool CharacterDao::exists(const QString& id)
// {
//     QSqlDatabase db = connection();
//     QSqlQuery query(db);

//     query.prepare("SELECT COUNT(*) FROM characters WHERE id = :id");
//     query.bindValue(":id", id);

//     if (!query.exec() || !query.next()) {
//         return false;
//     }

//     return query.value(0).toInt() > 0;
// }