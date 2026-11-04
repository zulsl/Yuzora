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
    // 分页查询会话消息（按时间正序）
    QList<ChatMessageEntity> selectBySessionIdPaged(
        const QString& sessionId, int offset = 0, int limit = 50);
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