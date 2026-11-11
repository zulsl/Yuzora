#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "BaseDao.h"
#include "IConnectionProvider.h"
#include "Utils.h"

// BaseDao 只在写文件日志时调用 Utils；测试不需要真实日志依赖。
void Utils::writeLog(const QString&, const QString&, const QString&)
{
}

struct TestEntity
{
    QString id;
    QString name;
};

class TestProvider : public IConnectionProvider
{
public:
    TestProvider()
    {
        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                       QStringLiteral("base-dao-test"));
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());
        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral(
            "CREATE TABLE test_entities (id TEXT PRIMARY KEY, name TEXT)")));
    }

    ~TestProvider() override
    {
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(QStringLiteral("base-dao-test"));
    }

    QSqlDatabase connection() override { return db; }

private:
    QSqlDatabase db;
};

class TestDao : public BaseDao<TestEntity>
{
protected:
    QString tableName() const override { return QStringLiteral("test_entities"); }
    QString primaryKey() const override { return QStringLiteral("id"); }
    QStringList columnNames() const override
    {
        return {QStringLiteral("id"), QStringLiteral("name")};
    }
    void bindValues(QSqlQuery& query, const TestEntity& entity) const override
    {
        query.addBindValue(entity.id);
        query.addBindValue(entity.name);
    }
    TestEntity parseFromQuery(const QSqlQuery& query) const override
    {
        return {query.value(QStringLiteral("id")).toString(),
                query.value(QStringLiteral("name")).toString()};
    }
};

class BaseDaoProviderTest : public QObject
{
    Q_OBJECT

private slots:
    void crudUsesInjectedProvider();
};

void BaseDaoProviderTest::crudUsesInjectedProvider()
{
    TestProvider provider;
    TestDao dao;
    dao.setConnectionProvider(&provider);

    TestEntity entity{QStringLiteral("id-1"), QStringLiteral("before")};
    QVERIFY(dao.insert(entity));

    const QList<TestEntity> all = dao.selectAll();
    QCOMPARE(all.size(), 1);
    QCOMPARE(all.first().name, QStringLiteral("before"));

    entity.name = QStringLiteral("after");
    QVERIFY(dao.update(entity));
    QCOMPARE(dao.selectById(QStringLiteral("id-1")).name, QStringLiteral("after"));

    QVERIFY(dao.remove(QStringLiteral("id-1")));
    QVERIFY(dao.selectAll().isEmpty());
}

QTEST_GUILESS_MAIN(BaseDaoProviderTest)

#include "BaseDaoProviderTest.moc"
