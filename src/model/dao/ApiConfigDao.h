#ifndef APICONFIGDAO_H
#define APICONFIGDAO_H

#include <QObject>
#include <QList>
#include <QSqlQuery>
#include "BaseDao.h"
#include "ApiConfigEntity.h"

class ApiConfigDao : public QObject , public BaseDao<ApiConfigEntity>{
    Q_OBJECT
public:
    explicit ApiConfigDao(QObject* parent = nullptr);

// 激活状态管理
    //选择激活的
    ApiConfigEntity selectActive();
    //设置激活
    bool setActive(const QString& id);
    //清除所有激活
    bool clearAllActive();

    // 重命名(未完成)
    bool rename(const QString& id, const QString& newName);

protected:
    // ========== 实现基类纯虚方法 ==========
    QString tableName() const override;
    QString primaryKey() const override;
    QStringList columnNames() const override;
    void bindValues(QSqlQuery& query, const ApiConfigEntity& entity) const override;
    ApiConfigEntity parseFromQuery(const QSqlQuery& query) const override;
};

#endif // APICONFIGDAO_H