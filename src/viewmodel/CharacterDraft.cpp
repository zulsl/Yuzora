#include "CharacterDraft.h"

CharacterDraft::CharacterDraft(QObject* parent)
    : QObject(parent)
{
}

void CharacterDraft::setId(const QString& value)
{
    if (m_id == value) return;
    m_id = value;
    if (!m_updating) { emit idChanged(); emit changed(); }
}

void CharacterDraft::setName(const QString& value)
{
    if (m_name == value) return;
    m_name = value;
    if (!m_updating) { emit nameChanged(); emit changed(); }
}

void CharacterDraft::setDescription(const QString& value)
{
    if (m_description == value) return;
    m_description = value;
    if (!m_updating) { emit descriptionChanged(); emit changed(); }
}

void CharacterDraft::setFirstMessage(const QString& value)
{
    if (m_firstMessage == value) return;
    m_firstMessage = value;
    if (!m_updating) { emit firstMessageChanged(); emit changed(); }
}

void CharacterDraft::setAlternateGreetings(const QStringList& value)
{
    if (m_alternateGreetings == value) return;
    m_alternateGreetings = value;
    if (!m_updating) { emit alternateGreetingsChanged(); emit changed(); }
}

void CharacterDraft::setCreatorNotes(const QString& value)
{
    if (m_creatorNotes == value) return;
    m_creatorNotes = value;
    if (!m_updating) { emit creatorNotesChanged(); emit changed(); }
}

void CharacterDraft::setCreator(const QString& value)
{
    if (m_creator == value) return;
    m_creator = value;
    if (!m_updating) { emit creatorChanged(); emit changed(); }
}

void CharacterDraft::setPersonality(const QString& value)
{
    if (m_personality == value) return;
    m_personality = value;
    if (!m_updating) { emit personalityChanged(); emit changed(); }
}

void CharacterDraft::setScenario(const QString& value)
{
    if (m_scenario == value) return;
    m_scenario = value;
    if (!m_updating) { emit scenarioChanged(); emit changed(); }
}

void CharacterDraft::setExampleDialogue(const QString& value)
{
    if (m_exampleDialogue == value) return;
    m_exampleDialogue = value;
    if (!m_updating) { emit exampleDialogueChanged(); emit changed(); }
}

void CharacterDraft::setCharacterVersion(const QString& value)
{
    if (m_characterVersion == value) return;
    m_characterVersion = value;
    if (!m_updating) { emit characterVersionChanged(); emit changed(); }
}

void CharacterDraft::setTags(const QStringList& value)
{
    if (m_tags == value) return;
    m_tags = value;
    if (!m_updating) { emit tagsChanged(); emit changed(); }
}

void CharacterDraft::setSystemPrompt(const QString& value)
{
    if (m_systemPrompt == value) return;
    m_systemPrompt = value;
    if (!m_updating) { emit systemPromptChanged(); emit changed(); }
}

void CharacterDraft::setPostHistoryInstructions(const QString& value)
{
    if (m_postHistoryInstructions == value) return;
    m_postHistoryInstructions = value;
    if (!m_updating) { emit postHistoryInstructionsChanged(); emit changed(); }
}

void CharacterDraft::setAvatarPath(const QString& value)
{
    if (m_avatarPath == value) return;
    m_avatarPath = value;
    if (!m_updating) { emit avatarPathChanged(); emit changed(); }
}

void CharacterDraft::setWorldBookIds(const QStringList& value)
{
    if (m_worldBookIds == value) return;
    m_worldBookIds = value;
    if (!m_updating) { emit worldBookIdsChanged(); emit changed(); }
}

void CharacterDraft::setLastSessionId(const QString& value)
{
    if (m_lastSessionId == value) return;
    m_lastSessionId = value;
    if (!m_updating) { emit lastSessionIdChanged(); emit changed(); }
}

void CharacterDraft::setCardFormatVersion(int value)
{
    if (m_cardFormatVersion == value) return;
    m_cardFormatVersion = value;
    if (!m_updating) { emit cardFormatVersionChanged(); emit changed(); }
}

void CharacterDraft::setIsFavorite(bool value)
{
    if (m_isFavorite == value) return;
    m_isFavorite = value;
    if (!m_updating) { emit isFavoriteChanged(); emit changed(); }
}

void CharacterDraft::setExtensions(const QVariantMap& value)
{
    if (m_extensions == value) return;
    m_extensions = value;
    if (!m_updating) { emit extensionsChanged(); emit changed(); }
}

void CharacterDraft::setCreatedAt(const QDateTime& value)
{
    if (m_createdAt == value) return;
    m_createdAt = value;
    if (!m_updating) { emit createdAtChanged(); emit changed(); }
}

void CharacterDraft::setUpdatedAt(const QDateTime& value)
{
    if (m_updatedAt == value) return;
    m_updatedAt = value;
    if (!m_updating) { emit updatedAtChanged(); emit changed(); }
}

void CharacterDraft::setEntity(const CharacterEntity& entity)
{
    m_updating = true;
    setId(entity.id);
    setName(entity.name);
    setDescription(entity.description);
    setFirstMessage(entity.firstMessage);
    setAlternateGreetings(entity.alternateGreetings);
    setCreatorNotes(entity.creatorNotes);
    setCreator(entity.creator);
    setPersonality(entity.personality);
    setScenario(entity.scenario);
    setExampleDialogue(entity.exampleDialogue);
    setCharacterVersion(entity.characterVersion);
    setTags(entity.tags);
    setSystemPrompt(entity.systemPrompt);
    setPostHistoryInstructions(entity.postHistoryInstructions);
    setAvatarPath(entity.avatarPath);
    setWorldBookIds(entity.worldBookIds);
    setLastSessionId(entity.lastSessionId);
    setCardFormatVersion(entity.cardFormatVersion);
    setIsFavorite(entity.isFavorite);
    setExtensions(entity.extensions);
    setCreatedAt(entity.createdAt);
    setUpdatedAt(entity.updatedAt);
    m_updating = false;
    emitAllChanged();
}

CharacterEntity CharacterDraft::toEntity() const
{
    CharacterEntity entity;
    entity.id = m_id;
    entity.name = m_name;
    entity.description = m_description;
    entity.firstMessage = m_firstMessage;
    entity.alternateGreetings = m_alternateGreetings;
    entity.creatorNotes = m_creatorNotes;
    entity.creator = m_creator;
    entity.personality = m_personality;
    entity.scenario = m_scenario;
    entity.exampleDialogue = m_exampleDialogue;
    entity.characterVersion = m_characterVersion;
    entity.tags = m_tags;
    entity.systemPrompt = m_systemPrompt;
    entity.postHistoryInstructions = m_postHistoryInstructions;
    entity.avatarPath = m_avatarPath;
    entity.worldBookIds = m_worldBookIds;
    entity.lastSessionId = m_lastSessionId;
    entity.cardFormatVersion = m_cardFormatVersion;
    entity.isFavorite = m_isFavorite;
    entity.extensions = m_extensions;
    entity.createdAt = m_createdAt;
    entity.updatedAt = m_updatedAt;
    return entity;
}

QVariant CharacterDraft::fieldValue(const QString& field) const
{
    if (field == "id") return m_id;
    if (field == "name") return m_name;
    if (field == "description") return m_description;
    if (field == "firstMessage") return m_firstMessage;
    if (field == "alternateGreetings") return m_alternateGreetings;
    if (field == "creatorNotes") return m_creatorNotes;
    if (field == "creator") return m_creator;
    if (field == "personality") return m_personality;
    if (field == "scenario") return m_scenario;
    if (field == "exampleDialogue") return m_exampleDialogue;
    if (field == "characterVersion") return m_characterVersion;
    if (field == "tags") return m_tags;
    if (field == "systemPrompt") return m_systemPrompt;
    if (field == "postHistoryInstructions") return m_postHistoryInstructions;
    if (field == "avatarPath") return m_avatarPath;
    if (field == "worldBookIds") return m_worldBookIds;
    if (field == "lastSessionId") return m_lastSessionId;
    if (field == "cardFormatVersion") return m_cardFormatVersion;
    if (field == "isFavorite") return m_isFavorite;
    if (field == "extensions") return m_extensions;
    if (field == "createdAt") return m_createdAt;
    if (field == "updatedAt") return m_updatedAt;
    return {};
}

bool CharacterDraft::setField(const QString& field, const QVariant& value)
{
    if (field == "id") { setId(value.toString()); return true; }
    if (field == "name") { setName(value.toString()); return true; }
    if (field == "description") { setDescription(value.toString()); return true; }
    if (field == "firstMessage") { setFirstMessage(value.toString()); return true; }
    if (field == "alternateGreetings") { setAlternateGreetings(value.toStringList()); return true; }
    if (field == "creatorNotes") { setCreatorNotes(value.toString()); return true; }
    if (field == "creator") { setCreator(value.toString()); return true; }
    if (field == "personality") { setPersonality(value.toString()); return true; }
    if (field == "scenario") { setScenario(value.toString()); return true; }
    if (field == "exampleDialogue") { setExampleDialogue(value.toString()); return true; }
    if (field == "characterVersion") { setCharacterVersion(value.toString()); return true; }
    if (field == "tags") { setTags(value.toStringList()); return true; }
    if (field == "systemPrompt") { setSystemPrompt(value.toString()); return true; }
    if (field == "postHistoryInstructions") { setPostHistoryInstructions(value.toString()); return true; }
    if (field == "avatarPath") { setAvatarPath(value.toString()); return true; }
    if (field == "worldBookIds") { setWorldBookIds(value.toStringList()); return true; }
    if (field == "lastSessionId") { setLastSessionId(value.toString()); return true; }
    if (field == "cardFormatVersion") { setCardFormatVersion(value.toInt()); return true; }
    if (field == "isFavorite") { setIsFavorite(value.toBool()); return true; }
    if (field == "extensions") { setExtensions(value.toMap()); return true; }
    if (field == "createdAt") { setCreatedAt(value.toDateTime()); return true; }
    if (field == "updatedAt") { setUpdatedAt(value.toDateTime()); return true; }
    return false;
}

void CharacterDraft::emitAllChanged()
{
    emit idChanged();
    emit nameChanged();
    emit descriptionChanged();
    emit firstMessageChanged();
    emit alternateGreetingsChanged();
    emit creatorNotesChanged();
    emit creatorChanged();
    emit personalityChanged();
    emit scenarioChanged();
    emit exampleDialogueChanged();
    emit characterVersionChanged();
    emit tagsChanged();
    emit systemPromptChanged();
    emit postHistoryInstructionsChanged();
    emit avatarPathChanged();
    emit worldBookIdsChanged();
    emit lastSessionIdChanged();
    emit cardFormatVersionChanged();
    emit isFavoriteChanged();
    emit extensionsChanged();
    emit createdAtChanged();
    emit updatedAtChanged();
    emit changed();
}
