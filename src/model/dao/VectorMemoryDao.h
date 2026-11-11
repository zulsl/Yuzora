#ifndef VECTORMEMORYDAO_H
#define VECTORMEMORYDAO_H

#include <QObject>
#include <QList>
#include <QSqlDatabase>
#include "entity/VectorMemoryEntity.h"

class IConnectionProvider;

//向量记忆数据访问类

class VectorMemoryDao : public QObject {
    Q_OBJECT
public:
    explicit VectorMemoryDao(QObject* parent = nullptr);

    void setConnectionProvider(IConnectionProvider* provider);
    QSqlDatabase connection() const;

    QList<VectorMemoryEntity> selectByCharacterId(const QString& characterId);
    QString insert(const VectorMemoryEntity& memory);
    bool remove(const QString& id);
    bool clearByCharacterId(const QString& characterId);

private:
    IConnectionProvider* m_connectionProvider = nullptr;
};

#endif // VECTORMEMORYDAO_H