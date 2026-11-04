#include "AppException.h"
#include <QDebug>

AppException::AppException(ErrorCode code, const QString& message)
    : m_code(code)
    , m_message(message)
    , m_whatBuffer(message.toUtf8())
{
    if (m_whatBuffer.isEmpty()) m_whatBuffer = "AppException";
}

const char* AppException::what() const noexcept
{
    return m_whatBuffer.constData();
}

AppException::ErrorCode AppException::getCode() const
{
    return m_code;
}

QString AppException::getMessage() const
{
    return m_message;
}

QString AppException::errorCodeToString(ErrorCode code)
{
    switch (code) {
    case ErrorCode::NetworkError:      return "NetworkError";
    case ErrorCode::FileIOError:       return "FileIOError";
    case ErrorCode::JsonParseError:    return "JsonParseError";
    case ErrorCode::DatabaseError:     return "DatabaseError";
    case ErrorCode::CharacterCardError:return "CharacterCardError";
    case ErrorCode::ModelError:        return "ModelError";
    case ErrorCode::PermissionError:   return "PermissionError";
    case ErrorCode::OperationError:   return "OperationError";
    case ErrorCode::UnknownError:      return "UnknownError";
    }
    return "Unknown";
}
