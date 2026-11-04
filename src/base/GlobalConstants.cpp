#include "GlobalConstants.h"

namespace GlobalConstants
{
    QString messageTypeToString(MessageType type)
    {
        switch (type)
        {
        case MessageType::User: return "User";
        case MessageType::Assistant: return "Assistant";
        case MessageType::System: return "System";
        default: return "User";
        }
    }

    MessageType stringToMessageType(const QString &str)
    {
        if (str == "Assistant") return MessageType::Assistant;
        if (str == "System") return MessageType::System;
        return MessageType::User;
    }

}


