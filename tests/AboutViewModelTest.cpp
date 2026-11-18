#include <QtTest>

#include "AboutViewModel.h"

class AboutViewModelTest : public QObject
{
    Q_OBJECT

private slots:
    void normalizesTags();
    void comparesVersions();
};

void AboutViewModelTest::normalizesTags()
{
    QCOMPARE(AboutViewModel::normalizeTag(QStringLiteral("v1.2.3")), QStringLiteral("1.2.3"));
    QCOMPARE(AboutViewModel::normalizeTag(QStringLiteral("V2.0")), QStringLiteral("2.0"));
    QCOMPARE(AboutViewModel::normalizeTag(QStringLiteral("refs/tags/v1.0.1")), QStringLiteral("1.0.1"));
    QCOMPARE(AboutViewModel::normalizeTag(QStringLiteral("  1.0.0  ")), QStringLiteral("1.0.0"));
    QVERIFY(AboutViewModel::normalizeTag(QString()).isEmpty());
}

void AboutViewModelTest::comparesVersions()
{
    QVERIFY(AboutViewModel::isNewerVersion(QStringLiteral("1.0.0"), QStringLiteral("v1.0.1")));
    QVERIFY(AboutViewModel::isNewerVersion(QStringLiteral("1.0.0"), QStringLiteral("1.1.0")));
    QVERIFY(AboutViewModel::isNewerVersion(QStringLiteral("1.9.9"), QStringLiteral("2.0")));
    QVERIFY(!AboutViewModel::isNewerVersion(QStringLiteral("1.0.0"), QStringLiteral("v1.0.0")));
    QVERIFY(!AboutViewModel::isNewerVersion(QStringLiteral("1.0.1"), QStringLiteral("v1.0.0")));
    QVERIFY(!AboutViewModel::isNewerVersion(QStringLiteral("1.0.0"), QStringLiteral("not-a-version")));
    QVERIFY(!AboutViewModel::isNewerVersion(QString(), QStringLiteral("1.0.0")));
}

QTEST_GUILESS_MAIN(AboutViewModelTest)
#include "AboutViewModelTest.moc"
