#ifndef CHATSESSIONDAO_H
#define CHATSESSIONDAO_H

#include <QObject>
#include <QList>
#include "BaseDao.h"
#include "ChatSessionEntity.h"

//聊天会话数据访问类

class ChatSessionDao : public QObject ,public BaseDao<ChatSessionEntity> {
    Q_OBJECT
public:
    explicit ChatSessionDao(QObject* parent = nullptr);

// 业务扩展查询
    QList<ChatSessionEntity> selectByCharacterId(const QString& characterId);
    //更新最后修改时间
    bool updateLastModified(const QString& id, const QDateTime& time);

    //删除当前角色全部会话
    bool removeByCharacterId(const QString& characterId);
protected:
    // 实现BaseDao纯虚接口
    QString tableName() const override;
    QString primaryKey() const override;
    QStringList columnNames() const override;
    void bindValues(QSqlQuery& query, const ChatSessionEntity& entity) const override;
    ChatSessionEntity parseFromQuery(const QSqlQuery& query) const override;

};

#endif // CHATSESSIONDAO_H