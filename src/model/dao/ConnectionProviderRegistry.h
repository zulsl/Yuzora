#ifndef CONNECTIONPROVIDERREGISTRY_H
#define CONNECTIONPROVIDERREGISTRY_H

class IConnectionProvider;

/**
 * @brief 默认数据库连接注册表
 *
 * BaseDao 不再直接引用 DatabaseManager 单例。生产启动时由
 * ApplicationBootstrap 注册 DatabaseManager；测试可以注册一个指向
 * :memory: SQLite 的 provider。
 */
class ConnectionProviderRegistry
{
public:
    static void setProvider(IConnectionProvider* provider);
    static IConnectionProvider* provider();
};

#endif // CONNECTIONPROVIDERREGISTRY_H
