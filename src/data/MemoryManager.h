#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <QObject>
#include <QList>
#include <QMap>
#include "GlobalConstants.h"
#include "ChatRecordManager.h"


//记忆管理类

//记忆项
struct MemoryItem {
    QString id;
    QString content;//内容
    QDateTime timestamp;//时间戳
    double importance = 0.5;//重要性
    QList<double> embedding;//嵌入
    QString sourceSessionId;//源会话ID
};

class MemoryManager : public QObject {
    Q_OBJECT
public:
    static MemoryManager& instance();
    ~MemoryManager() override = default;

// ======================================
// 短期记忆（Token滑动窗口截断） (需优化)
// ======================================
    //获取短期记忆
    QList<ChatMessage> getShortTermMemory(const QString& charId,const QString& sessionId, int maxToken);

// ======================================
// 长期摘要记忆
// ======================================
    //获取长期摘要
    QString getLongTermSummary(const QString& charId, const QString& sessionId);
    //生成长期摘要
    void generateLongTermSummary(const QString& charId, const QString& sessionId);

// ======================================
// 向量语义记忆
// ======================================
    //添加向量记忆
    void addVectorMemory(const QString& charId, const QString& content);
    //搜索向量存储器
    QList<MemoryItem> searchVectorMemory(const QString& charId,const QString& query, int topK = 5);
    //构建嵌入
    bool buildEmbeddings(const QString& charId);

// ======================================
// 整合记忆Prompt
// ======================================
    //构建内存提示
    QString buildMemoryPrompt(const QString& charId, const QString& sessionId, const QString& query);

signals:
    //摘要已生成
    void summaryGenerated(const QString& charId, const QString& sessionId);
    //嵌入已构建
    void embeddingsBuilt(const QString& charId);

private:
    MemoryManager(QObject* parent = nullptr);

    //加载角色记忆
    void loadMemoryForCharacter(const QString& charId);
    //保持角色到文件
    void saveMemoryToFile(const QString& charId);

    QMap<QString, QList<MemoryItem>> m_vectorMemories;//向量存储器
    QMap<QString, QString> m_longTermSummaries;//长期摘要
    QString m_memoryDir;//记忆目录
};

#endif // MEMORYMANAGER_H
