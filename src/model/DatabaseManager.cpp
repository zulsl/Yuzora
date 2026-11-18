#include "DatabaseManager.h"
#include <QDebug>
#include <QThread>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QMutexLocker>

#include "Utils.h"
#include "GlobalConstants.h"
#include "MigrationRunner.h"
#include "SchemaMigrations.h"


DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::~DatabaseManager()
{
    //数据库单例生命周期和程序进程一致,不需要析构,以下废弃
    // // 仅关闭主线程连接，子线程连接由线程生命周期自动管理
    // QString mainConnName = connectionNameForCurrentThread();
    // if (QSqlDatabase::contains(mainConnName)) {
    //     QSqlDatabase db = QSqlDatabase::database(mainConnName);
    //     if (db.isOpen()) {
    //         db.close();
    //     }
    //     QSqlDatabase::removeDatabase(mainConnName);
    // }
}

bool DatabaseManager::initialize()
{
    if (m_isReady) {
        return true;
    }

// 构建数据库文件路径，确保目录存在
    m_dbFilePath = Utils::getAppDataPath() + QDir::separator() + GlobalConstants::DB_FILE_NAME;
    Utils::ensureDirExists(Utils::getAppDataPath());

// 创建主线程数据库连接
    QSqlDatabase db = getThreadConnection();
    if (!db.isOpen()) {
        qCritical() << "[数据库] 打开失败：" << db.lastError().text();
        return false;
    }

    // 外键在每个线程连接建立时开启（见 getThreadConnection）。

    // 统一走版本化迁移：第一次启动直接建 V1。
    QString migrationError;
    const QVector<SchemaMigration> migrations = SchemaMigrations::all();
    if (!MigrationRunner::run(db, m_dbFilePath, migrations, &migrationError)) {
        qCritical() << "[数据库] 迁移失败：" << migrationError;
        return false;
    }

    m_isReady = true;
    qInfo() << "[数据库] 初始化成功，版本：" << SchemaMigrations::latestVersion();
    return true;
}

bool DatabaseManager::isReady() const
{
    return m_isReady;
}

QSqlDatabase DatabaseManager::getThreadConnection()
{
    const QString connName = connectionNameForCurrentThread();

    // 判断连接池中是否存在指定名称的数据库连接,连接已存在且已打开，直接返回
    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase db = QSqlDatabase::database(connName);
        if (db.isOpen()) {
            return db;
        }
    }

    // 加锁创建新连接（QSqlDatabase::addDatabase 非线程安全）
    QMutexLocker locker(&m_mutex);

    // 双重检查，避免加锁期间已被其他线程创建
    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase db = QSqlDatabase::database(connName);
        if (db.isOpen()) {
            return db;
        }
    }

    // 创建线程专属连接
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);//添加驱动
    db.setDatabaseName(m_dbFilePath);

    if (!db.open()) {
        qCritical() << "[数据库] 线程创建连接失败：" << connName
                    << "Error:" << db.lastError().text();
        return QSqlDatabase();
    }

    // 每个连接单独开启外键（SQLite默认关闭，每个连接都需要单独开启）
    QSqlQuery query(db);
    query.exec("PRAGMA foreign_keys = ON;");

    qDebug()<< "[数据库] 新连接已创建:" << connName;
    return db;
}

void DatabaseManager::closeAllResources()
{
    // 清除数据/切换目录前必须释放所有线程的 SQLite 文件句柄，
    // 否则 Windows 下 QDir::removeRecursively() 删不掉仍被打开的 .db。
    const QStringList names = QSqlDatabase::connectionNames();
    for (const QString& name : names) {
        if (!name.startsWith(QStringLiteral("db_conn_"))) {
            continue;
        }

        {
            QSqlDatabase db = QSqlDatabase::database(name, false);
            if (db.isOpen()) {
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(name);
    }
}

bool DatabaseManager::beginTransaction()
{
    QSqlDatabase db = getThreadConnection();
    if (!db.isOpen()) {
        return false;
    }
    return db.transaction();
}

bool DatabaseManager::commitTransaction()
{
    QSqlDatabase db = getThreadConnection();
    if (!db.isOpen()) {
        return false;
    }
    return db.commit();
}

bool DatabaseManager::rollbackTransaction()
{
    QSqlDatabase db = getThreadConnection();
    if (!db.isOpen()) {
        return false;
    }
    return db.rollback();
}

QSqlError DatabaseManager::executeQuery(const QString& sql, const QVariantMap& bindValues)
{
    QSqlDatabase db = getThreadConnection();
    if (!db.isOpen()) {
        return db.lastError();
    }

    QSqlQuery query(db);
    if (!query.prepare(sql)) {
        return query.lastError();
    }

    // 绑定命名参数
    for (auto it = bindValues.constBegin(); it != bindValues.constEnd(); ++it) {
        query.bindValue(":" + it.key(), it.value());
    }

    if (!query.exec()) {
        return query.lastError();
    }

    return QSqlError();
}

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
{
}

QString DatabaseManager::connectionNameForCurrentThread() const
{
    // 以线程ID哈希作为连接名，保证每个线程唯一
    return QString("db_conn_%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
}