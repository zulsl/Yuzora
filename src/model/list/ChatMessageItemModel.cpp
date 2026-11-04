#include "ChatMessageItemModel.h"
#include "GlobalConstants.h"

// ======================================
// 构造与析构
// ======================================
ChatMessageItemModel::ChatMessageItemModel(QObject* parent)
    : QObject(parent)
    , m_type(GlobalConstants::MessageType::User)
    , m_status(GlobalConstants::MessageGenerationStatus::Normal)
    , m_isEdited(false)
    , m_isHidden(false)
{
}

// ======================================
// 基础属性读写
// ======================================
QString ChatMessageItemModel::id() const
{
    return m_id;
}

void ChatMessageItemModel::setId(const QString& id)
{
    m_id = id;
}

int ChatMessageItemModel::messageType() const
{
    return static_cast<int>(m_type);
}

void ChatMessageItemModel::setMessageType(GlobalConstants::MessageType type)
{
    m_type = type;
}

int ChatMessageItemModel::status() const
{
    return static_cast<int>(m_status);
}

void ChatMessageItemModel::setStatus(GlobalConstants::MessageGenerationStatus status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged();
    }
}

QString ChatMessageItemModel::content() const
{
    return m_content;
}

void ChatMessageItemModel::setContent(const QString& content)
{
    if (m_content != content) {
        m_content = content;
        emit contentChanged();
    }
}

// ======================================
// 流式输出专用：增量追加内容
// ======================================
void ChatMessageItemModel::appendContent(const QString& chunk)
{
    if (Q_LIKELY(!chunk.isEmpty())) {
        m_content += chunk;
        emit contentChanged();
    }
}

// ======================================
// 附属属性读写
// ======================================
QString ChatMessageItemModel::senderName() const
{
    return m_senderName;
}

void ChatMessageItemModel::setSenderName(const QString& name)
{
    m_senderName = name;
}

QString ChatMessageItemModel::timeText() const
{
    return m_timeText;
}

void ChatMessageItemModel::setTimeText(const QString& text)
{
    m_timeText = text;
}

bool ChatMessageItemModel::isEdited() const
{
    return m_isEdited;
}

void ChatMessageItemModel::setIsEdited(bool edited)
{
    m_isEdited = edited;
}

bool ChatMessageItemModel::isHidden() const
{
    return m_isHidden;
}

void ChatMessageItemModel::setIsHidden(bool hidden)
{
    m_isHidden = hidden;
}