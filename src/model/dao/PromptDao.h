#ifndef PROMPTDAO_H
#define PROMPTDAO_H

#include <QObject>
#include "BaseDao.h"
#include "PromptEntity.h"

class PromptDao : public QObject , public BaseDao<PromptEntity>{
    Q_OBJECT
public:
    explicit PromptDao(QObject *parent = nullptr);

    QList<PromptEntity> selectPromptsByPresetId(const QString& presetId);
    bool createPrompts(const QList<PromptEntity>& prompts);

protected:
    // ========== 实现基类纯虚方法 ==========
    QString tableName() const override;
    QString primaryKey() const override;
    QStringList columnNames() const override;
    void bindValues(QSqlQuery& query, const PromptEntity& entity) const override;
    PromptEntity parseFromQuery(const QSqlQuery& query) const override;

};

#endif
