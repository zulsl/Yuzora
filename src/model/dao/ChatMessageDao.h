#ifndef CHATMESSAGEDAO_H
#define CHATMESSAGEDAO_H

#include <QObject>
#include <QList>
#include "BaseDao.h"
#include "ChatMessageEntity.h"

//聊天消息数据访问类

class ChatMessageDao : public QObject,public BaseDao<ChatMessageEntity> {
    Q_OBJECT
public:
    explicit ChatMessageDao(QObject* parent = nullptr);

    // 业务扩展方法
    // 最新一页（SQL 按时间倒序取，返回时恢复正序）
    QList<ChatMessageEntity> selectLatestBySessionId(
        const QString& sessionId, int limit = 50);
    // 早于指定游标的一页（游标为 timestamp + id，避免同一时间消息重复/丢失）
    QList<ChatMessageEntity> selectBefore(
        const QString& sessionId, const QDateTime& beforeTimestamp,
        const QString& beforeId, int limit = 50);
    // 增量更新消息内容（流式落库用）
    bool appendContent(const QString& messageId, const QString& chunk);
    //非流式更新内容
    bool updateContent(const QString& messageId, const QString& fullText);
    // 更新消息生成状态
    bool updateStatus(const QString& messageId,
        GlobalConstants::MessageGenerationStatus status);
    // 清空会话下所有消息
    bool removeBySessionId(const QString& sessionId);
    // 统计会话消息总数
    int countBySessionId(const QString& sessionId);
    // 获取会话最后一条消息
    ChatMessageEntity selectLastBySessionId(const QString& sessionId);
    // 获取会话第一条消息（识别开场白用）
    ChatMessageEntity selectFirstBySessionId(const QString& sessionId);
    // 获取会话第一条用户消息（自动标题用；开场白不算）
    ChatMessageEntity selectFirstUserBySessionId(const QString& sessionId);

    //按会话ID清除
    bool clearBySessionId(const QString& sessionId);

protected:
    // 实现BaseDao纯虚接口
    QString tableName() const override;
    QString primaryKey() const override;
    QStringList columnNames() const override;
    void bindValues(QSqlQuery& query, const ChatMessageEntity& entity) const override;
    ChatMessageEntity parseFromQuery(const QSqlQuery& query) const override;
};

#endif // CHATMESSAGEDAO_H