#include "ChatMessage.h"

ChatMessage ChatMessage::makeText(GlobalConstants::MessageType role, const QString& text)
{
    ChatMessage msg;
    msg.role = role;
    ChatMessagePart part;
    part.type = ChatMessagePart::Type::Text;
    part.data = text;
    msg.parts.append(part);
    return msg;
}

QString ChatMessage::textContent() const
{
    QStringList texts;
    for (const ChatMessagePart& part : parts) {
        if (part.type == ChatMessagePart::Type::Text) {
            texts.append(part.data);
        }
    }
    return texts.join("\n");
}

void ChatMessage::appendText(const QString& text)
{
    ChatMessagePart part;
    part.type = ChatMessagePart::Type::Text;
    part.data = text;
    parts.append(part);
}