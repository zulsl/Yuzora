#include <QtTest>
#include <QDateTime>
#include <QRegularExpression>

#include "PromptBuilder.h"
#include "Utils.h"
#include "CharacterEntity.h"
#include "ChatMessageEntity.h"
#include "CompletionPresetEntity.h"
#include "PromptEntity.h"

// ======================================
// 链接桩：PromptBuilder.cpp 只依赖 Utils 的两个函数
// ======================================
// replacePromptVariables 直接照抄生产实现（宏替换语义就是被测内容之一）
QString Utils::replacePromptVariables(const QString& templateStr, const QVariantMap& vars)
{
    QString result = templateStr;
    QRegularExpression re("\\{\\{(\\w+)\\}\\}");
    QRegularExpressionMatchIterator it = re.globalMatch(templateStr);
    int offset = 0;
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        const QString key = match.captured(1);
        if (vars.contains(key)) {
            const QString value = vars[key].toString();
            result.replace(match.capturedStart(0) + offset,
                           match.capturedLength(0), value);
            offset += value.length() - match.capturedLength(0);
        }
    }
    return result;
}

int Utils::calculateToken(const QString& text, GlobalConstants::ModelType)
{
    return qMax(1, text.length() / 4);
}

// ======================================
// fixture 工具
// ======================================
namespace {

CompletionPresetEntity makePreset(int context = 4096, int response = 1024)
{
    CompletionPresetEntity preset;
    preset.id = QStringLiteral("preset-1");
    preset.name = QStringLiteral("test");
    preset.maxContextToken = context;
    preset.maxResponseToken = response;
    preset.promptOrder = {QStringLiteral("main")};
    return preset;
}

PromptEntity makePrompt(const QString& identifier, const QString& content,
                        GlobalConstants::MessageType role = GlobalConstants::MessageType::System,
                        int injectionPosition = 0, bool active = true)
{
    PromptEntity p;
    p.id = identifier;
    p.identifier = identifier;
    p.content = content;
    p.role = role;
    p.injection_position = injectionPosition;
    p.isActive = active;
    p.system_prompt = true;
    return p;
}

CharacterEntity makeCharacter()
{
    CharacterEntity c;
    c.id = QStringLiteral("char-1");
    c.name = QStringLiteral("Alice");
    c.description = QStringLiteral("Alice 是个图书管理员");
    c.personality = QStringLiteral("温柔");
    c.scenario = QStringLiteral("图书馆");
    return c;
}

ChatMessageEntity makeMessage(GlobalConstants::MessageType type, const QString& content)
{
    ChatMessageEntity m;
    m.id = QString::number(qHash(content));
    m.type = type;
    m.status = GlobalConstants::MessageGenerationStatus::Normal;
    m.content = content;
    m.senderName = (type == GlobalConstants::MessageType::User)
        ? QStringLiteral("User") : QStringLiteral("Alice");
    m.timestamp = QDateTime::currentDateTime();
    m.isHidden = false;
    return m;
}

QStringList plainTexts(const QList<ChatMessage>& messages)
{
    QStringList out;
    for (const ChatMessage& m : messages) out.append(m.textContent());
    return out;
}

int countText(const QList<ChatMessage>& messages, const QString& text)
{
    int count = 0;
    for (const ChatMessage& m : messages)
        if (m.textContent() == text) ++count;
    return count;
}

} // namespace

// ======================================
// 用例
// ======================================
class PromptBuilderTest : public QObject {
    Q_OBJECT

private slots:
    // 默认预设（顺序表只有 main）→ 系统区顺序应为 ST 的默认顺序
    void orderFollowsSillyTavernDefault()
    {
        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), {}, QString(), makePreset(), WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("扮演 {{char}}"))});

        const QStringList texts = plainTexts(out);
        QCOMPARE(texts.size(), 5);
        QCOMPARE(texts[0], QStringLiteral("扮演 Alice"));      // 宏替换 + main 最前
        QCOMPARE(texts[1], QStringLiteral("Alice 是个图书管理员"));
        QCOMPARE(texts[2], QStringLiteral("温柔"));
        QCOMPARE(texts[3], QStringLiteral("图书馆"));
        QCOMPARE(texts[4], QStringLiteral("[Start a new Chat]"));  // 历史里的 New Chat 标记

        QCOMPARE(static_cast<int>(out[0].role), static_cast<int>(GlobalConstants::MessageType::System));
        QCOMPARE(static_cast<int>(out[4].role), static_cast<int>(GlobalConstants::MessageType::System));
    }

    // 回归 E1：worldInfoBefore 不能被 main 覆盖掉；worldInfoAfter 要落在角色定义之后
    void worldInfoBeforeIsNotOverwritten()
    {
        WorldInfoResult wi;
        wi.worldInfoBefore = QStringLiteral("WIB-CONTENT");
        wi.worldInfoAfter = QStringLiteral("WIA-CONTENT");

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), {}, QString(), makePreset(), wi,
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN"))});

        const QStringList texts = plainTexts(out);
        QCOMPARE(texts.size(), 7);
        QCOMPARE(texts[0], QStringLiteral("MAIN"));
        QCOMPARE(texts[1], QStringLiteral("WIB-CONTENT"));
        QCOMPARE(texts[5], QStringLiteral("WIA-CONTENT"));   // desc/pers/scen 之后的 after 段
        QCOMPARE(texts[6], QStringLiteral("[Start a new Chat]"));
    }

    // 回归 E2：dialogueExamples 不能顶掉别人的提示词，且要排在历史之前
    void dialogueExamplesDoNotClobberOthers()
    {
        CharacterEntity character = makeCharacter();
        character.exampleDialogue = QStringLiteral(
            "<START>\n{{user}}: 你好\n{{char}}: 你好呀\n"
            "<START>\n{{user}}: 借书\n{{char}}: 请便\n");

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            character, {}, QString(), makePreset(), WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN"))});

        // 两个示例块，各带一条 "[Example Chat]" 标记
        QCOMPARE(countText(out, QStringLiteral("[Example Chat]")), 2);

        int lastExampleIdx = -1;
        int historyIdx = -1;
        for (int i = 0; i < out.size(); ++i) {
            if (out[i].textContent() == QStringLiteral("[Example Chat]")) lastExampleIdx = i;
            if (out[i].textContent() == QStringLiteral("[Start a new Chat]")) historyIdx = i;
        }
        QVERIFY(lastExampleIdx > 0);
        QVERIFY(historyIdx > lastExampleIdx);

        // 示例里 user/assistant 角色要保留
        bool hasUserExample = false;
        bool hasAssistantExample = false;
        for (const ChatMessage& m : out) {
            if (m.textContent() == QStringLiteral("你好")
                && m.role == GlobalConstants::MessageType::User) hasUserExample = true;
            if (m.textContent() == QStringLiteral("你好呀")
                && m.role == GlobalConstants::MessageType::Assistant) hasAssistantExample = true;
        }
        QVERIFY(hasUserExample);
        QVERIFY(hasAssistantExample);
    }

    // main 内容为空时要用默认文案兜底，不能一条系统提示都不发
    void emptyMainFallsBackToDefault()
    {
        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), {}, QString(), makePreset(), WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QString())});

        QVERIFY(!out.isEmpty());
        QVERIFY(out[0].textContent().contains(QStringLiteral("Alice")));
        QVERIFY(out[0].textContent().startsWith(QStringLiteral("Write")));
    }

    // 顺序表里出现未知 identifier 时不能生成空壳提示词
    void unknownOrderIdentifiersAreIgnored()
    {
        CompletionPresetEntity preset = makePreset();
        preset.promptOrder = {QStringLiteral("no-such-prompt"),
                              QStringLiteral("main"),
                              QStringLiteral("worldInfoBefore")};

        WorldInfoResult wi;
        wi.worldInfoBefore = QStringLiteral("WIB");

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), {}, QString(), preset, wi,
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN"))});

        const QStringList texts = plainTexts(out);
        QVERIFY(!texts.contains(QString()));
        QCOMPARE(texts[0], QStringLiteral("MAIN"));
        QCOMPARE(texts[1], QStringLiteral("WIB"));
    }

    // 用户自定义提示词按顺序表里的位置发送
    void userPromptsKeepTheirOrder()
    {
        CompletionPresetEntity preset = makePreset();
        preset.promptOrder = {QStringLiteral("u1"), QStringLiteral("main"), QStringLiteral("u2")};

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), {}, QString(), preset, WorldInfoResult{},
            {makePrompt(QStringLiteral("u1"), QStringLiteral("U1"),
                        GlobalConstants::MessageType::User),
             makePrompt(QStringLiteral("main"), QStringLiteral("MAIN")),
             makePrompt(QStringLiteral("u2"), QStringLiteral("U2"),
                        GlobalConstants::MessageType::User)});

        const QStringList texts = plainTexts(out);
        // 三条自定义提示词之间的相对顺序要与顺序表一致
        QVERIFY(texts.indexOf(QStringLiteral("U1")) < texts.indexOf(QStringLiteral("MAIN")));
        QVERIFY(texts.indexOf(QStringLiteral("MAIN")) < texts.indexOf(QStringLiteral("U2")));
        // 锚点插在 main 之后，而不是全跑到末尾
        QCOMPARE(texts[2], QStringLiteral("Alice 是个图书管理员"));
    }

    // 历史按预算从新到旧填充，最新一条必须保留
    void historyIsTrimmedByBudget()
    {
        CompletionPresetEntity preset = makePreset(140, 40);

        QList<ChatMessageEntity> history;
        for (int i = 0; i < 12; ++i) {
            history.append(makeMessage(
                i % 2 == 0 ? GlobalConstants::MessageType::User
                           : GlobalConstants::MessageType::Assistant,
                QStringLiteral("第 %1 条消息，内容长度差不多够一个 token 组").arg(i)));
        }

        CharacterEntity character = makeCharacter();
        character.description.clear();
        character.personality.clear();
        character.scenario.clear();

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            character, history, QString(), preset, WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("M"))});

        // 最新一条一定在，且不会把 12 条全部塞进去
        QVERIFY(out.last().textContent().contains(QStringLiteral("第 11 条")));
        QVERIFY(out.size() < 13);
    }

    // 最新一条自身就超预算时也要保留（否则模型完全没有上下文）
    void oversizedNewestMessageIsKept()
    {
        CompletionPresetEntity preset = makePreset(80, 40);

        CharacterEntity character = makeCharacter();
        character.description.clear();
        character.personality.clear();
        character.scenario.clear();

        const QString huge(400, QLatin1Char('x'));
        QList<ChatMessageEntity> history;
        history.append(makeMessage(GlobalConstants::MessageType::User, huge));

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            character, history, QString(), preset, WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("M"))});

        QVERIFY(!out.isEmpty());
        QCOMPARE(out.last().textContent(), huge);
    }

    // continue + continuePrefill：把最后一条 assistant 消息挪到最末尾作为预填
    void continuePrefillMovesAssistantToTail()
    {
        CompletionPresetEntity preset = makePreset();
        preset.continuePrefill = true;

        QList<ChatMessageEntity> history;
        history.append(makeMessage(GlobalConstants::MessageType::User, QStringLiteral("问")));
        history.append(makeMessage(GlobalConstants::MessageType::Assistant, QStringLiteral("答")));

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), history, QString(), preset, WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("M"))},
            QString(), QStringLiteral("continue"));

        QCOMPARE(out.last().textContent(), QStringLiteral("答"));
        QCOMPARE(out.last().role, GlobalConstants::MessageType::Assistant);
        // 不能再在历史里出现一次
        QCOMPARE(countText(out, QStringLiteral("答")), 1);
        QVERIFY(countText(out, QStringLiteral("问")) == 1);
    }

    // 世界书里"示例消息之前/之后"的条目要落进示例区（旧实现匹配出来后无人消费）
    void worldInfoExampleEntriesAreUsed()
    {
        WorldInfoResult wi;
        WorldInfoExample example;
        example.content = QStringLiteral("EM-CONTENT");
        example.position = WorldInfoEntryEntity::EMBottom;
        wi.worldInfoExamples.append(example);

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), {}, QString(), makePreset(), wi,
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN"))});

        const QStringList texts = plainTexts(out);
        QVERIFY(texts.contains(QStringLiteral("EM-CONTENT")));
        // 应该在历史标记之前
        QVERIFY(texts.indexOf(QStringLiteral("EM-CONTENT"))
                < texts.indexOf(QStringLiteral("[Start a new Chat]")));
    }

    // 绝对位置（In-Chat）提示词按深度插进历史：depth 0 在最后一条之后
    void inChatPromptIsInsertedByDepth()
    {
        QList<ChatMessageEntity> history;
        history.append(makeMessage(GlobalConstants::MessageType::User, QStringLiteral("A")));
        history.append(makeMessage(GlobalConstants::MessageType::Assistant, QStringLiteral("B")));

        PromptEntity inChat = makePrompt(QStringLiteral("authorsNote"),
                                         QStringLiteral("NOTE"),
                                         GlobalConstants::MessageType::System,
                                         1);
        inChat.injection_depth = 0;

        CompletionPresetEntity preset = makePreset();
        preset.promptOrder = {QStringLiteral("main"), QStringLiteral("authorsNote")};

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), history, QString(), preset, WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("M")), inChat});

        QCOMPARE(out.last().textContent(), QStringLiteral("NOTE"));
    }
};

QTEST_GUILESS_MAIN(PromptBuilderTest)
#include "PromptBuilderTest.moc"
