#ifndef CHATMESSAGEITEMVIEWMODEL_H
#define CHATMESSAGEITEMVIEWMODEL_H

#include <QObject>
#include "GlobalConstants.h"

class ChatMessageItemModel : public QObject {
    Q_OBJECT
    // Q_PROPERTY(QString id READ id CONSTANT)
    // Q_PROPERTY(int messageType READ messageType CONSTANT)
    // Q_PROPERTY(int status READ status NOTIFY statusChanged)
    // Q_PROPERTY(QString content READ content NOTIFY contentChanged)
    // Q_PROPERTY(QString senderName READ senderName CONSTANT)
    // Q_PROPERTY(QString timeText READ timeText CONSTANT)
    // Q_PROPERTY(bool isEdited READ isEdited CONSTANT)
    // Q_PROPERTY(bool isHidden READ isHidden CONSTANT)

public:
    explicit ChatMessageItemModel(QObject* parent = nullptr);

    QString id() const;
    void setId(const QString& id);

    int messageType() const;
    void setMessageType(GlobalConstants::MessageType type);

    int status() const;
    void setStatus(GlobalConstants::MessageGenerationStatus status);

    QString content() const;
    void setContent(const QString& content);
    void appendContent(const QString& chunk);

    QString senderName() const;
    void setSenderName(const QString& name);

    QString timeText() const;
    void setTimeText(const QString& text);

    bool isEdited() const;
    void setIsEdited(bool edited);

    bool isHidden() const;
    void setIsHidden(bool hidden);

signals:
    void contentChanged();
    void statusChanged();

private:
    QString m_id;
    GlobalConstants::MessageType m_type;
    GlobalConstants::MessageGenerationStatus m_status;
    QString m_content;
    QString m_senderName;
    QString m_timeText;
    bool m_isEdited = false;
    bool m_isHidden = false;
};

#endif // CHATMESSAGEITEMVIEWMODEL_H