#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QIcon>
#include <qqml.h>

#include "ExceptionHandler.h"
#include "GlobalConstants.h"
#include "Utils.h"
#include "DatabaseManager.h"
#include "ApplicationBootstrap.h"
#include "DeviceHelper.h"

// ======================================
// 视图层与业务层的边界
// ======================================
// 6 个 ViewModel + DeviceHelper / ExceptionHandler 都以 QML 单例形式暴露：
// 前者用 QML_ELEMENT + QML_SINGLETON（create() 里把静态实例标记为 CppOwnership），
// 后者是"私有构造 + instance()"，同样在 create() 里返回该静态实例。
// 关键：create() 返回的静态实例必须 setObjectOwnership(CppOwnership)，
// 否则 QML 引擎会在析构时 delete 它们，退出阶段再被静态析构销毁一次，
// 表现为"关窗后进程卡死"。

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

// 必须显式选择非原生样式：
//   1) 本项目大量自定义 background / contentItem，Windows 原生样式不支持定制；
//   2) Material.theme / Material.primary / Material.accent 只在 Material 样式下生效。
// 必须在加载 QML 之前调用。
    QQuickStyle::setStyle(QStringLiteral("Material"));

//设置应用基本信息
    // 窗口图标按尺寸各注册一张：任务栏 32px、Alt+Tab 48px、高 DPI 用 64/128，
    // 比只给一张 256px 让 Qt 现场缩小要清晰。exe 文件图标由 app.rc 单独嵌入。
    QIcon appIcon;
    for (const int size : {16, 20, 24, 32, 40, 48, 64, 96, 128, 256}) {
        appIcon.addFile(QStringLiteral(":/icons/res/app/yuzora-%1.png").arg(size),
                        QSize(size, size));
    }
    app.setWindowIcon(appIcon);//窗口图标
    app.setApplicationName(GlobalConstants::APP_NAME);//内部文件统一名称,不可更改
    app.setApplicationDisplayName("语昼");//显示名称
    app.setApplicationVersion(APP_VERSION);
    app.setOrganizationDomain("com.qtyuzora");

//全局异常处理器和跨端适配层初始化
    ExceptionHandler& exceptionHandler = ExceptionHandler::instance();
    DeviceHelper& deviceHelper = DeviceHelper::instance();
    Q_UNUSED(exceptionHandler)
    Q_UNUSED(deviceHelper)

//确保应用数据目录存在
    Utils::getAppDataPath();
    Utils::getAvatarDirPath();

//初始化数据库，并在加载 QML 前写入种子数据
    if (!ApplicationBootstrap::initialize()) {
        qFatal("数据库初始化失败，程序退出。");
        return -1;
    }

// ========== QML引擎初始化 ==========
    QQmlApplicationEngine engine;

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("Yuzora", "Main");

    return app.exec();
}
