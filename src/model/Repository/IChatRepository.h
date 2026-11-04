#ifndef ICHATREPOSITORY_H
#define ICHATREPOSITORY_H

#include <QList>
#include "../entity/ChatSessionEntity.h"
#include "../entity/ChatMessageEntity.h"


//抽象仓库层

class IChatRepository {
public:
    virtual ~IChatRepository() = default;

    // 会话操作
    virtual QList<ChatSessionEntity> selectSessionsByCharacterId(const QString& characterId) = 0;
    virtual ChatSessionEntity selectSessionById(const QString& id) = 0;
    virtual QString insertSession(const ChatSessionEntity& session) = 0;
    virtual bool updateSession(const ChatSessionEntity& session) = 0;
    virtual bool removeSession(const QString& id) = 0;

    // 消息操作
    virtual QList<ChatMessageEntity> selectMessagesBySessionId(const QString& sessionId) = 0;
    virtual ChatMessageEntity selectMessageById(const QString& id) = 0;
    virtual QString insertMessage(const ChatMessageEntity& message) = 0;
    virtual bool updateMessage(const ChatMessageEntity& message) = 0;
    virtual bool removeMessage(const QString& id) = 0;
    virtual bool clearMessagesBySessionId(const QString& sessionId) = 0;
};

#endif // ICHATREPOSITORY_H