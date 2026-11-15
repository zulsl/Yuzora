#ifndef CHATMESSAGEGROUP_H
#define CHATMESSAGEGROUP_H

#include <QObject>
#include <QJsonArray>
#include <QVariantMap>

#include "GlobalConstants.h"
#include "ChatMessage.h"

struct SMessage {
    QString identifier;//来源,便于调试
    GlobalConstants::MessageType role; // "system" / "user" / "assistant" / "tool"
    QString content;
    QString name;         // 可选
    int tokens = 0;
    bool injected = false;   // 是否是注入的（debug 用）
    bool marker = false;     // 是否是 marker 占位（chatHistory / dialogueExamples）
};

class ChatMessageGroup{//消息集合,方便插入/定位
public:
    ChatMessageGroup() = default;
    explicit ChatMessageGroup(const QString& id) : identifier(id) {}

    void add(const SMessage& m) { collection.append(m); }

    int getTokens() const {//获取当前消息组tokens的和
        int total = 0;
        for (const auto& m : collection) total += m.tokens;
        return total;
    }

    bool isEmpty() const { return collection.isEmpty(); }

    QList<ChatMessage> getChat() const {
        QList<ChatMessage> chat;
        for (const auto& m : collection) {
            if (m.content.isEmpty()) continue;
            ChatMessage c;
            c.role = m.role;
            c.appendText(m.content);
            c.senderName = m.name;
            chat.append(c);
        }
        return chat;
    }

    QString identifier;
    QList<SMessage> collection;
};

#endif