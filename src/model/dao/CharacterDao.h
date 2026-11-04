#ifndef CHARACTERDAO_H
#define CHARACTERDAO_H

#include <QObject>
#include <QList>
#include "BaseDao.h"
#include "CharacterEntity.h"

//角色数据访问类

class CharacterDao : public QObject , public BaseDao<CharacterEntity>{
    Q_OBJECT
public:
    explicit CharacterDao(QObject* parent = nullptr);

    // ========== 业务扩展方法（基类未覆盖的专属查询,保留函数） ==========
    QList<CharacterEntity> searchByName(const QString& keyword);
    bool updateAvatar(const QString& charId, const QString& avatarPath);
protected:
    // ========== 实现基类纯虚方法 ==========
    QString tableName() const override;
    QString primaryKey() const override;
    QStringList columnNames() const override;
    void bindValues(QSqlQuery& query, const CharacterEntity& entity) const override;
    CharacterEntity parseFromQuery(const QSqlQuery& query) const override;
};

#endif // CHARACTERDAO_H