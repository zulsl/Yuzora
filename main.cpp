#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QIcon>

#include "ExceptionHandler.h"
#include "GlobalConstants.h"
#include "Utils.h"
#include "DatabaseManager.h"
#include "ChatViewModel.h"
#include "DeviceHelper.h"
#include "ApiConfigViewModel.h"
#include "CharacterViewModel.h"
#include "SettingsViewModel.h"
#include "CompletionPresetViewModel.h"
#include "WorldInfoViewModel.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

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

// ========== 创建ViewModel实例（UI专属，生命周期跟随QML引擎） ==========
    auto* chatViewModel = new ChatViewModel();
    auto* characterViewModel = new CharacterViewModel();
    auto* completionPresetViewModel = new CompletionPresetViewModel();
    auto* apiConfigViewModel = new ApiConfigViewModel();
    auto* worldInfoViewModel = new WorldInfoViewModel();
    auto* settingsViewModel = new SettingsViewModel();


// ========== QML引擎初始化与全局注册 ==========
    QQmlApplicationEngine engine;

    // // 注册全局上下文属性（View层仅通过这些对象与业务层交互）
    engine.rootContext()->setContextProperty("DeviceHelper", &deviceHelper);
    engine.rootContext()->setContextProperty("ExceptionHandler", &exceptionHandler);

    engine.rootContext()->setContextProperty("ApiConfigViewModel", apiConfigViewModel);
    engine.rootContext()->setContextProperty("ChatViewModel", chatViewModel);
    engine.rootContext()->setContextProperty("CharacterViewModel", characterViewModel);
    engine.rootContext()->setContextProperty("CompletionPresetViewModel", completionPresetViewModel);
    engine.rootContext()->setContextProperty("WorldInfoViewModel", worldInfoViewModel);
    engine.rootContext()->setContextProperty("SettingsViewModel", settingsViewModel);

//加载主 QML 界面
    //==监听启动失败信号,失败退出程序==
    QObject::connect(
        &engine,&QQmlApplicationEngine::objectCreationFailed,
        &app,[]() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("Yuzora", "Main");

    // 绑定ViewModel与QML引擎生命周期，自动释放内存
    chatViewModel->setParent(&engine);
    characterViewModel->setParent(&engine);
    completionPresetViewModel->setParent(&engine);
    apiConfigViewModel->setParent(&engine);
    worldInfoViewModel->setParent(&engine);
    settingsViewModel->setParent(&engine);



    return app.exec();
}
