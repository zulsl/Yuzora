#include "ApiConfigDraft.h"

ApiConfigDraft::ApiConfigDraft(QObject* parent)
    : QObject(parent)
{
}

void ApiConfigDraft::setEntity(const ApiConfigEntity& entity)
{
    m_entity = entity;
    m_dirty = false;

    emitAllFieldChanged();
    emit dirtyChanged();
}

void ApiConfigDraft::markSaved()
{
    if (!m_dirty) return;
    m_dirty = false;
    emit dirtyChanged();
}

void ApiConfigDraft::markDirty()
{
    if (m_dirty) return;
    m_dirty = true;
    emit dirtyChanged();
}

void ApiConfigDraft::emitAllFieldChanged()
{
    emit nameChanged();
    emit apiKeyChanged();
    emit baseDomainChanged();
    emit apiPathChanged();
    emit modelListChanged();
    emit currentModelChanged();
    emit modelsIndexChanged();
    emit isBuiltinChanged();
    emit isActiveChanged();
}

void ApiConfigDraft::setName(const QString& value)
{
    if (m_entity.name == value) return;
    m_entity.name = value;
    markDirty();
    emit nameChanged();
    emit changed();
}

void ApiConfigDraft::setApiKey(const QString& value)
{
    if (m_entity.apiKey == value) return;
    m_entity.apiKey = value;
    markDirty();
    emit apiKeyChanged();
    emit changed();
}

void ApiConfigDraft::setBaseDomain(const QString& value)
{
    if (m_entity.baseDomain == value) return;
    m_entity.baseDomain = value;
    markDirty();
    emit baseDomainChanged();
    emit changed();
}

void ApiConfigDraft::setApiPath(const QString& value)
{
    if (m_entity.apiPath == value) return;
    m_entity.apiPath = value;
    markDirty();
    emit apiPathChanged();
    emit changed();
}

void ApiConfigDraft::setModelList(const QStringList& value)
{
    if (m_entity.availableModels == value) return;
    m_entity.availableModels = value;
    // 列表变了以后，原来的 currentModel 可能已经不在里面，
    // 由 ViewModel 的 ensureCurrentModelSelected() 负责兜底
    markDirty();
    emit modelListChanged();
    emit modelsIndexChanged();
    emit changed();
}

void ApiConfigDraft::setCurrentModel(const QString& value)
{
    if (m_entity.currentModel == value) return;
    m_entity.currentModel = value;
    markDirty();
    emit currentModelChanged();
    emit modelsIndexChanged();
    emit changed();
}

void ApiConfigDraft::setCurrentModelsIndex(int index)
{
    if (index < 0 || index >= m_entity.availableModels.size()) return;
    setCurrentModel(m_entity.availableModels.at(index));
}
