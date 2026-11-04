#ifndef BASEDAO_H
#define BASEDAO_H

#include "AbstractDao.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>

#include "DatabaseManager.h"
#include "Utils.h"

/**
@brief DAO 模板基类，提供通用 CRUD 默认实现
子类只需实现 5 个纯虚方法即可获得完整基础能力
@tparam T 实体类型
*/

template<typename T>
class BaseDao : public AbstractDao<T>
{
public:
    ~BaseDao() override = default;

    QList<T> selectAll() override
    {
        QList<T> result;
        QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
        QSqlQuery query(db);

        QString sql = QString("SELECT * FROM %1").arg(tableName());
        if (!query.exec(sql)) {
            DatabaseError("selectAll", query.lastError(),sql,query);
            return result;
        }

        while (query.next()) {
            result.append(parseFromQuery(query));
        }
        return result;
    }
    T selectById(const QString& id) override
    {
        QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
        QSqlQuery query(db);

        QString sql = QString("SELECT * FROM %1 WHERE %2 = ?").arg(tableName(), primaryKey());
        query.prepare(sql);
        query.addBindValue(id);

        if (!query.exec()) {
            DatabaseError("selectById", query.lastError(),sql,query);
            return T{};
        }

        if (query.next()) {
            return parseFromQuery(query);
        }
        return T{};
    }
    bool exists(const QString& id) override
    {
        QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
        QSqlQuery query(db);

        QString sql = QString("SELECT 1 FROM %1 WHERE %2 = ? LIMIT 1").arg(tableName(), primaryKey());
        query.prepare(sql);
        query.addBindValue(id);

        if (!query.exec()) {
            DatabaseError("exists", query.lastError(),sql,query);
            return false;
        }
        return query.next();
    }
    bool insert(const T& entity) override
    {
        QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
        QSqlQuery query(db);

        QStringList cols = columnNames();
        QStringList placeholders(cols.size(), "?");

        QString sql = QString("INSERT INTO %1 (%2) VALUES (%3)")
                          .arg(tableName(), cols.join(", "), placeholders.join(", "));

        query.prepare(sql);
        bindValues(query, entity);

        if (!query.exec()) {
            DatabaseError("insert", query.lastError(),sql,query);
            return false;
        }
        return true;
    }
    bool update(const T& entity) override
    {
        QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
        QSqlQuery query(db);

        QStringList cols = columnNames();
        QStringList setClauses;
        for (const QString& col : cols) {
            setClauses.append(QString("%1 = ?").arg(col));
        }

        QString sql = QString("UPDATE %1 SET %2 WHERE %3 = ?")
            .arg(tableName(), setClauses.join(", "), primaryKey());

        query.prepare(sql);
        bindValues(query, entity);
        query.addBindValue(entity.id);

        if (!query.exec()) {
            DatabaseError("update", query.lastError(),sql,query);
            return false;
        }

        bool affected=query.numRowsAffected() > 0;
        if(!affected){
            Utils::writeLog("ERROR","数据库update","更新0行");
        }
        return query.numRowsAffected() > 0;
    }
    bool remove(const QString& id) override
    {
        QSqlDatabase db = DatabaseManager::instance().getThreadConnection();
        QSqlQuery query(db);

        QString sql = QString("DELETE FROM %1 WHERE %2 = ?").arg(tableName(), primaryKey());
        query.prepare(sql);
        query.addBindValue(id);

        if (!query.exec()) {
            DatabaseError("remove", query.lastError(),sql,query);
            return false;
        }
        return query.numRowsAffected() > 0;
    }

protected:
// ========== 子类必须实现的纯虚方法 ==========
    virtual QString tableName() const = 0;          // 数据库表名
    virtual QString primaryKey() const = 0;         // 主键字段名
    virtual QStringList columnNames() const = 0;    // 所有业务字段名（顺序与bindValues对应）
    // 实体与SQL参数绑定、结果解析
    virtual void bindValues(QSqlQuery& query, const T& entity) const = 0;
    virtual T parseFromQuery(const QSqlQuery& query) const = 0;

// ========== 通用工具方法 ==========
    void DatabaseError(const QString& operation,const QSqlError& error){
        Utils::writeLog("ERROR","Database",QString("[DAO] 表:%1 操作:%2 失败:%3")
            .arg(tableName(), operation, error.text())
        );
    }

    void DatabaseError(const QString& operation,const QSqlError& error,
        const QString& sqlStatement,const QSqlQuery& query) const
    {
        Utils::writeLog("ERROR","Database",
            QString("[DAO] 数据表:%1 操作:%2 驱动错误信息:%3 数据库原生信息:%4")
            .arg(tableName(),operation,error.text(),error.databaseText())
        );
        qWarning().noquote() << "SQL语句：" << sqlStatement;
        qWarning() << "占位符总数：" << query.boundValues().size();
        qWarning().noquote() << "绑定参数详情：" << query.boundValues();
    }
};

#endif // BASEDAO_H