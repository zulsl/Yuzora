#include <QtTest>

#include "CharacterDraft.h"

class CharacterDraftTest : public QObject
{
    Q_OBJECT

private slots:
    void typedFieldsAndEntityRoundTrip();
    void setFieldHandlesTypes();
};

void CharacterDraftTest::typedFieldsAndEntityRoundTrip()
{
    CharacterEntity entity;
    entity.id = QStringLiteral("id-1");
    entity.name = QStringLiteral("Alice");
    entity.description = QStringLiteral("desc");
    entity.alternateGreetings = {QStringLiteral("hi")};
    entity.tags = {QStringLiteral("tag")};
    entity.cardFormatVersion = 3;

    CharacterDraft draft;
    QSignalSpy spy(&draft, &CharacterDraft::changed);
    draft.setEntity(entity);

    QCOMPARE(draft.name(), QStringLiteral("Alice"));
    QCOMPARE(draft.tags(), QStringList{QStringLiteral("tag")});
    QVERIFY(spy.count() > 0);

    draft.setName(QStringLiteral("Bob"));
    QCOMPARE(draft.toEntity().name, QStringLiteral("Bob"));
    QVERIFY(draft.toEntity() != entity);
}

void CharacterDraftTest::setFieldHandlesTypes()
{
    CharacterDraft draft;
    QVERIFY(draft.setField(QStringLiteral("name"), QStringLiteral("Alice")));
    QVERIFY(draft.setField(QStringLiteral("tags"),
                           QStringList{QStringLiteral("a"), QStringLiteral("b")}));
    QVERIFY(draft.setField(QStringLiteral("cardFormatVersion"), 4));
    QVERIFY(draft.setField(QStringLiteral("isFavorite"), true));
    QVERIFY(!draft.setField(QStringLiteral("unknown"), 1));

    QCOMPARE(draft.name(), QStringLiteral("Alice"));
    QCOMPARE(draft.tags().size(), 2);
    QCOMPARE(draft.cardFormatVersion(), 4);
    QVERIFY(draft.isFavorite());
}

QTEST_GUILESS_MAIN(CharacterDraftTest)

#include "CharacterDraftTest.moc"
