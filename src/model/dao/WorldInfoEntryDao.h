#ifndef WORLDINFOENTRYDAO_H
#define WORLDINFOENTRYDAO_H

#include <QObject>
#include "BaseDao.h"
#include "WorldInfoEntryEntity.h"

class WorldInfoEntryDao : public QObject, public BaseDao<WorldInfoEntryEntity>
{
    Q_OBJECT
public:
    explicit WorldInfoEntryDao(QObject *parent = nullptr);
// 条目操作
    //按书籍ID选择条目
    QList<WorldInfoEntryEntity> selectEntriesByBookId(const QString& bookId);

protected:
    // ========== 实现基类纯虚方法 ==========
    QString tableName() const override;
    QString primaryKey() const override;
    QStringList columnNames() const override;
    void bindValues(QSqlQuery& query, const WorldInfoEntryEntity& entity) const override;
    WorldInfoEntryEntity parseFromQuery(const QSqlQuery& query) const override;

};

#endif // WORLDINFOENTRYDAO_H
