#ifndef COMPLETIONPRESETDAO_H
#define COMPLETIONPRESETDAO_H

#include <QObject>
#include <QList>
#include "BaseDao.h"
#include "CompletionPresetEntity.h"

//对话补全预设数据访问类

class CompletionPresetDao : public QObject , public BaseDao<CompletionPresetEntity>{
    Q_OBJECT
public:
    explicit CompletionPresetDao(QObject* parent = nullptr);


    // 默认状态管理
    bool setDefault(const QString& id);
    bool clearAllDefault();
    CompletionPresetEntity selectDefault();

    // 重命名
    bool rename(const QString& id, const QString& newName);
protected:
    // ========== 实现基类纯虚方法 ==========
    QString tableName() const override;
    QString primaryKey() const override;
    QStringList columnNames() const override;
    void bindValues(QSqlQuery& query, const CompletionPresetEntity& entity) const override;
    CompletionPresetEntity parseFromQuery(const QSqlQuery& query) const override;
};

#endif // COMPLETIONPRESETDAO_H