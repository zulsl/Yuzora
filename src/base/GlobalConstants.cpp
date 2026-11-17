#include "GlobalConstants.h"

namespace GlobalConstants
{
    QString messageTypeToString(MessageType type)
    {
        switch (type)
        {
        case MessageType::User: return "user";
        case MessageType::Assistant: return "assistant";
        case MessageType::System: return "system";
        default: return "user";
        }
    }

    MessageType stringToMessageType(const QString &str)
    {
        if (str == "Assistant") return MessageType::Assistant;
        if (str == "System") return MessageType::System;
        return MessageType::User;
    }

}


