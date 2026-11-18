#ifndef MEMORYSERVICE_H
#define MEMORYSERVICE_H

#include <QObject>
#include <QList>
#include "GlobalConstants.h"
#include "entity/ChatMessageEntity.h"
#include "entity/VectorMemoryEntity.h"

//记忆业务

class MemoryService : public QObject {
    Q_OBJECT
public:
    static MemoryService& instance();
    ~MemoryService() override = default;

    // 短期记忆
    QList<ChatMessageEntity> getShortTermMemory(const QList<ChatMessageEntity>& allMessages, int maxToken);

    // 长期摘要
    QString getLongTermSummary(const QString& characterId, const QString& sessionId);
    void generateLongTermSummary(const QString& characterId, const QString& sessionId, const QList<ChatMessageEntity>& messages);

    // 向量记忆
    void addVectorMemory(const QString& characterId, const QString& content);
    QList<VectorMemoryEntity> searchVectorMemory(const QString& characterId, const QString& query, int topK = 5);

    QString buildMemoryPrompt(const QString& characterId, const QString& sessionId, const QString& query);

signals:
    void summaryGenerated(const QString& characterId, const QString& sessionId);
    void embeddingsBuilt(const QString& characterId);

private:
    MemoryService() = default;
};

#endif // MEMORYSERVICE_H