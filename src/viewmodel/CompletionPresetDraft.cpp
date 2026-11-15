#include "CompletionPresetDraft.h"

CompletionPresetDraft::CompletionPresetDraft(QObject* parent)
    : QObject(parent)
{
}

void CompletionPresetDraft::setId(const QString& value)
{
    if (m_id == value) return;
    m_id = value;
    if (!m_updating) { emit idChanged(); emit changed(); }
}

void CompletionPresetDraft::setName(const QString& value)
{
    if (m_name == value) return;
    m_name = value;
    if (!m_updating) { emit nameChanged(); emit changed(); }
}

void CompletionPresetDraft::setIsDefault(bool value)
{
    if (m_isDefault == value) return;
    m_isDefault = value;
    if (!m_updating) { emit isDefaultChanged(); emit changed(); }
}

void CompletionPresetDraft::setMaxContextToken(int value)
{
    if (m_maxContextToken == value) return;
    m_maxContextToken = value;
    if (!m_updating) { emit maxContextTokenChanged(); emit changed(); }
}

void CompletionPresetDraft::setMaxResponseToken(int value)
{
    if (m_maxResponseToken == value) return;
    m_maxResponseToken = value;
    if (!m_updating) { emit maxResponseTokenChanged(); emit changed(); }
}

void CompletionPresetDraft::setStreamEnabled(bool value)
{
    if (m_streamEnabled == value) return;
    m_streamEnabled = value;
    if (!m_updating) { emit streamEnabledChanged(); emit changed(); }
}

void CompletionPresetDraft::setTemperature(double value)
{
    if (m_temperature == value) return;
    m_temperature = value;
    if (!m_updating) { emit temperatureChanged(); emit changed(); }
}

void CompletionPresetDraft::setTopP(double value)
{
    if (m_topP == value) return;
    m_topP = value;
    if (!m_updating) { emit topPChanged(); emit changed(); }
}

void CompletionPresetDraft::setTopK(double value)
{
    if (m_topK == value) return;
    m_topK = value;
    if (!m_updating) { emit topKChanged(); emit changed(); }
}

void CompletionPresetDraft::setFrequencyPenalty(double value)
{
    if (m_frequencyPenalty == value) return;
    m_frequencyPenalty = value;
    if (!m_updating) { emit frequencyPenaltyChanged(); emit changed(); }
}

void CompletionPresetDraft::setPresencePenalty(double value)
{
    if (m_presencePenalty == value) return;
    m_presencePenalty = value;
    if (!m_updating) { emit presencePenaltyChanged(); emit changed(); }
}

void CompletionPresetDraft::setEntity(const CompletionPresetEntity& entity)
{
    m_updating = true;
    setId(entity.id);
    setName(entity.name);
    setIsDefault(entity.isActive);
    setMaxContextToken(entity.maxContextToken);
    setMaxResponseToken(entity.maxResponseToken);
    setStreamEnabled(entity.streamEnabled);
    setTemperature(entity.temperature);
    setTopP(entity.topP);
    setTopK(entity.topK);
    setFrequencyPenalty(entity.frequencyPenalty);
    setPresencePenalty(entity.presencePenalty);
    m_updating = false;
    emitAllChanged();
}

CompletionPresetEntity CompletionPresetDraft::toEntity(
    const CompletionPresetEntity& base) const
{
    CompletionPresetEntity entity = base;
    entity.id = m_id;
    entity.name = m_name;
    entity.isActive = m_isDefault;
    entity.maxContextToken = m_maxContextToken;
    entity.maxResponseToken = m_maxResponseToken;
    entity.streamEnabled = m_streamEnabled;
    entity.temperature = m_temperature;
    entity.topP = m_topP;
    entity.topK = m_topK;
    entity.frequencyPenalty = m_frequencyPenalty;
    entity.presencePenalty = m_presencePenalty;
    return entity;
}

bool CompletionPresetDraft::equals(const CompletionPresetEntity& entity) const
{
    return m_id == entity.id
        && m_name == entity.name
        && m_isDefault == entity.isActive
        && m_maxContextToken == entity.maxContextToken
        && m_maxResponseToken == entity.maxResponseToken
        && m_streamEnabled == entity.streamEnabled
        && m_temperature == entity.temperature
        && m_topP == entity.topP
        && m_topK == entity.topK
        && m_frequencyPenalty == entity.frequencyPenalty
        && m_presencePenalty == entity.presencePenalty;
}

QVariant CompletionPresetDraft::fieldValue(const QString& field) const
{
    if (field == "id") return m_id;
    if (field == "name") return m_name;
    if (field == "isDefault") return m_isDefault;
    if (field == "maxContextToken") return m_maxContextToken;
    if (field == "maxResponseToken") return m_maxResponseToken;
    if (field == "streamEnabled") return m_streamEnabled;
    if (field == "temperature") return m_temperature;
    if (field == "topP") return m_topP;
    if (field == "topK") return m_topK;
    if (field == "frequencyPenalty") return m_frequencyPenalty;
    if (field == "presencePenalty") return m_presencePenalty;
    return {};
}

bool CompletionPresetDraft::setField(const QString& field, const QVariant& value)
{
    if (field == "id") { setId(value.toString()); return true; }
    if (field == "name") { setName(value.toString()); return true; }
    if (field == "isDefault") { setIsDefault(value.toBool()); return true; }
    if (field == "maxContextToken") { setMaxContextToken(value.toInt()); return true; }
    if (field == "maxResponseToken") { setMaxResponseToken(value.toInt()); return true; }
    if (field == "streamEnabled") { setStreamEnabled(value.toBool()); return true; }
    if (field == "temperature") { setTemperature(value.toDouble()); return true; }
    if (field == "topP") { setTopP(value.toDouble()); return true; }
    if (field == "topK") { setTopK(value.toDouble()); return true; }
    if (field == "frequencyPenalty") { setFrequencyPenalty(value.toDouble()); return true; }
    if (field == "presencePenalty") { setPresencePenalty(value.toDouble()); return true; }
    return false;
}

void CompletionPresetDraft::emitAllChanged()
{
    emit idChanged();
    emit nameChanged();
    emit isDefaultChanged();
    emit maxContextTokenChanged();
    emit maxResponseTokenChanged();
    emit streamEnabledChanged();
    emit temperatureChanged();
    emit topPChanged();
    emit topKChanged();
    emit frequencyPenaltyChanged();
    emit presencePenaltyChanged();
    emit changed();
}
