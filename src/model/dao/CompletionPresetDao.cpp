#include "CompletionPresetDao.h"
#include "DatabaseManager.h"
#include "Utils.h"

CompletionPresetDao::CompletionPresetDao(QObject* parent)
    : QObject(parent)
{
}

bool CompletionPresetDao::setDefault(const QString &id)
{
    if (id.isEmpty()) return false;
    QSqlDatabase db = connection();
    if (!db.transaction()) return false;

    QSqlQuery clearQuery(db);
    if (!clearQuery.exec("UPDATE completion_presets SET is_default = 0")) {
        db.rollback();
        return false;
    }

    QSqlQuery setQuery(db);
    setQuery.prepare("UPDATE completion_presets SET is_default = 1 WHERE id = ?");
    setQuery.addBindValue(id);
    if (!setQuery.exec()) {
        db.rollback();
        return false;
    }

    return db.commit();
}

bool CompletionPresetDao::clearAllDefault()
{
    QSqlDatabase db = connection();
    QSqlQuery query(db);
    return query.exec("UPDATE completion_presets SET is_default = 0");
}

CompletionPresetEntity CompletionPresetDao::selectDefault()
{
    CompletionPresetEntity entity;
    QSqlDatabase db = connection();
    QSqlQuery query(db);
    query.exec("SELECT * FROM completion_presets WHERE is_default = 1 LIMIT 1");
    if (query.next()) {
        QString id = query.value("id").toString();
        return selectById(id);
    }
    return entity;
}

bool CompletionPresetDao::rename(const QString &id, const QString &newName)
{
    if (id.isEmpty() || newName.isEmpty()) return false;
    QSqlDatabase db = connection();
    QSqlQuery query(db);
    query.prepare("UPDATE completion_presets SET name = ?, updated_at = ? WHERE id = ?");
    query.addBindValue(newName);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.addBindValue(id);
    return query.exec() && query.numRowsAffected() > 0;
}

QString CompletionPresetDao::tableName() const
{
    return "completion_presets";
}

QString CompletionPresetDao::primaryKey() const
{
    return "id";
}

QStringList CompletionPresetDao::columnNames() const
{
    return {
        "id", "name", "is_default","max_context_token", "max_response_token", "stream_enabled",
        "temperature", "top_p", "top_k", "frequency_penalty", "presence_penalty",
        "repetition_penalty", "top_a", "min_p","extra_settings", "created_at", "updated_at",
        "max_context_unlocked","names_behavior","continue_postfix","continue_prefill",
        "squash_system_messages","function_calling","show_thoughts","prompt_order",
        "impersonation_Prompt","wi_Format","scenario_Format","personality_Format",
        "group_nudge_prompt","new_chat_prompt","new_group_chat_prompt",
        "new_example_chat_prompt","continue_nudge_prompt"
    };
}

void CompletionPresetDao::bindValues(QSqlQuery &query, const CompletionPresetEntity &entity) const
{
    //严格顺序
    query.addBindValue(entity.id);
    query.addBindValue(entity.name);
    query.addBindValue(entity.isActive ? 1 : 0);
    query.addBindValue(entity.maxContextToken);
    query.addBindValue(entity.maxResponseToken);
    query.addBindValue(entity.streamEnabled ? 1 : 0);

    query.addBindValue(entity.temperature);
    query.addBindValue(entity.topP);
    query.addBindValue(entity.topK);
    query.addBindValue(entity.frequencyPenalty);
    query.addBindValue(entity.presencePenalty);
    query.addBindValue(entity.repetitionPenalty);
    query.addBindValue(entity.topA);
    query.addBindValue(entity.minP);

    query.addBindValue(Utils::variantMapToJson(entity.extraSettings));
    query.addBindValue(entity.createdAt.toString(Qt::ISODate));
    query.addBindValue(entity.updatedAt.toString(Qt::ISODate));

    query.addBindValue(entity.maxContextUnlocked);
    query.addBindValue(entity.namesBehavior);
    query.addBindValue(entity.continuePostfix);
    query.addBindValue(entity.continuePrefill);
    query.addBindValue(entity.squashSystemMessages);
    query.addBindValue(entity.functionCalling);
    query.addBindValue(entity.showThoughts);
    query.addBindValue(Utils::stringListToJson(entity.promptOrder));
    query.addBindValue(entity.impersonationPrompt);
    query.addBindValue(entity.wiFormat);
    query.addBindValue(entity.scenarioFormat);
    query.addBindValue(entity.personalityFormat);
    query.addBindValue(entity.groupNudgePrompt);
    query.addBindValue(entity.newChatPrompt);
    query.addBindValue(entity.newGroupChatPrompt);
    query.addBindValue(entity.newExampleChatPrompt);
    query.addBindValue(entity.continueNudgePrompt);

}

CompletionPresetEntity CompletionPresetDao::parseFromQuery(const QSqlQuery &query) const
{
    CompletionPresetEntity entity;
    entity.id = query.value("id").toString();
    entity.name = query.value("name").toString();
    entity.isActive = query.value("is_default").toBool();
    entity.maxContextToken = query.value("max_context_token").toInt();
    entity.maxResponseToken = query.value("max_response_token").toInt();
    entity.streamEnabled = query.value("stream_enabled").toBool();

    entity.temperature = query.value("temperature").toDouble();
    entity.topP = query.value("top_p").toDouble();
    entity.topK = query.value("top_k").toDouble();
    entity.frequencyPenalty = query.value("frequency_penalty").toDouble();
    entity.presencePenalty = query.value("presence_penalty").toDouble();
    entity.repetitionPenalty = query.value("repetition_penalty").toDouble();
    entity.topA = query.value("top_a").toDouble();
    entity.minP = query.value("min_p").toDouble();

    entity.extraSettings = Utils::jsonToVariantMap(query.value("extra_settings"));
    entity.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
    entity.updatedAt = QDateTime::fromString(query.value("updated_at").toString(), Qt::ISODate);

    entity.maxContextUnlocked = query.value("max_context_unlocked").toBool();
    entity.namesBehavior = query.value("names_behavior").toInt();
    entity.continuePostfix = query.value("continue_postfix").toString();
    entity.continuePrefill = query.value("continue_prefill").toBool();
    entity.squashSystemMessages = query.value("squash_system_messages").toBool();
    entity.functionCalling = query.value("function_calling").toBool();
    entity.showThoughts = query.value("show_thoughts").toBool();
    entity.promptOrder = Utils::jsonToStringList(query.value("prompt_order"));
    entity.impersonationPrompt = query.value("impersonation_Prompt").toString();
    entity.wiFormat = query.value("wi_Format").toString();
    entity.scenarioFormat = query.value("scenario_Format").toString();
    entity.personalityFormat = query.value("personality_Format").toString();
    entity.groupNudgePrompt = query.value("group_nudge_prompt").toString();
    entity.newChatPrompt = query.value("new_chat_prompt").toString();
    entity.newGroupChatPrompt = query.value("new_group_chat_prompt").toString();
    entity.newExampleChatPrompt = query.value("new_example_chat_prompt").toString();
    entity.continueNudgePrompt = query.value("continue_nudge_prompt").toString();

    return entity;
}

