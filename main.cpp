#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QIcon>
#include <qqml.h>

#include "ExceptionHandler.h"
#include "GlobalConstants.h"
#include "Utils.h"
#include "DatabaseManager.h"
#include "DeviceHelper.h"

// ======================================
// 视图层与业务层的边界
// ======================================
// 6 个 ViewModel 已改为 QML 单例（QML_ELEMENT + QML_SINGLETON），定义在 src/viewmodel/ 下，
// 由 QML 引擎在首次被引用时构造并托管生命周期，因此这里不再手工 new，也不再注入上下文属性。
// DeviceHelper / ExceptionHandler 是"私有构造 + instance()"的单例，无法交给 QML 自动构造，
// 故改用 qmlRegisterSingletonInstance 注册成同名 QML 单例，QML 侧写法保持不变。

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

//设置应用基本信息
    app.setWindowIcon(QIcon(":/icons/res/app/favicon-48x48.png"));
    app.setApplicationName(GlobalConstants::APP_NAME);//设置应用名称
    app.setApplicationVersion(GlobalConstants::APP_VERSION.toString());//设置应用版本
    //弃用app.setOrganizationName("zzds");//设置组织名(公司名)
    app.setOrganizationDomain("com.qtyuzora");//域名(macOs偏好文件)

//全局异常处理器和跨端适配层初始化
    ExceptionHandler& exceptionHandler = ExceptionHandler::instance();
    DeviceHelper& deviceHelper = DeviceHelper::instance();

//确保应用数据目录存在
    Utils::getAppDataPath();
    Utils::getAvatarDirPath();

//初始化数据库（主线程建表、创建连接池基础配置）
    bool dbInitOk = DatabaseManager::instance().initialize();
    if (!dbInitOk) {
        //致命错误
        qFatal("数据库初始化失败，程序退出。");
        return -1;
    }

// ========== QML引擎初始化 ==========
    QQmlApplicationEngine engine;

//加载主 QML 界面
    //==监听启动失败信号,失败退出程序==
    QObject::connect(
        &engine,&QQmlApplicationEngine::objectCreationFailed,
        &app,[]() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("Yuzora", "Main");

    return app.exec();
}
