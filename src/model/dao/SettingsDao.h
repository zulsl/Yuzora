#ifndef SETTINGSDAO_H
#define SETTINGSDAO_H

#include <QObject>
#include <QList>
#include "BaseDao.h"
#include "SettingsEntity.h"

class SettingsDao : public QObject, public BaseDao<SettingsEntity> {
    Q_OBJECT
public:
    explicit SettingsDao(QObject* parent = nullptr);

    // 按键查询设置项
    SettingsEntity selectByKey(const QString& key);
    // 插入或更新设置项（主键冲突则更新）
    bool insertOrUpdate(const SettingsEntity& entity);
    // 批量查询所有设置项
    QList<SettingsEntity> selectAll();
    // 按键删除
    bool removeByKey(const QString& key);
    // 清空所有设置
    bool clearAll();

protected:
    // BaseDao 纯虚接口实现
    QString tableName() const override;
    QString primaryKey() const override;
    QStringList columnNames() const override;
    void bindValues(QSqlQuery& query, const SettingsEntity& entity) const override;
    SettingsEntity parseFromQuery(const QSqlQuery& query) const override;
};

#endif