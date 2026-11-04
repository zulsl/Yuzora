#ifndef VECTORMEMORYDAO_H
#define VECTORMEMORYDAO_H

#include <QObject>
#include <QList>
#include "entity/VectorMemoryEntity.h"

//向量记忆数据访问类

class VectorMemoryDao : public QObject {
    Q_OBJECT
public:
    explicit VectorMemoryDao(QObject* parent = nullptr);

    QList<VectorMemoryEntity> selectByCharacterId(const QString& characterId);
    QString insert(const VectorMemoryEntity& memory);
    bool remove(const QString& id);
    bool clearByCharacterId(const QString& characterId);
};

#endif // VECTORMEMORYDAO_H