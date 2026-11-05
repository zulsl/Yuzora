#ifndef DEVICEHELPER_H
#define DEVICEHELPER_H

#include <QObject>
#include <QSize>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

#include <QPermission>

class QJSEngine;

//除了权限适配需优化,其他全面完成
// 设备助手类（单例），负责双端设备判断、路径适配、权限处理
class DeviceHelper : public QObject
{
    /*
    Q_PROPERTY：暴露属性给 QML
    Q_INVOKABLE：暴露成员函数给 QML
    signals：声明 C++ 信号，可被 QML 接收
    slots：声明 C++ 槽函数，可被 QML 调用
    */
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // 本类是"私有构造 + instance()"的单例，QML 侧通过 create() 拿到同一个实例。
    // 必须是 public：QML 只会查找 public 的 create()。
    static DeviceHelper* create(QQmlEngine*, QJSEngine*)
    {
        DeviceHelper* inst = &instance();
        // 生命周期归 C++。若不给这个标记，QML 引擎会在析构时 delete 掉本静态单例，
        // 进程退出时静态析构再销毁一次 → 关窗后进程卡死（本次修复的核心问题）。
        QQmlEngine::setObjectOwnership(inst, QQmlEngine::CppOwnership);
        return inst;
    }
    Q_PROPERTY(bool isMobile READ isMobile CONSTANT)
    Q_PROPERTY(bool isPC READ isPC CONSTANT)
    Q_PROPERTY(bool isTablet READ isTablet CONSTANT)
    Q_PROPERTY(DeviceType deviceType READ deviceType CONSTANT)
    Q_PROPERTY(qreal screenDensity READ screenDensity CONSTANT)
    Q_PROPERTY(QSize screenSize READ screenSize CONSTANT)
    // 性能探针开关（供 QML 使用）：设置环境变量 YUZORA_PERF=1 或启动参数 --perf 时为 true。
    // 供 QML 使用。
    Q_PROPERTY(bool perfEnabled READ perfEnabled CONSTANT)

    // ===== 以下属性供 QML 使用（响应式布局 / 安全区 / 主题系统）=====
    // 安全区内边距：Qt 6.8 未向 QML 暴露系统 insets，只能由 C++ 计算后暴露。
    // 桌面平台恒为 0；Android/iOS 取屏幕可用区域与完整区域的差值，并支持
    // 环境变量 YUZORA_SAFE_AREA_TOP/BOTTOM/LEFT/RIGHT 覆盖，便于桌面端模拟验证。
    Q_PROPERTY(int safeAreaTop READ safeAreaTop NOTIFY safeAreaChanged)
    Q_PROPERTY(int safeAreaBottom READ safeAreaBottom NOTIFY safeAreaChanged)
    Q_PROPERTY(int safeAreaLeft READ safeAreaLeft NOTIFY safeAreaChanged)
    Q_PROPERTY(int safeAreaRight READ safeAreaRight NOTIFY safeAreaChanged)
    // 是否触屏优先设备：供 QML 降级 hover 态、隐藏鼠标光标形状
    Q_PROPERTY(bool isTouchDevice READ isTouchDevice CONSTANT)
    // 系统当前是否为深色：供 QML 在"跟随系统"模式下选择色板。
    // Material.theme 只对 Material 控件生效，自定义色板拿不到解析结果。
    Q_PROPERTY(bool systemDarkMode READ systemDarkMode NOTIFY systemColorSchemeChanged)
    // 测试注入：模拟软键盘占用高度，便于在桌面上验证键盘避让
    Q_PROPERTY(int testKeyboardInset READ testKeyboardInset
               WRITE setTestKeyboardInset NOTIFY testKeyboardInsetChanged)

public:
    enum class DeviceType {
        Windows,
        macOS,
        Linux,
        Android,
        iOS,
        Unknown
    };
    /*注册到 Qt 元系统,加了Q_ENUM，能使用Qt提供的枚举字符串互转QMetaEnum,可以在信号槽里直接传枚举
    qDebug 输出枚举字符串,（不加 Q_ENUM 只会输出数字）
    可直接转成字符串,如:
    QMetaEnum me = QMetaEnum::fromType<Device::DeviceType>();
    qDebug() << me.valueToKey(m_deviceType);*/
    Q_ENUM(DeviceType);

    static DeviceHelper& instance();//引用型单例,无空指针风险
    ~DeviceHelper() = default;//自动生成析构

    // 设备类型判断
    bool isMobile() const; // 是否为手机设备（Android）
    bool isPC() const; // 是否为 PC 设备（Windows）
    bool isTablet() const;//是否为平板(不用)
    DeviceType deviceType() const { return m_deviceType; }
    qreal screenDensity() const { return m_screenDensity; }
    QSize screenSize() const { return m_screenSize; }

    // ===== 供 QML 使用 =====
    int safeAreaTop() const { return m_safeAreaTop; }
    int safeAreaBottom() const { return m_safeAreaBottom; }
    int safeAreaLeft() const { return m_safeAreaLeft; }
    int safeAreaRight() const { return m_safeAreaRight; }
    bool isTouchDevice() const;
    bool systemDarkMode() const;
    // 性能探针是否开启（供 QML 使用）
    bool perfEnabled() const;
    // 追加一行性能日志到 <应用数据目录>/perf.log（供 QML 使用）
    Q_INVOKABLE void perfLog(const QString& text);
    // 清空 perf.log（供 QML 使用）
    Q_INVOKABLE void perfLogClear();
    // 性能日志文件路径（供 QML 使用）
    Q_INVOKABLE QString perfLogPath() const;
    int testKeyboardInset() const { return m_testKeyboardInset; }
    void setTestKeyboardInset(int inset);
    // QML 里按 dp 取尺寸：DeviceHelper.dp(48)
    Q_INVOKABLE qreal dp(qreal value) const;

    // 双端路径适配
    QString getAppDataPath() const; //获取应用数据的标准目录路径
    void reSetAppDataPath(const QString & newAppPath);//重新设置应用数据路径
    bool removeDataDir(const QString & path);//删除数据
    void restartApplication(const QStringList &extraArgs = {});//重启应用程序

//以下废弃
    QString getExternalStoragePath() const; //获取外部存储路径(仅Android有效)
    // 权限适配 (安卓系统权限,必须主动申请,用户同意才能用)(当前预计阻塞100毫秒,可优化)(当前问题较多,需优化)
    bool checkPermission(const QString& permissionString); //检测权限
    bool requestPermission(const QString& permissionString);//请求权限
    // UI缩放适配(px:像素Pixel dp:独立像素专门给安卓UI用)
    qreal dpToPx(qreal dp) const;
    qreal pxToDp(qreal px) const;
    int getScaledFontSize(int baseSize) const;// 获取缩放后的字体大小

signals:
    void permissionGranted(const QString& permission);
    void permissionDenied(const QString& permission);

    // ===== 以下信号供 QML 使用 =====
    void safeAreaChanged();            // 安全区变化（旋转、分屏、系统栏显隐）
    void systemColorSchemeChanged();   // 系统深浅色变化
    void testKeyboardInsetChanged();   // 测试注入的高度变化

private:
    DeviceHelper(QObject *parent = nullptr);//私有构造
    //检测设备类型
    void detectDeviceType();
    //检测屏幕信息
    void detectScreenInfo();
    //计算安全区内边距（Qt 6.8 无公共 API，用可用区域差值 + 环境变量覆盖）
    void detectSafeArea();
    //初始化应用路径
    void initAppPath();

    //字符串转换QPermission
    QPermission toPerm(const QString& permissionString);

    DeviceType m_deviceType; //枚举类型,由公有函数访问
    qreal m_screenDensity; //屏幕缩放大小
    QSize m_screenSize; //屏幕当前分辨率
    QString appPath;//应用路径

    // 安全区（由 detectSafeArea 计算，供 QML 使用）
    int m_safeAreaTop = 0;
    int m_safeAreaBottom = 0;
    int m_safeAreaLeft = 0;
    int m_safeAreaRight = 0;
    // 测试注入的键盘高度（桌面端验证键盘避让用）
    int m_testKeyboardInset = 0;

    // ========== 工具函数 ==========
    bool validateTargetPath(const QString & newPath, qint64 requiredBytes);
    bool testWritable(const QString & dirPath);//测试写入权限
    qint64 calculateDirSize(const QString &path); //计算数据大小
    bool copyDataDir(const QString & src, const QString & dst);//复制数据
    bool moveDataDir(const QString & src, const QString & dst);//移动数据

};
#endif
