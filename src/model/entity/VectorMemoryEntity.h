#ifndef VECTORMEMORYENTITY_H
#define VECTORMEMORYENTITY_H

#include <QString>
#include <QDateTime>
#include <QList>

//向量记忆数据实体

struct VectorMemoryEntity {
    QString id;
    QString characterId;
    QString content;
    QDateTime timestamp;
    double importance = 0.5;
    QString embeddingFilePath;
    QString sourceSessionId;
};

#endif // VECTORMEMORYENTITY_H