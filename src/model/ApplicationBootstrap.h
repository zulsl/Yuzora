#ifndef APPLICATIONBOOTSTRAP_H
#define APPLICATIONBOOTSTRAP_H

/**
 * @brief 应用启动编排
 *
 * 数据库初始化和种子数据写入必须在这里显式发生，不能藏在 Service 构造函数里。
 * 这样测试可以初始化 :memory: 数据库并决定是否写入种子数据，而普通 QML 启动
 * 只需要在加载 QML 之前调用一次 initialize()。
 */
class ApplicationBootstrap
{
public:
    static bool initialize();
};

#endif // APPLICATIONBOOTSTRAP_H
