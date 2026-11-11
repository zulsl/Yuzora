#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "SettingsDao.h"
#include "SettingsService.h"
#include "ConnectionProviderRegistry.h"
#include "IConnectionProvider.h"
#include "Utils.h"

void Utils::writeLog(const QString&, const QString&, const QString&)
{
}

QString Utils::getAvatarDirPath()
{
    return QStringLiteral(".");
}

class SettingsTestProvider : public IConnectionProvider
{
public:
    SettingsTestProvider()
    {
        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                       QStringLiteral("settings-service-test"));
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());
        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral(
            "CREATE TABLE settings ("
            "setting_key TEXT PRIMARY KEY, setting_value TEXT, value_type INTEGER,"
            "description TEXT, updated_at TEXT)")));
    }

    ~SettingsTestProvider() override
    {
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(QStringLiteral("settings-service-test"));
    }

    QSqlDatabase connection() override { return db; }

private:
    QSqlDatabase db;
};

class SettingsServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void writesAndReloadsThroughInjectedDao();
    void resetRestoresDefaults();
};

void SettingsServiceTest::writesAndReloadsThroughInjectedDao()
{
    SettingsTestProvider provider;
    SettingsDao dao;
    dao.setConnectionProvider(&provider);

    SettingsService service(&dao);
    service.setValue(GlobalConstants::USER_NAME, QStringLiteral("Alice"));
    QCOMPARE(service.getString(GlobalConstants::USER_NAME), QStringLiteral("Alice"));

    service.reloadFromDatabase();
    QCOMPARE(service.getString(GlobalConstants::USER_NAME), QStringLiteral("Alice"));
}

void SettingsServiceTest::resetRestoresDefaults()
{
    SettingsTestProvider provider;
    SettingsDao dao;
    dao.setConnectionProvider(&provider);

    SettingsService service(&dao);
    service.setValue(GlobalConstants::USER_NAME, QStringLiteral("Alice"));
    service.setValue(GlobalConstants::THEME_MODE, 2);

    service.resetAll();
    QCOMPARE(service.getInt(GlobalConstants::THEME_MODE),
             static_cast<int>(GlobalConstants::ThemeType::Default));
    QVERIFY(service.getString(GlobalConstants::USER_NAME).isEmpty());
}

QTEST_GUILESS_MAIN(SettingsServiceTest)

#include "SettingsServiceTest.moc"
