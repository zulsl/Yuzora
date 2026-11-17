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
    //删除某本书的全部条目
    bool removeByBookId(const QString& bookId);
    //用给定条目整体替换某本书的条目（事务内 DELETE + INSERT）。
    //重新导入同名世界书时使用：book id 不变，角色表里的 world_book_ids 链接不会断。
    bool replaceForBook(const QString& bookId, const QList<WorldInfoEntryEntity>& entries);

protected:
    // ========== 实现基类纯虚方法 ==========
    QString tableName() const override;
    QString primaryKey() const override;
    QStringList columnNames() const override;
    void bindValues(QSqlQuery& query, const WorldInfoEntryEntity& entity) const override;
    WorldInfoEntryEntity parseFromQuery(const QSqlQuery& query) const override;

};

#endif // WORLDINFOENTRYDAO_H
