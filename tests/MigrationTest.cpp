#include <QtTest>

#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>

#include "MigrationRunner.h"
#include "SchemaMigrations.h"

class MigrationTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void freshDatabaseAppliesBaseline();
    void existingDatabaseWithoutHistoryAdoptsBaseline();
    void laterMigrationRunsAndCreatesBackup();
    void failedMigrationRollsBack();
    void newerDatabaseIsRejected();

private:
    QSqlDatabase db() const;
    QString dbPath() const;
    QVector<SchemaMigration> migrations() const;
    bool exec(const QString& sql);
    bool tableExists(const QString& table);
    bool columnExists(const QString& table, const QString& column);
    int migrationCount();
    int userVersion();
    QString migrationName(int version);

    QTemporaryDir m_dir;
    QString m_connectionName;
    QString m_dbPath;
};

void MigrationTest::init()
{
    QVERIFY(m_dir.isValid());
    m_connectionName = QStringLiteral("migration_test_%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    m_dbPath = m_dir.filePath(m_connectionName + QStringLiteral(".db"));

    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    database.setDatabaseName(m_dbPath);
    QVERIFY2(database.open(), qPrintable(database.lastError().text()));
}

void MigrationTest::cleanup()
{
    {
        QSqlDatabase database = QSqlDatabase::database(m_connectionName, false);
        if (database.isOpen()) {
            database.close();
        }
    }
    QSqlDatabase::removeDatabase(m_connectionName);
    m_connectionName.clear();
    m_dbPath.clear();
}

QSqlDatabase MigrationTest::db() const
{
    return QSqlDatabase::database(m_connectionName);
}

QString MigrationTest::dbPath() const
{
    return m_dbPath;
}

QVector<SchemaMigration> MigrationTest::migrations() const
{
    return SchemaMigrations::all();
}

bool MigrationTest::exec(const QString& sql)
{
    QSqlQuery query(db());
    return query.exec(sql);
}

bool MigrationTest::tableExists(const QString& table)
{
    QSqlQuery query(db());
    query.prepare(QStringLiteral(
        "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = :name LIMIT 1"));
    query.bindValue(QStringLiteral(":name"), table);
    if (!query.exec()) {
        return false;
    }
    return query.next();
}

bool MigrationTest::columnExists(const QString& table, const QString& column)
{
    QSqlQuery query(db());
    if (!query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table))) {
        return false;
    }
    while (query.next()) {
        if (query.value(1).toString() == column) {
            return true;
        }
    }
    return false;
}

int MigrationTest::migrationCount()
{
    QSqlQuery query(db());
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM schema_migrations")) || !query.next()) {
        return -1;
    }
    return query.value(0).toInt();
}

int MigrationTest::userVersion()
{
    QSqlQuery query(db());
    if (!query.exec(QStringLiteral("PRAGMA user_version")) || !query.next()) {
        return -1;
    }
    return query.value(0).toInt();
}

QString MigrationTest::migrationName(int version)
{
    QSqlQuery query(db());
    query.prepare(QStringLiteral("SELECT name FROM schema_migrations WHERE version = :version"));
    query.bindValue(QStringLiteral(":version"), version);
    if (!query.exec() || !query.next()) {
        return {};
    }
    return query.value(0).toString();
}

void MigrationTest::freshDatabaseAppliesBaseline()
{
    QSqlDatabase database = db();
    QString error;
    QVERIFY2(MigrationRunner::run(database, dbPath(), migrations(), &error),
             qPrintable(error));

    QCOMPARE(migrationCount(), 1);
    QCOMPARE(migrationName(1), QStringLiteral("baseline_v1"));
    QCOMPARE(userVersion(), 1);
    QVERIFY(tableExists(QStringLiteral("characters")));
    QVERIFY(tableExists(QStringLiteral("chat_messages")));
    QVERIFY(columnExists(QStringLiteral("characters"),
                         QStringLiteral("post_history_instructions")));
    QVERIFY(!tableExists(QStringLiteral("bad_table")));

    // 再跑一次必须是 no-op。
    QVERIFY2(MigrationRunner::run(database, dbPath(), migrations(), &error),
             qPrintable(error));
    QCOMPARE(migrationCount(), 1);
    QCOMPARE(userVersion(), 1);
}

void MigrationTest::existingDatabaseWithoutHistoryAdoptsBaseline()
{
    QSqlDatabase database = db();
    QString error;

    // 模拟第一个版本之前已经建过表的开发库：没有 schema_migrations。
    QVERIFY2(SchemaMigrations::applyBaseline(database, error), qPrintable(error));
    QVERIFY2(exec(QStringLiteral(
                 "INSERT INTO characters(id, name, created_at, updated_at) "
                 "VALUES('legacy-character', '旧角色', '2026-01-01', '2026-01-01')")),
             qPrintable(db().lastError().text()));
    QVERIFY2(exec(QStringLiteral("PRAGMA user_version = 2")),
             qPrintable(db().lastError().text()));

    QVERIFY2(MigrationRunner::run(database, dbPath(), migrations(), &error),
             qPrintable(error));

    QCOMPARE(migrationCount(), 1);
    QCOMPARE(migrationName(1), QStringLiteral("baseline_v1"));
    QCOMPARE(userVersion(), 1);

    QSqlQuery query(db());
    QVERIFY(query.exec(QStringLiteral(
        "SELECT name FROM characters WHERE id = 'legacy-character'")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("旧角色"));

    // 已有数据时执行基线前应留下 .bak-v0-* 备份。
    const QStringList backups = QDir(m_dir.path()).entryList(
        QStringList() << QStringLiteral("*.bak-v0-*"), QDir::Files);
    QVERIFY(!backups.isEmpty());
}

void MigrationTest::laterMigrationRunsAndCreatesBackup()
{
    QSqlDatabase database = db();
    QString error;
    QVERIFY2(MigrationRunner::run(database, dbPath(), migrations(), &error),
             qPrintable(error));

    QVector<SchemaMigration> list = migrations();
    list.append({2, QStringLiteral("add_test_table"),
                 [](QSqlDatabase& database, QString& migrationError) {
                     QSqlQuery query(database);
                     if (!query.exec(QStringLiteral(
                             "CREATE TABLE test_v2 (id INTEGER PRIMARY KEY)"))) {
                         migrationError = query.lastError().text();
                         return false;
                     }
                     return true;
                 }});

    QVERIFY2(MigrationRunner::run(database, dbPath(), list, &error),
             qPrintable(error));

    QCOMPARE(migrationCount(), 2);
    QCOMPARE(migrationName(2), QStringLiteral("add_test_table"));
    QCOMPARE(userVersion(), 2);
    QVERIFY(tableExists(QStringLiteral("test_v2")));

    const QStringList backups = QDir(m_dir.path()).entryList(
        QStringList() << QStringLiteral("*.bak-v1-*"), QDir::Files);
    QVERIFY(!backups.isEmpty());
}

void MigrationTest::failedMigrationRollsBack()
{
    QSqlDatabase database = db();
    QString error;
    QVERIFY2(MigrationRunner::run(database, dbPath(), migrations(), &error),
             qPrintable(error));

    QVector<SchemaMigration> list = migrations();
    list.append({2, QStringLiteral("broken_migration"),
                 [](QSqlDatabase& database, QString& migrationError) {
                     QSqlQuery query(database);
                     if (!query.exec(QStringLiteral("CREATE TABLE bad_table (id INTEGER)"))) {
                         migrationError = query.lastError().text();
                         return false;
                     }
                     migrationError = QStringLiteral("故意失败");
                     return false;
                 }});

    QVERIFY(!MigrationRunner::run(database, dbPath(), list, &error));
    QVERIFY(error.contains(QStringLiteral("故意失败")));
    QVERIFY(!tableExists(QStringLiteral("bad_table")));
    QCOMPARE(migrationCount(), 1);
    QCOMPARE(userVersion(), 1);
}

void MigrationTest::newerDatabaseIsRejected()
{
    QSqlDatabase database = db();
    QString error;
    QVERIFY2(MigrationRunner::run(database, dbPath(), migrations(), &error),
             qPrintable(error));
    QVERIFY2(exec(QStringLiteral(
                 "UPDATE schema_migrations SET version = 99, name = 'future'")),
             qPrintable(db().lastError().text()));

    QVERIFY(!MigrationRunner::run(database, dbPath(), migrations(), &error));
    QVERIFY(error.contains(QStringLiteral("高于程序支持")));
}

QTEST_GUILESS_MAIN(MigrationTest)
#include "MigrationTest.moc"
