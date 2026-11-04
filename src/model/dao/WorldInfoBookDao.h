#ifndef WORLDINFOBOOKDAO_H
#define WORLDINFOBOOKDAO_H

#include <QObject>
#include <QList>
#include "BaseDao.h"
#include "WorldInfoBookEntity.h"
#include "WorldInfoEntryEntity.h"

//世界书数据访问类

class WorldInfoBookDao : public QObject , public BaseDao<WorldInfoBookEntity>{
    Q_OBJECT
public:
    explicit WorldInfoBookDao(QObject* parent = nullptr);

protected:
    // ========== 实现基类纯虚方法 ==========
    QString tableName() const override;
    QString primaryKey() const override;
    QStringList columnNames() const override;
    void bindValues(QSqlQuery& query, const WorldInfoBookEntity& entity) const override;
    WorldInfoBookEntity parseFromQuery(const QSqlQuery& query) const override;
};

#endif // WORLDINFOBOOKDAO_H
