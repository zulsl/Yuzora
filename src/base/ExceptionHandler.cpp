#include "ExceptionHandler.h"
#include "Utils.h"
#include <QDebug>

ExceptionHandler& ExceptionHandler::instance()
{
    static ExceptionHandler instance;
    return instance;
}

void ExceptionHandler::handleException(const AppException& e)
{
    //控制台打印信息
    QString logMsg = QString("捕获到 AppException - 错误码: %1, 错误信息: %2")
        // .arg(static_cast<int>(e.getCode())).arg(e.getMessage());
        .arg(AppException::errorCodeToString(e.getCode())).arg(e.getMessage());
    Utils::writeLog("ERROR","异常处理器",logMsg);

    // 发出信号，由 UI 层决定如何展示
    emit errorOccurred(e.getCode(), e.getMessage());
}

void ExceptionHandler::handleException(const std::exception& e)
{
    QString logMsg = QString("捕获到 std::exception - 错误信息: %1")
        .arg(QString::fromStdString(e.what()));
    Utils::writeLog("ERROR", "异常处理器", logMsg);

    emit errorOccurred(AppException::ErrorCode::UnknownError,QString::fromStdString(e.what()));
}

void ExceptionHandler::handleException()
{
    Utils::writeLog("ERROR", "异常处理器", "捕获到未知的非std异常");
    emit errorOccurred(AppException::ErrorCode::UnknownError,"发生未知错误");
}
