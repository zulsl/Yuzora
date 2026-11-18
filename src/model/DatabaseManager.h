#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMutex>
#include "dao/IConnectionProvider.h"

class DatabaseManager : public QObject, public IConnectionProvider {
    Q_OBJECT
public:
    static DatabaseManager& instance();
    ~DatabaseManager() override;

    // 初始化数据库核心配置（主线程调用一次）
    bool initialize();//数据库初始化
    bool isReady() const;

    // 获取当前线程专属数据库连接（每个线程首次调用自动创建）
    QSqlDatabase getThreadConnection();
    QSqlDatabase connection() override { return getThreadConnection(); }

    //关闭所有连接
    void closeAllResources();

    // 事务控制（仅在当前线程连接上生效）
    bool beginTransaction();//开始事务
    bool commitTransaction();//提交事务
    bool rollbackTransaction();//回滚事务

    // 当前线程执行SQL
    QSqlError executeQuery(const QString& sql, const QVariantMap& bindValues = {});

private:
    explicit DatabaseManager(QObject* parent = nullptr);
    //返回当前线程的连接名称
    QString connectionNameForCurrentThread() const;

    bool m_isReady = false;//已就绪
    QString m_dbFilePath;
    QMutex m_mutex;
};

#endif