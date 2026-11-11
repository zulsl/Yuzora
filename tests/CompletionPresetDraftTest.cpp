#include <QtTest>

#include "CompletionPresetDraft.h"

class CompletionPresetDraftTest : public QObject
{
    Q_OBJECT

private slots:
    void roundTripPreservesUneditedFields();
    void setFieldHandlesTypes();
};

void CompletionPresetDraftTest::roundTripPreservesUneditedFields()
{
    CompletionPresetEntity base;
    base.id = QStringLiteral("preset-1");
    base.name = QStringLiteral("Default");
    base.wiFormat = QStringLiteral("keep-me");
    base.maxContextToken = 4096;

    CompletionPresetDraft draft;
    draft.setEntity(base);
    QCOMPARE(draft.name(), QStringLiteral("Default"));

    draft.setName(QStringLiteral("Renamed"));
    draft.setTemperature(1.0);

    const CompletionPresetEntity result = draft.toEntity(base);
    QCOMPARE(result.name, QStringLiteral("Renamed"));
    QCOMPARE(result.temperature, 1.0);
    QCOMPARE(result.wiFormat, QStringLiteral("keep-me"));
    QVERIFY(!draft.equals(base));
}

void CompletionPresetDraftTest::setFieldHandlesTypes()
{
    CompletionPresetDraft draft;
    QVERIFY(draft.setField(QStringLiteral("name"), QStringLiteral("P")));
    QVERIFY(draft.setField(QStringLiteral("maxContextToken"), 8192));
    QVERIFY(draft.setField(QStringLiteral("streamEnabled"), false));
    QVERIFY(draft.setField(QStringLiteral("temperature"), 1.25));
    QVERIFY(!draft.setField(QStringLiteral("unknown"), 1));

    QCOMPARE(draft.name(), QStringLiteral("P"));
    QCOMPARE(draft.maxContextToken(), 8192);
    QVERIFY(!draft.streamEnabled());
    QCOMPARE(draft.temperature(), 1.25);
}

QTEST_GUILESS_MAIN(CompletionPresetDraftTest)

#include "CompletionPresetDraftTest.moc"
