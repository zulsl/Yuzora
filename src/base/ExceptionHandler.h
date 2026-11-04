#ifndef EXCEPTIONHANDLER_H
#define EXCEPTIONHANDLER_H

#include <QObject>
#include "AppException.h"

//异常处理类

class ExceptionHandler : public QObject
{
    Q_OBJECT

public:
    static ExceptionHandler& instance();
    // 禁止拷贝和移动
    ExceptionHandler(const ExceptionHandler&) = delete;
    ExceptionHandler& operator=(const ExceptionHandler&) = delete;
    ExceptionHandler(ExceptionHandler&&) = delete;
    ExceptionHandler& operator=(ExceptionHandler&&) = delete;

    // 三个重载的异常处理方法
    void handleException(const AppException& e);//处理自定义异常
    void handleException(const std::exception& e);// 处理标准异常
    void handleException();// 处理未知异常（如 catch(...) ）




signals:
    // 异常发生时发出信号，UI层连接这个信号决定如何展示（弹窗、Toast、打印等）
    void errorOccurred(AppException::ErrorCode code, const QString& message);
    void errorOccurred(const QString& message);
    void tipOccurred(const QString& message);

private:
    ExceptionHandler() = default;
};

#endif
