#include <QtTest>

#include "WorldInfoEntryListModel.h"

class WorldInfoEntryListModelTest : public QObject
{
    Q_OBJECT

private slots:
    void fieldUpdatesKeepTypedValues();
    void businessFieldsAreReadOnly();
};

static WorldInfoEntryEntity makeEntry()
{
    WorldInfoEntryEntity entry;
    entry.id = QStringLiteral("entry-1");
    entry.bookId = QStringLiteral("book-1");
    entry.name = QStringLiteral("old");
    entry.content = QStringLiteral("body");
    entry.isEnabled = true;
    entry.constant = false;
    entry.vectorized = false;
    entry.position = WorldInfoEntryEntity::AfterChar;
    entry.insertRole = GlobalConstants::MessageType::System;
    entry.insertDepth = 4;
    entry.scanDepth = -1;
    entry.sortOrder = 100;
    return entry;
}

void WorldInfoEntryListModelTest::fieldUpdatesKeepTypedValues()
{
    WorldInfoEntryListModel model;
    model.resetData({makeEntry()});

    QVERIFY(model.updateField(QStringLiteral("entry-1"),
                              WorldInfoEntryListModel::TriggerControlRole, 2));
    QCOMPARE(model.data(model.index(0), WorldInfoEntryListModel::TriggerControlRole).toInt(), 2);
    QVERIFY(model.entryById(QStringLiteral("entry-1")).vectorized);

    QVERIFY(model.updateField(QStringLiteral("entry-1"),
                              WorldInfoEntryListModel::InsertPositionRole, 7));
    QCOMPARE(model.data(model.index(0), WorldInfoEntryListModel::InsertPositionRole).toInt(), 7);
    QCOMPARE(model.entryById(QStringLiteral("entry-1")).insertRole,
             GlobalConstants::MessageType::User);

    QVERIFY(model.updateField(QStringLiteral("entry-1"),
                              WorldInfoEntryListModel::InsertDepthRole, 9));
    QVERIFY(model.updateField(QStringLiteral("entry-1"),
                              WorldInfoEntryListModel::ScanDepthRole, 128));
    QVERIFY(model.updateField(QStringLiteral("entry-1"),
                              WorldInfoEntryListModel::SortOrderRole, 3));

    QCOMPARE(model.data(model.index(0), WorldInfoEntryListModel::InsertDepthRole).toInt(), 9);
    QCOMPARE(model.data(model.index(0), WorldInfoEntryListModel::ScanDepthRole).toInt(), 128);
    QCOMPARE(model.data(model.index(0), WorldInfoEntryListModel::SortOrderRole).toInt(), 3);
}

void WorldInfoEntryListModelTest::businessFieldsAreReadOnly()
{
    WorldInfoEntryListModel model;
    model.resetData({makeEntry()});

    // 业务字段只能由 ViewModel 的 updateField() 修改。
    QVERIFY(!model.setData(model.index(0), QStringLiteral("renamed"),
                           WorldInfoEntryListModel::NameRole));
    QCOMPARE(model.entryById(QStringLiteral("entry-1")).name, QStringLiteral("old"));

    // 展开态仍属于 UI 状态，允许模型自己保存。
    QVERIFY(model.setData(model.index(0), true,
                          WorldInfoEntryListModel::ExpandedStateRole));
    QVERIFY(model.data(model.index(0), WorldInfoEntryListModel::ExpandedStateRole).toBool());
}

QTEST_APPLESS_MAIN(WorldInfoEntryListModelTest)

#include "WorldInfoEntryListModelTest.moc"
