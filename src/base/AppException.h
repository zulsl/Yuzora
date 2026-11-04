#ifndef APPEXCEPTION_H
#define APPEXCEPTION_H

#include <QString>
#include <QByteArray>
#include <exception>

//异常类

class AppException : public std::exception
{
public:

    //错误码
    enum class ErrorCode {
        NetworkError,           //网络错误
        FileIOError,            //文件读写错误
        JsonParseError,         //Json解析错误
        DatabaseError,          //数据库错误
        CharacterCardError,     //角色卡错误
        ModelError,             //AI模型错误
        PermissionError,        //权限错误
        OperationError,         //操作错误
        UnknownError            //未知错误
    };

    explicit AppException(ErrorCode code, const QString& message);

    // 重写标准异常的what()方法
    const char* what() const noexcept override;

    // 只读获取方法
    ErrorCode getCode() const;
    QString getMessage() const;

    static QString errorCodeToString(ErrorCode code);

private:
    ErrorCode m_code;//错误码
    QString m_message;//错误信息
    QByteArray m_whatBuffer;
};

// THROW宏依赖AppException,所以放在这里,简化抛出异常的写法
#define THROW(code, msg) throw AppException(AppException::ErrorCode::code, msg)

#endif
