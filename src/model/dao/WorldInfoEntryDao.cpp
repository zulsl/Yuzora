#include "WorldInfoEntryDao.h"


WorldInfoEntryDao::WorldInfoEntryDao(QObject *parent)
    : QObject{parent}
{
}

QList<WorldInfoEntryEntity> WorldInfoEntryDao::selectEntriesByBookId(const QString & bookId)
{
    QList<WorldInfoEntryEntity> result;
    QSqlDatabase db = connection();
    QSqlQuery query(db);

    QString sql = QString("SELECT * FROM %1 WHERE book_id = ? ").arg(tableName());
    query.prepare(sql);
    query.addBindValue(bookId);

    if (!query.exec()) {
        DatabaseError("selectEntriesByBookId error:", query.lastError(),sql,query);
        return result;
    }

    while (query.next()) {
        result.append(parseFromQuery(query));
    }
    return result;

}

QString WorldInfoEntryDao::tableName() const
{
    return "world_info_entries";
}

bool WorldInfoEntryDao::removeByBookId(const QString &bookId)
{
    QSqlDatabase db = connection();
    QSqlQuery query(db);

    QString sql = QString("DELETE FROM %1 WHERE book_id = ?").arg(tableName());
    query.prepare(sql);
    query.addBindValue(bookId);

    if (!query.exec()) {
        DatabaseError("removeByBookId error:", query.lastError(), sql, query);
        return false;
    }
    return true;
}

bool WorldInfoEntryDao::replaceForBook(const QString &bookId,
                                       const QList<WorldInfoEntryEntity> &entries)
{
    QSqlDatabase db = connection();
    if (!db.isValid()) return false;

    // 重新导入同名世界书是"整体替换"：删一半再失败会把书变成半残，
    // 所以放在事务里。SQLite 驱动不支持事务时退化成顺序执行（行为与旧导入一致）。
    const bool inTransaction = db.transaction();

    QSqlQuery del(db);
    del.prepare(QString("DELETE FROM %1 WHERE book_id = ?").arg(tableName()));
    del.addBindValue(bookId);
    if (!del.exec()) {
        DatabaseError("replaceForBook/delete error:", del.lastError(), del.lastQuery(), del);
        if (inTransaction) db.rollback();
        return false;
    }

    for (const WorldInfoEntryEntity &entry : entries) {
        if (!insert(entry)) {
            if (inTransaction) db.rollback();
            return false;
        }
    }

    if (inTransaction && !db.commit()) {
        DatabaseError("replaceForBook/commit error:", db.lastError());
        db.rollback();
        return false;
    }
    return true;
}

QString WorldInfoEntryDao::primaryKey() const
{
    return "id";
}

QStringList WorldInfoEntryDao::columnNames() const
{
    return {
        "id", "book_id", "name", "keywords", "secondary_keywords", "content",
        "is_enabled", "constant", "vectorized", "position", "role", "insert_depth",
        "sort_order", "use_probability", "probability", "selective", "selective_logic",
        "scan_depth", "extensions"
    };
}

void WorldInfoEntryDao::bindValues(QSqlQuery &query, const WorldInfoEntryEntity &entity) const
{
    //严格顺序
    query.addBindValue(entity.id);
    query.addBindValue(entity.bookId);
    query.addBindValue(entity.name);
    query.addBindValue(Utils::stringListToJson(entity.keywords));
    query.addBindValue(Utils::stringListToJson(entity.secondaryKeywords));
    query.addBindValue(entity.content);
    query.addBindValue(entity.isEnabled ? 1 : 0);
    query.addBindValue(entity.constant?1:0);
    query.addBindValue(entity.vectorized?1:0);
    query.addBindValue(entity.position);
    query.addBindValue(entity.insertRole);
    query.addBindValue(entity.insertDepth);
    query.addBindValue(entity.sortOrder);
    query.addBindValue(entity.useProbability ? 1 : 0);
    query.addBindValue(entity.probability);
    query.addBindValue(entity.selective ? 1 : 0);
    query.addBindValue(entity.selectiveLogic);
    query.addBindValue(entity.scanDepth);
    query.addBindValue(Utils::variantMapToJson(entity.extensions));
}

#include "WorldInfoEntryEntity.h"
WorldInfoEntryEntity WorldInfoEntryDao::parseFromQuery(const QSqlQuery &query) const
{
    WorldInfoEntryEntity entity;
    entity.id = query.value("id").toString();
    entity.bookId = query.value("book_id").toString();
    entity.name = query.value("name").toString();
    entity.keywords = Utils::jsonToStringList(query.value("keywords"));
    entity.secondaryKeywords = Utils::jsonToStringList(query.value("secondary_keywords"));
    entity.content = query.value("content").toString();
    entity.isEnabled = query.value("is_enabled").toBool();
    entity.constant = query.value("constant").toBool();
    entity.vectorized = query.value("vectorized").toBool();
    entity.position = static_cast<WorldInfoEntryEntity::Position>(query.value("position").toInt());
    entity.insertRole = static_cast<GlobalConstants::MessageType>(query.value("role").toInt());
    entity.insertDepth = query.value("insert_depth").toInt();
    entity.sortOrder = query.value("sort_order").toInt();
    entity.useProbability = query.value("use_probability").toBool();
    entity.probability = query.value("probability").toInt();
    entity.selective = query.value("selective").toBool();
    entity.selectiveLogic = static_cast<WorldInfoEntryEntity::SelectiveLogic>(
        query.value("selective_logic").toInt());
    entity.scanDepth = query.value("scan_depth").toInt();
    entity.extensions = Utils::jsonToVariantMap(query.value("extensions"));
    return entity;
}
