#ifndef ABSTRACTDAO_H
#define ABSTRACTDAO_H

#include <QList>
#include <QString>


/** DAO 层纯虚接口基类
 * @tparam T 实体类型
 * 约定：主键统一为 QString 类型（UUID）
 * 声明DAO必须提供哪些方法
 */

template<typename T>
class AbstractDao
{
public:
    virtual ~AbstractDao() = default;

    // 基础查询
    virtual QList<T> selectAll() = 0;
    virtual T selectById(const QString& id) = 0;
    virtual bool exists(const QString& id) = 0;

    // 基础增删改
    virtual bool insert(const T& entity) = 0;
    virtual bool update(const T& entity) = 0;
    virtual bool remove(const QString& id) = 0;
};

#endif // ABSTRACTDAO_H
