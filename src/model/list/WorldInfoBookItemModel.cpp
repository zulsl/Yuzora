#include "WorldInfoBookItemModel.h"

WorldInfoBookItemModel::WorldInfoBookItemModel(const WorldInfoBookEntity& entity, QObject* parent)
    : QObject(parent)
    , m_id(entity.id)
    , m_name(entity.name)
    , m_isEnabled(entity.isEnabled)
{
}

QString WorldInfoBookItemModel::id() const
{
    return m_id;
}

QString WorldInfoBookItemModel::name() const
{
    return m_name;
}

QString WorldInfoBookItemModel::description() const
{
    return m_description;
}

bool WorldInfoBookItemModel::isEnabled() const
{
    return m_isEnabled;
}

bool WorldInfoBookItemModel::isGlobal() const
{
    return m_isGlobal;
}