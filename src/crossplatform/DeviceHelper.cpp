#include "DeviceHelper.h"
#include "Utils.h"
#include "ExceptionHandler.h"
#include "DatabaseManager.h"

#include <QGuiApplication>
#include <QScreen>
#include <QStandardPaths>
#include <QSysInfo>
#include <QDebug>
#include <QFuture>
#include <QSslSocket>
#include <QSettings>


#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QtCore/private/qandroidextras_p.h>
#endif

// 单例实现
DeviceHelper& DeviceHelper::instance()
{
    static DeviceHelper inst;
    return inst;
}

DeviceHelper::DeviceHelper(QObject *parent)
    : QObject(parent)
    , m_deviceType(DeviceType::Unknown)
    , m_screenDensity(1.0)
{
    qDebug() << "Qt编译时的SSL版本:" << QSslSocket::sslLibraryBuildVersionString();
    qDebug() << "运行时是否支持SSL:" << QSslSocket::supportsSsl();
    qDebug() << "运行时SSL版本:" << QSslSocket::sslLibraryVersionString();


    // 检测
    detectDeviceType();
    detectScreenInfo();
    initAppPath();
}

void DeviceHelper::detectDeviceType()
{
    // 基于Qt预定义宏自动检测操作系统(已舍弃)
    //设备类型：基于QSysInfo::productType() 识别
    QString os = QSysInfo::productType();
    if (os == "windows") {
        m_deviceType = DeviceType::Windows;
    } else if (os == "osx") {
        m_deviceType = DeviceType::macOS;
    } else if (os == "linux") {
        m_deviceType = DeviceType::Linux;
    } else if (os == "android") {
        m_deviceType = DeviceType::Android;
    } else if (os == "ios") {
        m_deviceType = DeviceType::iOS;
    } else {
        m_deviceType = DeviceType::Unknown;
        qWarning() << "检测到不支持的设备类型";
    }
    qDebug()<<"当前设备操作系统:"<<os;
    qDebug()<<"当前设备操作系统版本:"<<QSysInfo::productVersion();
    qDebug()<<"当前cup架构:"<<QSysInfo::buildCpuArchitecture();
}

void DeviceHelper::detectScreenInfo()
{
    //获取系统主屏幕对象指针
    QScreen* primaryScreen = QGuiApplication::primaryScreen();
    if (Q_LIKELY(primaryScreen)) {//Q_LIKELY(...)分支预测优化宏
        // 逻辑尺寸
        m_screenSize = primaryScreen->size();

        // 设备屏幕缩放大小
        m_screenDensity = primaryScreen->devicePixelRatio();

        // 限制合理密度范围，防止异常设备导致UI错乱
        m_screenDensity = qBound(0.5, m_screenDensity, 4.0);

        //屏幕分辨率
        QSize realSize = primaryScreen->size() * primaryScreen->devicePixelRatio();
        qDebug() << "检测到屏幕大小:" << m_screenSize
                 << "密度:" << m_screenDensity<<"屏幕分辨率:"<<realSize;
    } else {
        // 极端情况兜底值,获取屏幕失败
        m_screenSize = QSize(1920, 1080);
        m_screenDensity = 1.0;
        qWarning() << "无法使用默认值获取主屏幕";
    }
}

void DeviceHelper::initAppPath()
{
    QSettings settings;
    QString path = settings.value("storage/path").toString();

    if(path.isEmpty()){
        appPath=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        qDebug()<<"无自定义路径";
        qDebug()<<"当前路径:"<<QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    }
    else {
        qDebug()<<"当前路径:"<<path;
        appPath=path;
    }

}

QPermission DeviceHelper::toPerm(const QString &permissionString)
{
    //saf(存储访问框架)
    //PhotoPicker（系统相册选择器)

    QPermission permission;
    if (permissionString == "android.permission.CAMERA") {
        permission = QCameraPermission();
    }
    else if (permissionString == "android.permission.RECORD_AUDIO") {
        permission = QMicrophonePermission();
    }
    else if (permissionString == "android.permission.ACCESS_FINE_LOCATION" ||
             permissionString == "android.permission.ACCESS_COARSE_LOCATION") {
        permission = QLocationPermission();
    }
    else if (permissionString == "android.permission.BLUETOOTH" ||
             permissionString == "android.permission.BLUETOOTH_SCAN" ||
             permissionString == "android.permission.BLUETOOTH_CONNECT") {
        permission = QBluetoothPermission();
    }
    else if (permissionString == "android.permission.READ_CALENDAR" ||
             permissionString == "android.permission.WRITE_CALENDAR") {
        permission = QCalendarPermission();
    }
    // 注意：READ_EXTERNAL_STORAGE 等存储权限没有对应的跨平台类，不支持
    //读取相册:READ_MEDIA_IMAGES
    //新版权限:MANAGE_EXTERNAL_STORAGE
    else {
        qWarning() << "检查权限: 不支持或特定于平台的权限:"
                   << permissionString;
    }
    return permission;
}

bool DeviceHelper::validateTargetPath(const QString & newPath, qint64 requiredBytes)
{
    QString path=newPath;
    QFileInfo info(path);
    if (!info.isAbsolute()) {
        emit ExceptionHandler::instance().errorOccurred("文件路径不能为相对路径");
        return false;
    }
    //权限检测
    if (info.exists() && info.isDir()) {
        if (!info.isWritable()) {
            emit ExceptionHandler::instance().errorOccurred("当前文件路径无权限");
            return false;
        }
    }
    path = info.canonicalFilePath();
    //写入测试
    bool ok=testWritable(path);
    if(!ok){
        emit ExceptionHandler::instance().errorOccurred("当前文件路径无权限或写入失败");
        return false;
    }
    // 空间检查
    QStorageInfo storage(path);
    if (storage.bytesAvailable() < requiredBytes){
        emit ExceptionHandler::instance().errorOccurred("新路径空间不足");
        return false;
    }

    return true;
}

bool DeviceHelper::testWritable(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        // 尝试创建目录
        if (!dir.mkpath(".")) return false;
    }

    QString testFile = dir.filePath(".write_test_XXXXXX");
    QFile f(testFile);
    if (!f.open(QIODevice::WriteOnly)) {
        return false;
    }
    f.write("test");
    f.close();
    f.remove();
    return true;

}

qint64 DeviceHelper::calculateDirSize(const QString & path)
{
    qint64 total = 0;
    QDirIterator it(path, QDir::Files|QDir::NoDotAndDotDot,QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        total += it.fileInfo().size();
    }
    return total;
}

bool DeviceHelper::copyDataDir(const QString & src, const QString & dst)
{
    QDir srcDir(src);
    if (!srcDir.exists())
        return true; // 源不存在，视为成功

    QDir dstDir(dst);
    if (!dstDir.exists() && !dstDir.mkpath("."))
        return false;

    const auto entries = srcDir.entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QFileInfo &info : entries) {
        QString dstPath = dst + "/" + info.fileName();
        if (info.isDir()) {
            if (!copyDataDir(info.absoluteFilePath(), dstPath))
                return false;
        } else {
            if (QFile::exists(dstPath))
                QFile::remove(dstPath); // 覆盖
            if (!QFile::copy(info.absoluteFilePath(), dstPath))
                return false;
        }
    }
    return true;
}

bool DeviceHelper::moveDataDir(const QString &src, const QString &dst)
{
    // 先尝试 rename（同盘瞬间完成）
    if (QFile::rename(src, dst))
        return true;

    // rename 失败（跨盘），退化为复制 + 删除
    if (!copyDataDir(src, dst)){
        THROW(FileIOError, "[DeviceHelper::moveDataDir]文件复制失败");
    }

    if (!removeDataDir(src)) THROW(FileIOError, "[DeviceHelper::moveDataDir]文件删除失败");

    return true;
}

bool DeviceHelper::removeDataDir(const QString & path)
{
    QDir dir(path);
    return dir.removeRecursively();
}

void DeviceHelper::restartApplication(const QStringList &extraArgs)
{
    QString program = QCoreApplication::applicationFilePath();
    QStringList args = QCoreApplication::arguments();
    if (!args.isEmpty())
        args.removeFirst();
    args << extraArgs;

    QProcess::startDetached(program, args);
    QCoreApplication::quit();
}

bool DeviceHelper::isMobile() const
{
    return m_deviceType == DeviceType::Android || m_deviceType == DeviceType::iOS;
}

bool DeviceHelper::isPC() const
{
    return m_deviceType == DeviceType::Windows
           || m_deviceType == DeviceType::macOS
           || m_deviceType == DeviceType::Linux;
}

bool DeviceHelper::isTablet() const
{
    if (!isMobile()) return false;

    // 计算屏幕对角线英寸数
    qreal widthInch = m_screenSize.width() / (m_screenDensity * 160.0);
    qreal heightInch = m_screenSize.height() / (m_screenDensity * 160.0);
    qreal diagonalInch = qSqrt(widthInch * widthInch + heightInch * heightInch);

    // 行业标准：7-12英寸视为平板
    return diagonalInch >= 7.0 && diagonalInch < 12.0;
}

QString DeviceHelper::getAppDataPath() const
{
    return appPath;
}

void DeviceHelper::reSetAppDataPath(const QString & newAppPath)
{
    qDebug()<<"选择新路径为:"<<newAppPath;
    QString path=newAppPath+"/Yozora";
    Utils::ensureDirExists(path);
    qDebug()<<"完整新路径为:"<<path;
    //验证路径
    bool ok=validateTargetPath(path,calculateDirSize(path));
    if(!ok) {
        qDebug()<<"未知错误";
        return;
    }


    //更改路径
    QSettings settings;
    settings.setValue("storage/path", path);
    settings.sync();



    // 关闭数据库
    DatabaseManager::instance().closeAllResources();
    // 关闭文件句柄()
    // 停止写入线程()

    //迁移数据
    moveDataDir(appPath,path);



    //重新加载或者重启
    // initAppPath();
    emit ExceptionHandler::instance().tipOccurred("路径修改成功");
    restartApplication();
}

QString DeviceHelper::getExternalStoragePath() const
{
    // Android外部存储（SD卡）路径，需要存储权限
#if defined(Q_OS_ANDROID)
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
#else
    // 桌面平台无外部存储概念，返回应用数据目录
    return getAppDataPath();
#endif
}

//检测权限
bool DeviceHelper::checkPermission(const QString& permissionString)
{
#ifdef Q_OS_ANDROID
    QPermission permission;

    //当前支持的权限
    permission=toPerm(permissionString);

    // 同步检查权限状态
    Qt::PermissionStatus status = QCoreApplication::instance()->checkPermission(permission);
    return (status == Qt::PermissionStatus::Granted);
#else
    Q_UNUSED(permissionString);
    return true;
#endif
}

//请求权限
bool DeviceHelper::requestPermission(const QString& permissionString)
{
// 请求权限（同步阻塞，返回是否授予）
// 注意：此函数会阻塞调用线程（通常为 UI 线程），请谨慎使用。

#ifdef Q_OS_ANDROID
    QPermission permission;

    //当前支持的权限
    permission=toPerm(permissionString);
    // 先检查当前权限状态
    Qt::PermissionStatus status = QCoreApplication::instance()->checkPermission(permission);
    if (status == Qt::PermissionStatus::Granted) {
        return true;
    }
    if (status == Qt::PermissionStatus::Denied) {
        return false;
    }

    // 权限未确定，需要请求用户授权
    QEventLoop loop;
    bool granted = false;

    QCoreApplication::instance()->requestPermission(permission, [&](const QPermission &perm) {
        granted = (perm.status() == Qt::PermissionStatus::Granted);
        loop.quit();
    });

    // 等待回调执行（注意：会阻塞当前线程）
    loop.exec();

    return granted;
#else
    Q_UNUSED(permissionString);
    emit permissionGranted(permissionString);
    return true;
#endif
}

// dp转px
qreal DeviceHelper::dpToPx(qreal dp) const
{
    // Android标准: 1dp = 1px @ 160dpi
    // Qt的devicePixelRatio已经考虑了系统缩放
    return dp * m_screenDensity;
}

// px转dp
qreal DeviceHelper::pxToDp(qreal px) const
{
    if (m_screenDensity <= 0) {
        return px;
    }
    return px / m_screenDensity;
}


int DeviceHelper::getScaledFontSize(int baseSize) const
{
    return qRound(baseSize * m_screenDensity);
}


