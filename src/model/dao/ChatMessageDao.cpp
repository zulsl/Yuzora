#include "ChatMessageDao.h"
#include <algorithm>
#include "DatabaseManager.h"
#include "Utils.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>

ChatMessageDao::ChatMessageDao(QObject* parent)
    : QObject(parent)
{
}

QString ChatMessageDao::tableName() const
{
    return "chat_messages";
}

QString ChatMessageDao::primaryKey() const
{
    return "id";
}

QStringList ChatMessageDao::columnNames() const
{
    // 顺序必须与 bindValues 严格一一对应
    return {
        "id",
        "session_id",
        "message_type",
        "status",
        "content",
        "sender_name",
        "timestamp",
        "metadata",
        "is_edited",
        "is_hidden"
    };
}

void ChatMessageDao::bindValues(QSqlQuery& query, const ChatMessageEntity& entity) const
{
    query.addBindValue(entity.id);
    query.addBindValue(entity.sessionId);
    query.addBindValue(static_cast<int>(entity.type));
    query.addBindValue(static_cast<int>(entity.status));
    query.addBindValue(entity.content);
    query.addBindValue(entity.senderName);
    query.addBindValue(entity.timestamp.toString(Qt::ISODate));
    query.addBindValue(Utils::variantMapToJson(entity.metadata));
    query.addBindValue(entity.isEdited ? 1 : 0);
    query.addBindValue(entity.isHidden ? 1 : 0);
}

ChatMessageEntity ChatMessageDao::parseFromQuery(const QSqlQuery& query) const
{
    ChatMessageEntity entity;
    entity.id = query.value("id").toString();
    entity.sessionId = query.value("session_id").toString();
    entity.type = static_cast<GlobalConstants::MessageType>(query.value("message_type").toInt());
    entity.status = static_cast<GlobalConstants::MessageGenerationStatus>(query.value("status").toInt());
    entity.content = query.value("content").toString();
    entity.senderName = query.value("sender_name").toString();
    entity.timestamp = QDateTime::fromString(query.value("timestamp").toString(), Qt::ISODate);
    entity.metadata = Utils::jsonToVariantMap(query.value("metadata"));
    entity.isEdited = query.value("is_edited").toInt() == 1;
    entity.isHidden = query.value("is_hidden").toInt() == 1;
    return entity;
}

// ======================================
// 业务扩展方法：分页查询会话消息
// ======================================
QList<ChatMessageEntity> ChatMessageDao::selectLatestBySessionId(
    const QString& sessionId, int limit)
{
    QList<ChatMessageEntity> result;
    if (sessionId.isEmpty()) return result;

    QSqlDatabase db = connection();
    QSqlQuery query(db);
    // 时间戳只有秒精度：同一条 user 消息和它的 assistant 占位消息是同一个事务里插入的，
    // 秒值完全相同。再用随机的 uuid 做次级排序等于抛硬币——重启后"AI 回复跑到用户上面"
    // 就是这个原因。rowid 是插入顺序，正好等于对话顺序，用它当第二排序键。
    query.prepare(QString(
        "SELECT * FROM %1 WHERE session_id = ? "
        "ORDER BY timestamp DESC, rowid DESC LIMIT ?").arg(tableName()));
    query.addBindValue(sessionId);
    query.addBindValue(limit);

    if (!query.exec()) {
        DatabaseError("selectLatestBySessionId", query.lastError());
        return result;
    }
    while (query.next())
        result.append(parseFromQuery(query));

    std::reverse(result.begin(), result.end());
    return result;
}

QList<ChatMessageEntity> ChatMessageDao::selectBefore(
    const QString& sessionId, const QDateTime& beforeTimestamp,
    const QString& beforeId, int limit)
{
    QList<ChatMessageEntity> result;
    if (sessionId.isEmpty() || !beforeTimestamp.isValid())
        return result;

    QSqlDatabase db = connection();
    QSqlQuery query(db);
    // 分页游标同样按 (timestamp, rowid) 排序；rowid 由 beforeId 反查，接口保持只传 id。
    // 表名出现两次，用拼接而不是同一个 %1，避免占位符重复替换的歧义。
    const QString table = tableName();
    query.prepare(QStringLiteral(
        "SELECT * FROM ") + table
        + QStringLiteral(" WHERE session_id = ? "
                         "AND (timestamp < ? OR (timestamp = ? "
                         "AND rowid < (SELECT rowid FROM ") + table
        + QStringLiteral(" WHERE id = ?))) "
                         "ORDER BY timestamp DESC, rowid DESC LIMIT ?"));
    const QString ts = beforeTimestamp.toString(Qt::ISODate);
    query.addBindValue(sessionId);
    query.addBindValue(ts);
    query.addBindValue(ts);
    query.addBindValue(beforeId);
    query.addBindValue(limit);

    if (!query.exec()) {
        DatabaseError("selectBefore", query.lastError());
        return result;
    }
    while (query.next())
        result.append(parseFromQuery(query));

    std::reverse(result.begin(), result.end());
    return result;
}

// ======================================
// 业务扩展方法：流式增量追加内容
// ======================================
bool ChatMessageDao::appendContent(const QString& messageId, const QString& chunk)
{
    if (messageId.isEmpty() || chunk.isEmpty()) return false;

    QSqlDatabase db = connection();
    QSqlQuery query(db);

    // 使用 SQLite 原生字符串拼接，原子更新，避免读写竞争,||为SQL字符串拼接运算符
    QString sql = QString("UPDATE %1 SET content = content || ? WHERE id = ?").arg(tableName());

    query.prepare(sql);
    query.addBindValue(chunk);
    query.addBindValue(messageId);

    if (!query.exec()) {
        DatabaseError("appendContent", query.lastError());
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool ChatMessageDao::updateContent(const QString &messageId, const QString &fullText)
{
    if (messageId.isEmpty() || fullText.isEmpty())
    {
        Utils::writeLog("ERROR","ChatMessageDao",
            QString("id:%1 内容:%2").arg(messageId).arg(fullText));
        return false;
    }

    QSqlDatabase db = connection();
    QSqlQuery query(db);

    // 使用 SQLite 原生字符串拼接，原子更新，避免读写竞争
    QString sql = QString("UPDATE %1 SET content = ? WHERE id = ?").arg(tableName());

    query.prepare(sql);
    query.addBindValue(fullText);
    query.addBindValue(messageId);

    if (!query.exec()) {
        DatabaseError("updateContent", query.lastError());
        return false;
    }
    bool ok=query.numRowsAffected() > 0;
    return ok;

}

// ======================================
// 业务扩展方法：更新消息生成状态
// ======================================
bool ChatMessageDao::updateStatus(const QString& messageId,
        GlobalConstants::MessageGenerationStatus status)
{
    if (messageId.isEmpty()) return false;

    QSqlDatabase db = connection();
    QSqlQuery query(db);

    QString sql = QString("UPDATE %1 SET status = ? WHERE id = ?").arg(tableName());

    query.prepare(sql);
    query.addBindValue(static_cast<int>(status));
    query.addBindValue(messageId);

    if (!query.exec()) {
        DatabaseError("updateStatus", query.lastError());
        return false;
    }
    return query.numRowsAffected() > 0;
}

// ======================================
// 业务扩展方法：按会话批量删除消息
// ======================================
bool ChatMessageDao::removeBySessionId(const QString& sessionId)
{
    if (sessionId.isEmpty()) return false;

    QSqlDatabase db = connection();
    QSqlQuery query(db);

    QString sql = QString("DELETE FROM %1 WHERE session_id = ?").arg(tableName());

    query.prepare(sql);
    query.addBindValue(sessionId);

    if (!query.exec()) {
        DatabaseError("removeBySessionId", query.lastError());
        return false;
    }
    return query.numRowsAffected() > 0;
}

// ======================================
// 业务扩展方法：统计会话消息总数
// ======================================
int ChatMessageDao::countBySessionId(const QString& sessionId)
{
    if (sessionId.isEmpty()) return 0;

    QSqlDatabase db = connection();
    QSqlQuery query(db);

    QString sql = QString("SELECT COUNT(*) FROM %1 WHERE session_id = ?").arg(tableName());

    query.prepare(sql);
    query.addBindValue(sessionId);

    if (!query.exec()) {
        DatabaseError("countBySessionId", query.lastError());
        return 0;
    }

    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

// ======================================
// 业务扩展方法：获取会话最后一条消息
// ======================================
ChatMessageEntity ChatMessageDao::selectLastBySessionId(const QString& sessionId)
{
    if (sessionId.isEmpty()) return {};

    QSqlDatabase db = connection();
    QSqlQuery query(db);

    QString sql = QString(
                      "SELECT * FROM %1 "
                      "WHERE session_id = ? "
                      "ORDER BY timestamp DESC, rowid DESC "
                      "LIMIT 1"
                      ).arg(tableName());

    query.prepare(sql);
    query.addBindValue(sessionId);

    if (!query.exec()) {
        DatabaseError("selectLastBySessionId", query.lastError());
        return {};
    }

    if (query.next()) {
        return parseFromQuery(query);
    }
    return {};
}

// ======================================
// 业务扩展方法：获取会话第一条消息
// ======================================
ChatMessageEntity ChatMessageDao::selectFirstBySessionId(const QString& sessionId)
{
    if (sessionId.isEmpty()) return {};

    QSqlDatabase db = connection();
    QSqlQuery query(db);

    QString sql = QString(
                      "SELECT * FROM %1 "
                      "WHERE session_id = ? "
                      // 同一秒内多条消息按插入顺序判定谁在前（uuid 是随机的，不能用）
                      "ORDER BY timestamp ASC, rowid ASC "
                      "LIMIT 1"
                      ).arg(tableName());

    query.prepare(sql);
    query.addBindValue(sessionId);

    if (!query.exec()) {
        DatabaseError("selectFirstBySessionId", query.lastError());
        return {};
    }

    if (query.next()) {
        return parseFromQuery(query);
    }
    return {};
}

bool ChatMessageDao::clearBySessionId(const QString& sessionId)
{
    QSqlDatabase db = connection();
    QSqlQuery query(db);

    query.prepare("DELETE FROM chat_messages WHERE session_id = :sessionId");
    query.bindValue(":sessionId", sessionId);

    return query.exec();
}
