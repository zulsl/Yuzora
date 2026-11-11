#ifndef ICONNECTIONPROVIDER_H
#define ICONNECTIONPROVIDER_H

#include <QSqlDatabase>

/**
 * @brief 数据库连接 seam
 *
 * DAO 只依赖这个接口，不直接依赖 DatabaseManager 单例。生产环境由
 * DatabaseManager 提供线程连接；测试可以提供一个指向 :memory: SQLite 的
 * provider，从而在不碰用户数据目录的情况下测试 DAO/service。
 */
class IConnectionProvider
{
public:
    virtual ~IConnectionProvider() = default;
    virtual QSqlDatabase connection() = 0;
};

#endif // ICONNECTIONPROVIDER_H
