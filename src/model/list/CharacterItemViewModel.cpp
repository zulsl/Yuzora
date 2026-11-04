#include "CharacterItemViewModel.h"
#include "Utils.h"
#include <QDateTime>
#include <QDir>

CharacterItemViewModel::CharacterItemViewModel(QObject* parent)
    : QObject(parent)
{
}

CharacterItemViewModel::~CharacterItemViewModel() = default;

QString CharacterItemViewModel::id() const
{
    return m_id;
}

QString CharacterItemViewModel::name() const
{
    return m_name;
}

QString CharacterItemViewModel::description() const
{
    return m_description;
}

QString CharacterItemViewModel::avatarPath() const
{
    return Utils::avatarPathToUrl(m_avatarRelativePath);

}

bool CharacterItemViewModel::isFavorite() const
{
    return m_isFavorite;
}

QString CharacterItemViewModel::lastUsedText() const
{
    if (!m_lastUsedAt.isValid()) {
        return "从未使用";
    }
    return formatFriendlyTime(m_lastUsedAt);
}

QString CharacterItemViewModel::updateTimeText() const
{
    if (!m_updatedAt.isValid()) {
        return "";
    }
    return formatFriendlyTime(m_updatedAt);
}

int CharacterItemViewModel::useCount() const
{
    return m_useCount;
}

void CharacterItemViewModel::updateFromEntity(const CharacterEntity& entity)
{
    m_id = entity.id;
    m_name = entity.name;
    m_description = entity.description;
    m_avatarRelativePath = entity.avatarPath;
    m_isFavorite = entity.isFavorite;
    m_updatedAt = entity.updatedAt;

    emit dataChanged();
}

// 时间友好格式化：分钟/小时/天级展示，超过7天显示完整日期
QString CharacterItemViewModel::formatFriendlyTime(const QDateTime& time) const
{
    QDateTime now = QDateTime::currentDateTime();
    qint64 secs = time.secsTo(now);

    if (secs < 0) {
        return time.toString("yyyy-MM-dd");
    }
    if (secs < 60) {
        return "刚刚";
    }
    if (secs < 3600) {
        return QString("%1分钟前").arg(secs / 60);
    }
    if (secs < 86400) {
        return QString("%1小时前").arg(secs / 3600);
    }
    if (secs < 7 * 86400) {
        return QString("%1天前").arg(secs / 86400);
    }
    return time.toString("yyyy-MM-dd");
}