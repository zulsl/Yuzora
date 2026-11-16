#include <QtTest>

#include "WorldInfoMatcher.h"
#include "WorldInfoEntryEntity.h"
#include "CharacterEntity.h"
#include "ChatMessageEntity.h"
#include "GlobalConstants.h"

namespace {

WorldInfoEntryEntity makeEntry(const QString& id, const QString& content,
                               WorldInfoEntryEntity::Position position,
                               GlobalConstants::MessageType role
                                   = GlobalConstants::MessageType::System,
                               int sortOrder = 100, int depth = 4)
{
    WorldInfoEntryEntity entry;
    entry.id = id;
    entry.name = id;
    entry.content = content;
    entry.position = position;
    entry.insertRole = role;
    entry.sortOrder = sortOrder;
    entry.insertDepth = depth;
    entry.constant = true;   // 常驻命中，测试只关心"分类/排序"，不关心关键词
    return entry;
}

ChatMessageEntity makeMessage(GlobalConstants::MessageType type,
                              const QString& sender, const QString& content)
{
    ChatMessageEntity msg;
    msg.type = type;
    msg.senderName = sender;
    msg.content = content;
    msg.status = GlobalConstants::MessageGenerationStatus::Normal;
    msg.isHidden = false;
    return msg;
}

} // namespace

class WorldInfoMatcherTest : public QObject {
    Q_OBJECT

private slots:
    // 同 depth、不同 role 的条目必须是两条独立消息（旧实现会合并、并沿用第一条的 role）
    void sameDepthDifferentRolesStaySeparate()
    {
        QList<WorldInfoEntryEntity> entries;
        entries.append(makeEntry(QStringLiteral("sys"), QStringLiteral("SYSTEM-AT-DEPTH"),
                                 WorldInfoEntryEntity::AtDepth,
                                 GlobalConstants::MessageType::System, 100, 4));
        entries.append(makeEntry(QStringLiteral("usr"), QStringLiteral("USER-AT-DEPTH"),
                                 WorldInfoEntryEntity::AtDepth,
                                 GlobalConstants::MessageType::User, 100, 4));

        const WorldInfoResult result = WorldInfoMatcher::instance().matchAndResolve(
            {}, entries, QString(), CharacterEntity{});

        QCOMPARE(result.worldInfoDepth.size(), 2);

        int systemGroups = 0;
        int userGroups = 0;
        for (const WorldInfoDepthGroup& group : result.worldInfoDepth) {
            QCOMPARE(group.depth, 4);
            if (group.insertRole == GlobalConstants::MessageType::System) {
                ++systemGroups;
                QCOMPARE(group.entries.join(QLatin1Char('|')), QStringLiteral("SYSTEM-AT-DEPTH"));
            } else if (group.insertRole == GlobalConstants::MessageType::User) {
                ++userGroups;
                QCOMPARE(group.entries.join(QLatin1Char('|')), QStringLiteral("USER-AT-DEPTH"));
            }
        }
        QCOMPARE(systemGroups, 1);
        QCOMPARE(userGroups, 1);
    }

    // 同 depth、同 role 仍然合并成一条
    void sameDepthSameRoleMerged()
    {
        QList<WorldInfoEntryEntity> entries;
        entries.append(makeEntry(QStringLiteral("a"), QStringLiteral("A"),
                                 WorldInfoEntryEntity::AtDepth,
                                 GlobalConstants::MessageType::System, 1, 4));
        entries.append(makeEntry(QStringLiteral("b"), QStringLiteral("B"),
                                 WorldInfoEntryEntity::AtDepth,
                                 GlobalConstants::MessageType::System, 2, 4));

        const WorldInfoResult result = WorldInfoMatcher::instance().matchAndResolve(
            {}, entries, QString(), CharacterEntity{});

        QCOMPARE(result.worldInfoDepth.size(), 1);
        QCOMPARE(result.worldInfoDepth.first().insertRole,
                 GlobalConstants::MessageType::System);
        QCOMPARE(result.worldInfoDepth.first().entries,
                 QStringList({QStringLiteral("A"), QStringLiteral("B")}));
    }

    // sortOrder 相同（默认全是 100）时，顺序必须与输入顺序一致，且多次调用完全一样
    void equalSortOrderKeepsInputOrder()
    {
        QList<WorldInfoEntryEntity> entries;
        for (int i = 0; i < 12; ++i) {
            entries.append(makeEntry(QStringLiteral("e%1").arg(i),
                                     QStringLiteral("C%1").arg(i),
                                     WorldInfoEntryEntity::BeforeChar,
                                     GlobalConstants::MessageType::System, 100));
        }

        QStringList firstRun;
        for (int round = 0; round < 5; ++round) {
            const WorldInfoResult result = WorldInfoMatcher::instance().matchAndResolve(
                {}, entries, QString(), CharacterEntity{});
            const QStringList parts = result.worldInfoBefore.split(QLatin1Char('\n'));
            if (round == 0) firstRun = parts;
            else QCOMPARE(parts, firstRun);
        }

        QStringList expected;
        for (int i = 0; i < 12; ++i) expected.append(QStringLiteral("C%1").arg(i));
        QCOMPARE(firstRun, expected);
    }

    // 示例区（EMTop/EMBottom）也要按 sortOrder 排序，并且同位置内稳定
    void exampleEntriesAreSorted()
    {
        QList<WorldInfoEntryEntity> entries;
        entries.append(makeEntry(QStringLiteral("em3"), QStringLiteral("EM-3"),
                                 WorldInfoEntryEntity::EMBottom,
                                 GlobalConstants::MessageType::System, 30));
        entries.append(makeEntry(QStringLiteral("em1"), QStringLiteral("EM-1"),
                                 WorldInfoEntryEntity::EMBottom,
                                 GlobalConstants::MessageType::System, 10));
        entries.append(makeEntry(QStringLiteral("em2"), QStringLiteral("EM-2"),
                                 WorldInfoEntryEntity::EMBottom,
                                 GlobalConstants::MessageType::System, 20));

        const WorldInfoResult result = WorldInfoMatcher::instance().matchAndResolve(
            {}, entries, QString(), CharacterEntity{});

        QCOMPARE(result.worldInfoExamples.size(), 3);
        QCOMPARE(result.worldInfoExamples[0].content, QStringLiteral("EM-1"));
        QCOMPARE(result.worldInfoExamples[1].content, QStringLiteral("EM-2"));
        QCOMPARE(result.worldInfoExamples[2].content, QStringLiteral("EM-3"));
    }

    // 最新用户输入已经落在历史里时，不能再被扫描第二遍
    void userInputIsNotScannedTwice()
    {
        QList<ChatMessageEntity> history;
        history.append(makeMessage(GlobalConstants::MessageType::User,
                                   QStringLiteral("User"),
                                   QStringLiteral("unique-marker")));

        const QString scanText = WorldInfoMatcher::instance().buildScanText(
            history, QStringLiteral("unique-marker"));

        QCOMPARE(scanText.count(QStringLiteral("unique-marker")), 1);
    }

    // 历史里还没有这轮输入时，仍然要把它补进扫描文本
    void userInputAddedWhenMissingFromHistory()
    {
        QList<ChatMessageEntity> history;
        history.append(makeMessage(GlobalConstants::MessageType::Assistant,
                                   QStringLiteral("Alice"),
                                   QStringLiteral("上一轮回复")));

        const QString scanText = WorldInfoMatcher::instance().buildScanText(
            history, QStringLiteral("本轮输入"));

        QVERIFY(scanText.contains(QStringLiteral("本轮输入")));
        QVERIFY(scanText.contains(QStringLiteral("上一轮回复")));
    }
};

QTEST_GUILESS_MAIN(WorldInfoMatcherTest)
#include "WorldInfoMatcherTest.moc"
