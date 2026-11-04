#ifndef DEVICEHELPER_H
#define DEVICEHELPER_H

#include <QObject>
#include <QSize>

#include <QPermission>

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
    Q_PROPERTY(bool isMobile READ isMobile CONSTANT)
    Q_PROPERTY(bool isPC READ isPC CONSTANT)
    Q_PROPERTY(bool isTablet READ isTablet CONSTANT)
    Q_PROPERTY(DeviceType deviceType READ deviceType CONSTANT)
    Q_PROPERTY(qreal screenDensity READ screenDensity CONSTANT)
    Q_PROPERTY(QSize screenSize READ screenSize CONSTANT)

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

private:
    DeviceHelper(QObject *parent = nullptr);//私有构造
    //检测设备类型
    void detectDeviceType();
    //检测屏幕信息
    void detectScreenInfo();
    //初始化应用路径
    void initAppPath();

    //字符串转换QPermission
    QPermission toPerm(const QString& permissionString);

    DeviceType m_deviceType; //枚举类型,由公有函数访问
    qreal m_screenDensity; //屏幕缩放大小
    QSize m_screenSize; //屏幕当前分辨率
    QString appPath;//应用路径

    // ========== 工具函数 ==========
    bool validateTargetPath(const QString & newPath, qint64 requiredBytes);
    bool testWritable(const QString & dirPath);//测试写入权限
    qint64 calculateDirSize(const QString &path); //计算数据大小
    bool copyDataDir(const QString & src, const QString & dst);//复制数据
    bool moveDataDir(const QString & src, const QString & dst);//移动数据

};
#endif
