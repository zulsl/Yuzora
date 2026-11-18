#include "MigrationRunner.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QSqlError>
#include <QSqlQuery>

namespace {

constexpr auto kHistoryTable = "schema_migrations";

void setError(QString* error, const QString& text)
{
    if (error) {
        *error = text;
    }
}

bool execSql(QSqlDatabase& db, const QString& sql, QString& error)
{
    QSqlQuery query(db);
    if (query.exec(sql)) {
        return true;
    }

    error = QStringLiteral("SQL 执行失败：\n%1\n%2")
                .arg(sql, query.lastError().text());
    return false;
}

bool tableExists(QSqlDatabase& db, const QString& table,
                 bool& exists, QString& error)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = :name LIMIT 1"));
    query.bindValue(QStringLiteral(":name"), table);
    if (!query.exec()) {
        error = QStringLiteral("读取表 %1 是否存在失败：%2")
                    .arg(table, query.lastError().text());
        return false;
    }

    exists = query.next();
    return true;
}

bool createHistoryTable(QSqlDatabase& db, QString& error)
{
    return execSql(db, QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS schema_migrations (
            version INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            checksum TEXT,
            applied_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
        )
    )"), error);
}

bool readApplied(QSqlDatabase& db, QMap<int, QString>& applied, QString& error)
{
    applied.clear();

    QSqlQuery query(db);
    if (!query.exec(QStringLiteral(
            "SELECT version, name FROM schema_migrations ORDER BY version"))) {
        error = QStringLiteral("读取迁移记录失败：%1").arg(query.lastError().text());
        return false;
    }

    while (query.next()) {
        applied.insert(query.value(0).toInt(), query.value(1).toString());
    }
    return true;
}

bool insertApplied(QSqlDatabase& db, const SchemaMigration& migration, QString& error)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO schema_migrations(version, name) VALUES(:version, :name)"));
    query.bindValue(QStringLiteral(":version"), migration.version);
    query.bindValue(QStringLiteral(":name"), migration.name);
    if (!query.exec()) {
        error = QStringLiteral("写入迁移记录失败（v%1 %2）：%3")
                    .arg(QString::number(migration.version),
                         migration.name,
                         query.lastError().text());
        return false;
    }
    return true;
}

bool setUserVersion(QSqlDatabase& db, int version, QString& error)
{
    return execSql(db, QStringLiteral("PRAGMA user_version = %1").arg(version), error);
}

//验证迁移
bool validateMigrations(const QVector<SchemaMigration>& migrations, QString& error)
{
    if (migrations.isEmpty()) {
        error = QStringLiteral("迁移清单为空");
        return false;
    }

    int expected = 1;
    for (const SchemaMigration& migration : migrations) {
        if (migration.version != expected) {
            error = QStringLiteral("迁移版本必须从 1 开始连续递增：期望 %1，实际 %2")
                        .arg(expected)
                        .arg(migration.version);
            return false;
        }
        if (migration.name.trimmed().isEmpty() || !migration.apply) {
            error = QStringLiteral("迁移 v%1 缺少名称或执行函数").arg(migration.version);
            return false;
        }
        ++expected;
    }
    return true;
}

bool backupDatabase(QSqlDatabase& db, const QString& dbFilePath,
                    int fromVersion, QString& error)
{
    if (dbFilePath.isEmpty()) {
        return true;
    }

    const QFileInfo info(dbFilePath);
    if (!info.exists() || info.size() <= 0) {
        return true;
    }

    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"));
    QString backupPath = QStringLiteral("%1.bak-v%2-%3")
                             .arg(dbFilePath)
                             .arg(fromVersion)
                             .arg(stamp);
    int suffix = 1;
    while (QFile::exists(backupPath)) {
        backupPath = QStringLiteral("%1.bak-v%2-%3-%4")
                         .arg(dbFilePath)
                         .arg(fromVersion)
                         .arg(stamp)
                         .arg(suffix++);
    }

    // VACUUM INTO 生成的是一致快照；个别环境不支持时再退回文件复制。
    QString escapedPath = backupPath;
    escapedPath.replace(QLatin1Char('\''), QStringLiteral("''"));

    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("VACUUM INTO '%1'").arg(escapedPath))) {
        query.finish();

        QSqlQuery checkpoint(db);
        checkpoint.exec(QStringLiteral("PRAGMA wal_checkpoint(FULL)"));

        if (QFile::exists(backupPath)) {
            QFile::remove(backupPath);
        }
        if (!QFile::copy(dbFilePath, backupPath)) {
            error = QStringLiteral("迁移前备份数据库失败：%1").arg(backupPath);
            return false;
        }
    }

    return true;
}

} // namespace

bool MigrationRunner::run(
        QSqlDatabase& db,const QString& dbFilePath,
        const QVector<SchemaMigration>& migrations,QString* error)
{
    QString localError;
    if (!error) {
        error = &localError;
    }
    error->clear();

    if (!db.isOpen()) {
        setError(error, QStringLiteral("数据库连接未打开"));
        return false;
    }

    if (!validateMigrations(migrations, *error)) {
        return false;
    }

    const int latestVersion = migrations.last().version;

    bool hasHistory = false;
    bool hasCharacters = false;
    if (!tableExists(db, QString::fromLatin1(kHistoryTable), hasHistory, *error)
        || !tableExists(db, QStringLiteral("characters"), hasCharacters, *error)) {
        return false;
    }

    if (!hasHistory && !createHistoryTable(db, *error)) {
        return false;
    }

    QMap<int, QString> applied;
    if (!readApplied(db, applied, *error)) {
        return false;
    }

    // 空记录是合法的：这是第一次建库，正要执行 V1。
    const int maxApplied = applied.isEmpty() ? 0 : applied.lastKey();
    if (maxApplied > latestVersion) {
        setError(error, QStringLiteral(
            "数据库版本 %1 高于程序支持的版本 %2，请升级程序后再打开。")
            .arg(maxApplied)
            .arg(latestVersion));
        return false;
    }

    QMap<int, SchemaMigration> migrationByVersion;
    for (const SchemaMigration& migration : migrations) {
        migrationByVersion.insert(migration.version, migration);
    }

    for (auto it = applied.constBegin(); it != applied.constEnd(); ++it) {
        const auto migrationIt = migrationByVersion.constFind(it.key());
        if (migrationIt == migrationByVersion.constEnd()) {
            setError(error, QStringLiteral("数据库存在未知迁移版本 v%1").arg(it.key()));
            return false;
        }
        if (migrationIt->name != it.value()) {
            setError(error, QStringLiteral(
                "迁移记录与程序不一致：v%1 记录为“%2”，程序里是“%3”")
                .arg(it.key())
                .arg(it.value(), migrationIt->name));
            return false;
        }
    }

    // 不允许跳版本：v1..maxApplied 必须连续存在。
    for (int version = 1; version <= maxApplied; ++version) {
        if (!applied.contains(version)) {
            setError(error, QStringLiteral("数据库迁移记录缺少 v%1").arg(version));
            return false;
        }
    }

    QVector<SchemaMigration> pending;
    for (const SchemaMigration& migration : migrations) {
        if (!applied.contains(migration.version)) {
            pending.append(migration);
        }
    }

    // 第一次建空库不用备份；已有业务表时先备份再建/升。
    if (!pending.isEmpty() && hasCharacters) {
        if (!backupDatabase(db, dbFilePath, maxApplied, *error)) {
            return false;
        }
    }

    for (const SchemaMigration& migration : pending) {
        if (!db.transaction()) {
            setError(error, QStringLiteral("开启迁移 v%1 的事务失败：%2")
                .arg(migration.version)
                .arg(db.lastError().text()));
            return false;
        }

        //开始建表
        if (!migration.apply(db, *error)
            || !insertApplied(db, migration, *error)
            || !setUserVersion(db, migration.version, *error)) {
            db.rollback();
            return false;
        }

        if (!db.commit()) {
            const QString commitError = db.lastError().text();
            db.rollback();
            setError(error, QStringLiteral("提交迁移 v%1 失败：%2")
                .arg(migration.version)
                .arg(commitError));
            return false;
        }
    }

    if (!setUserVersion(db, latestVersion, *error)) {
        return false;
    }

    bool charactersExist = false;
    if (!tableExists(db, QStringLiteral("characters"), charactersExist, *error)) {
        return false;
    }
    if (!charactersExist) {
        setError(error, QStringLiteral("迁移完成后仍缺少 characters 表"));
        return false;
    }

    return true;
}
