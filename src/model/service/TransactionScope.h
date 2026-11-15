#ifndef TRANSACTIONSCOPE_H
#define TRANSACTIONSCOPE_H

// 事务作用域：ChatService 里"插用户消息 + 插助手占位"必须原子，
// 但事务实现属于数据层。抽成接口之后，单测可以给一个只计数的事务替身，
// 而不用把 DatabaseManager 整个单例拖进测试。
//
// 命名：它是"一段事务"而不是"跑东西的东西"，所以叫 Scope 不叫 Runner。
class TransactionScope {
public:
    virtual ~TransactionScope() = default;

    virtual bool begin() = 0;
    virtual bool commit() = 0;
    virtual void rollback() = 0;
};

#endif // TRANSACTIONSCOPE_H
