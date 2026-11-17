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

// In-Chat（absolute）提示词：ST 里由 depth + injection_order + role 决定落点
PromptEntity makeInChatPrompt(const QString& identifier, const QString& content,
                              GlobalConstants::MessageType role,
                              int depth, int order = 100)
{
    PromptEntity p = makePrompt(identifier, content, role, 1);
    p.injection_depth = depth;
    p.injection_order = order;
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

SMessage makeSMessage(const QString& identifier, const QString& content,
                      GlobalConstants::MessageType role = GlobalConstants::MessageType::System,
                      const QString& name = QString())
{
    SMessage m;
    m.identifier = identifier;
    m.role = role;
    m.content = content;
    m.name = name;
    m.tokens = content.size();
    return m;
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

        // 示例一律以 system 角色发送，说话人靠 name 区分（ST openai.js:1119-1125）
        bool hasUserExample = false;
        bool hasAssistantExample = false;
        for (const ChatMessage& m : out) {
            if (m.textContent() == QStringLiteral("你好")
                && m.role == GlobalConstants::MessageType::System
                && m.senderName == QStringLiteral("User")) hasUserExample = true;
            if (m.textContent() == QStringLiteral("你好呀")
                && m.role == GlobalConstants::MessageType::System
                && m.senderName == QStringLiteral("Alice")) hasAssistantExample = true;
        }
        QVERIFY(hasUserExample);
        QVERIFY(hasAssistantExample);
    }

    // main 内容为空时 ST 不会凭空造默认文案（生成期没有 fallback）：
    // 空消息在扁平化时被跳过，剩下的第一条就是角色描述。
    void emptyMainIsNotSent()
    {
        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), {}, QString(), makePreset(), WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QString())});

        QVERIFY(!out.isEmpty());
        QVERIFY(!out[0].textContent().startsWith(QStringLiteral("Write")));
        QCOMPARE(out[0].textContent(), QStringLiteral("Alice 是个图书管理员"));
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

    // ST openai.js:851-877：同 depth 下 role 的可见顺序是 assistant → user → system
    // （ST 把 [system,user,assistant] 追加进 newest-first 数组后整体 reverse）
    void inChatRoleOrderMatchesSillyTavern()
    {
        QList<ChatMessageEntity> history;
        history.append(makeMessage(GlobalConstants::MessageType::User, QStringLiteral("hi")));

        CompletionPresetEntity preset = makePreset();
        preset.promptOrder = {QStringLiteral("main"), QStringLiteral("in-chat-system"),
                              QStringLiteral("in-chat-user"), QStringLiteral("in-chat-assistant")};

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), history, QString(), preset, WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN")),
             makeInChatPrompt(QStringLiteral("in-chat-system"), QStringLiteral("SYS"),
                              GlobalConstants::MessageType::System, 0),
             makeInChatPrompt(QStringLiteral("in-chat-user"), QStringLiteral("USR"),
                              GlobalConstants::MessageType::User, 0),
             makeInChatPrompt(QStringLiteral("in-chat-assistant"), QStringLiteral("ASST"),
                              GlobalConstants::MessageType::Assistant, 0)});

        QCOMPARE(out.size(), 9);   // main + 3 个角色卡锚点 + newChat + hi + 3 条注入
        QCOMPARE(out[out.size() - 3].textContent(), QStringLiteral("ASST"));
        QCOMPARE(out[out.size() - 2].textContent(), QStringLiteral("USR"));
        QCOMPARE(out[out.size() - 1].textContent(), QStringLiteral("SYS"));
    }

    // ST：同 depth 内 order 小的桶在上面；世界书 depth 条目并进 order=100 的同 role 消息里
    void inChatOrderBucketsAndWorldInfoMerge()
    {
        QList<ChatMessageEntity> history;
        history.append(makeMessage(GlobalConstants::MessageType::User, QStringLiteral("hi")));

        CompletionPresetEntity preset = makePreset();
        preset.promptOrder = {QStringLiteral("main"), QStringLiteral("hi-order"),
                              QStringLiteral("lo-order")};

        WorldInfoResult wi;
        WorldInfoDepthGroup depthGroup;
        depthGroup.depth = 0;
        depthGroup.insertRole = GlobalConstants::MessageType::System;
        depthGroup.entries = {QStringLiteral("WI")};
        wi.worldInfoDepth.append(depthGroup);

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), history, QString(), preset, wi,
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN")),
             makeInChatPrompt(QStringLiteral("hi-order"), QStringLiteral("O100"),
                              GlobalConstants::MessageType::System, 0, 100),
             makeInChatPrompt(QStringLiteral("lo-order"), QStringLiteral("O50"),
                              GlobalConstants::MessageType::System, 0, 50)});

        QCOMPARE(out[out.size() - 2].textContent(), QStringLiteral("O50"));
        QCOMPARE(out[out.size() - 1].textContent(), QStringLiteral("O100\nWI"));
    }

    // ST getPromptCollection：injection_trigger 非空时只在列出的生成类型下发送
    void injectionTriggerFiltersPrompts()
    {
        PromptEntity onlyContinue = makePrompt(QStringLiteral("only-continue"),
                                               QStringLiteral("ONLY-CONTINUE"));
        onlyContinue.injection_trigger = {QStringLiteral("continue")};

        CompletionPresetEntity preset = makePreset();
        preset.promptOrder = {QStringLiteral("main"), QStringLiteral("only-continue")};

        const QList<ChatMessage> normal = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), {}, QString(), preset, WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN")), onlyContinue});
        QVERIFY(!plainTexts(normal).contains(QStringLiteral("ONLY-CONTINUE")));

        const QList<ChatMessage> continued = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), {}, QString(), preset, WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN")), onlyContinue},
            QString(), QStringLiteral("continue"));
        QVERIFY(plainTexts(continued).contains(QStringLiteral("ONLY-CONTINUE")));
    }

    // continue 非 prefill：ST 把最新一条消息从历史摘出来，和 nudge 一起放到最末尾
    void continueNudgeMovesLastMessageToTail()
    {
        CompletionPresetEntity preset = makePreset();
        preset.continueNudgePrompt = QStringLiteral("[Continue {{lastChatMessage}}]");

        QList<ChatMessageEntity> history;
        history.append(makeMessage(GlobalConstants::MessageType::User, QStringLiteral("问")));
        history.append(makeMessage(GlobalConstants::MessageType::Assistant, QStringLiteral("答")));

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), history, QString(), preset, WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN"))},
            QString(), QStringLiteral("continue"));

        QCOMPARE(out.last().textContent(), QStringLiteral("[Continue 答]"));
        QCOMPARE(out[out.size() - 2].textContent(), QStringLiteral("答"));
        QCOMPARE(countText(out, QStringLiteral("答")), 1);   // 不能在历史里再出现一次
    }

    // ST 默认（names_behavior=DEFAULT）只有示例消息带 name，历史消息不带
    //（否则会把 "第一条消息" 这类 UI 文案当说话人发给接口）
    void onlyExamplesCarryName()
    {
        CharacterEntity character = makeCharacter();
        character.exampleDialogue = QStringLiteral("<START>\n{{user}}: 你好\n{{char}}: 你好呀\n");

        QList<ChatMessageEntity> history;
        history.append(makeMessage(GlobalConstants::MessageType::User, QStringLiteral("在吗")));

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            character, history, QString(), makePreset(), WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN"))});

        int namedExamples = 0;
        for (const ChatMessage& m : out) {
            if (m.textContent() == QStringLiteral("在吗")) {
                QVERIFY(!m.includeName);
            }
            if (m.textContent() == QStringLiteral("你好") || m.textContent() == QStringLiteral("你好呀")) {
                QVERIFY(m.includeName);
                ++namedExamples;
            }
        }
        QCOMPARE(namedExamples, 2);
    }

    // ============================================================
    // ST 的 squash_system_messages（openai.js:3983-4015）
    // ============================================================
    // A.U.T.O. 那类预设把一大段系统提示拆成很多条 prompt（每条 role=system、无 name），
    // ST 默认 squash_system_messages=true，会把连续的 system 消息用 '\n' 拼回一条。
    // 这里直接测 ChatCompletion::build 的合并规则，不掺 PromptBuilder 的锚点消息。
    void squashSystemMessagesMergesConsecutiveSystemPrompts()
    {
        ChatCompletion cc(4096, 1024);
        cc.ensureGroup(QStringLiteral("p1"));
        cc.ensureGroup(QStringLiteral("p2"));
        cc.ensureGroup(QStringLiteral("p3"));
        cc.ensureGroup(QStringLiteral("p4"));

        cc.insert(makeSMessage(QStringLiteral("p1"), QStringLiteral("<A>\nbody")), QStringLiteral("p1"));
        cc.insert(makeSMessage(QStringLiteral("p2"), QStringLiteral("</A>")), QStringLiteral("p2"));
        cc.insert(makeSMessage(QStringLiteral("p3"), QString()), QStringLiteral("p3"));   // 空内容丢掉
        cc.insert(makeSMessage(QStringLiteral("p4"), QStringLiteral("<B>"),
                               GlobalConstants::MessageType::User), QStringLiteral("p4"));

        const QList<ChatMessage> out = cc.build(
            {QStringLiteral("p1"), QStringLiteral("p2"), QStringLiteral("p3"), QStringLiteral("p4")},
            true);

        QCOMPARE(out.size(), 2);
        QCOMPARE(out[0].textContent(), QStringLiteral("<A>\nbody\n</A>"));
        QCOMPARE(out[1].textContent(), QStringLiteral("<B>"));
    }

    // 关掉 squash 时一条一条发
    void squashSystemMessagesOffKeepsPromptsSeparate()
    {
        ChatCompletion cc(4096, 1024);
        cc.ensureGroup(QStringLiteral("p1"));
        cc.ensureGroup(QStringLiteral("p2"));
        cc.insert(makeSMessage(QStringLiteral("p1"), QStringLiteral("<A>")), QStringLiteral("p1"));
        cc.insert(makeSMessage(QStringLiteral("p2"), QStringLiteral("</A>")), QStringLiteral("p2"));

        QCOMPARE(cc.build({QStringLiteral("p1"), QStringLiteral("p2")}, false).size(), 2);
        QCOMPARE(cc.build({QStringLiteral("p1"), QStringLiteral("p2")}, true).size(), 1);
    }

    // 预设里用 " " 占位的系统提示，ST 会照发（合并后表现为空行），我们也要保留
    void whitespaceOnlyPromptIsKept()
    {
        ChatCompletion cc(4096, 1024);
        cc.ensureGroup(QStringLiteral("p1"));
        cc.ensureGroup(QStringLiteral("p2"));
        cc.ensureGroup(QStringLiteral("p3"));
        cc.insert(makeSMessage(QStringLiteral("p1"), QStringLiteral(" ")), QStringLiteral("p1"));
        cc.insert(makeSMessage(QStringLiteral("p2"), QStringLiteral(" ")), QStringLiteral("p2"));
        cc.insert(makeSMessage(QStringLiteral("p3"), QStringLiteral("<X>")), QStringLiteral("p3"));

        const QList<ChatMessage> out = cc.build(
            {QStringLiteral("p1"), QStringLiteral("p2"), QStringLiteral("p3")}, true);

        QCOMPARE(out.size(), 1);
        QCOMPARE(out[0].textContent(), QStringLiteral(" \n \n<X>"));
    }

    // 带 name 的消息（示例区）和 newMainChat 标记不能被合并进去
    void namedMessagesAndMarkersAreNotSquashed()
    {
        ChatCompletion cc(4096, 1024);
        cc.ensureGroup(QStringLiteral("p1"));
        cc.ensureGroup(QStringLiteral("examples"));
        cc.ensureGroup(QStringLiteral("chatHistory"));
        cc.insert(makeSMessage(QStringLiteral("p1"), QStringLiteral("<A>")), QStringLiteral("p1"));
        cc.insert(makeSMessage(QStringLiteral("ex-1"), QStringLiteral("你好"),
                               GlobalConstants::MessageType::System, QStringLiteral("Alice")),
                  QStringLiteral("examples"));
        cc.insert(makeSMessage(QStringLiteral("newMainChat"), QStringLiteral("[Start a new Chat]")),
                  QStringLiteral("chatHistory"));

        const QList<ChatMessage> out = cc.build(
            {QStringLiteral("p1"), QStringLiteral("examples"), QStringLiteral("chatHistory")}, true);

        QCOMPARE(out.size(), 3);
        QCOMPARE(out[0].textContent(), QStringLiteral("<A>"));
        QCOMPARE(out[1].textContent(), QStringLiteral("你好"));
        QVERIFY(out[1].includeName);
        QCOMPARE(out[2].textContent(), QStringLiteral("[Start a new Chat]"));
    }

    // {{user}} 由调用方注入（PromptBuilder 不碰 SettingsService），示例区 name 也用它
    void userNameFlowsIntoMacrosAndExamples()
    {
        CharacterEntity character = makeCharacter();
        character.exampleDialogue = QStringLiteral("<START>\n{{user}}: 你好\n{{char}}: 你好呀\n");

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            character, {}, QString(), makePreset(), WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("{{user}} 和 {{char}}"))},
            QString(), QStringLiteral("normal"), QStringLiteral("Kirito"));

        QCOMPARE(out[0].textContent(), QStringLiteral("Kirito 和 Alice"));

        bool nameOk = false;
        for (const ChatMessage& m : out) {
            if (m.textContent() == QStringLiteral("你好")
                && m.senderName == QStringLiteral("Kirito"))
                nameOk = true;
        }
        QVERIFY(nameOk);
    }

    // ============================================================
    // ST 的宏执行域（PromptManager.js:1537-1556）
    // ============================================================
    // 只有集合里的提示词（= order 里 enabled 的）才会走 preparePrompt。
    // 被禁用的提示词不发送、**也不执行 {{setvar}}**。
    // 反例（修复前的真实 bug）：狐神抚把 actingsystem/hugou/huying/kill1 写在禁用条目里，
    // 我们照样执行，{{getvar}} 就渲染出 ST 根本不会有的文本。
    void disabledPromptsDoNotRunMacros()
    {
        PromptBuilder::instance().setVariableSession(QStringLiteral("test-disabled-macros"));

        CompletionPresetEntity preset = makePreset();
        preset.promptOrder = {QStringLiteral("getter"), QStringLiteral("setter")};

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), {}, QString(), preset, WorldInfoResult{},
            {
                makePrompt(QStringLiteral("getter"), QStringLiteral("A:{{getvar::leak}}")),
                makePrompt(QStringLiteral("setter"), QStringLiteral("{{setvar::leak::LEAKED}}"),
                           GlobalConstants::MessageType::System, 0, false),
            });

        // getter 渲染为空值（禁用条目的 setvar 没执行），且整条请求里没有 LEAKED
        const QStringList texts = plainTexts(out);
        QVERIFY(texts.contains(QStringLiteral("A:")));
        QVERIFY(!texts.contains(QStringLiteral("LEAKED")));
    }

    // ============================================================
    // 聊天级宏（chat-macros.js:103-116）
    // ============================================================
    void chatMacrosComeFromHistory()
    {
        PromptBuilder::instance().setVariableSession(QStringLiteral("test-chat-macros"));

        const QList<ChatMessageEntity> history = {
            makeMessage(GlobalConstants::MessageType::User, QStringLiteral("你好")),
            makeMessage(GlobalConstants::MessageType::Assistant, QStringLiteral("嗨")),
        };

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), history, QStringLiteral("你好"), makePreset(), WorldInfoResult{},
            {makePrompt(QStringLiteral("main"),
                        QStringLiteral("[L={{lastUserMessage}}][C={{lastCharMessage}}][M={{lastMessage}}]"))});

        QCOMPARE(out[0].textContent(), QStringLiteral("[L=你好][C=嗨][M=嗨]"));
    }

    // 历史消息也要过宏（ST 的 populateChatHistory 对每条调 preparePrompt，openai.js:955）
    // 修复前：开场白里的 {{user}} 会原样发进请求。
    void historyMessagesAreMacroSubstituted()
    {
        PromptBuilder::instance().setVariableSession(QStringLiteral("test-history-macros"));

        const QList<ChatMessageEntity> history = {
            makeMessage(GlobalConstants::MessageType::Assistant,
                        QStringLiteral("{{user}} 的开场白，{{char}} 在这里")),
            makeMessage(GlobalConstants::MessageType::User, QStringLiteral("{{user}} 你好")),
        };

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), history, QString(), makePreset(), WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN"))},
            QString(), QStringLiteral("normal"), QStringLiteral("Kirito"));

        const QStringList texts = plainTexts(out);
        QVERIFY(texts.contains(QStringLiteral("Kirito 的开场白，Alice 在这里")));
        QVERIFY(texts.contains(QStringLiteral("Kirito 你好")));
        for (const QString& text : texts)
            QVERIFY2(!text.contains(QStringLiteral("{{")),
                     qPrintable(QStringLiteral("残留宏: ") + text));
    }

    // 示例区的 New Example Chat 标记 identifier 必须是 'newChat'：
    // ST 的 squashSystemMessages 例外名单只认 newMainChat / newChat / groupNudge。
    void exampleMarkerIsNewChatAndBreaksSquash()
    {
        PromptBuilder::instance().setVariableSession(QStringLiteral("test-new-chat-marker"));

        CompletionPresetEntity preset = makePreset();
        preset.squashSystemMessages = true;

        CharacterEntity character = makeCharacter();
        character.exampleDialogue = QStringLiteral("<START>\n{{user}}: 你好\n{{char}}: 你好呀\n");

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            character, {}, QString(), preset, WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN"))});

        // 标记自成一消息，没有被并进前面的 system 大块
        QCOMPARE(countText(out, QStringLiteral("[Example Chat]")), 1);
        for (const ChatMessage& m : out) {
            if (m.textContent() == QStringLiteral("[Example Chat]")) continue;
            QVERIFY(!m.textContent().contains(QStringLiteral("[Example Chat]")));
        }
    }

    // ST 的 impersonate 正文来自 oai_settings.impersonation_prompt（openai.js:1413、1240）
    void impersonateUsesPresetPrompt()
    {
        PromptBuilder::instance().setVariableSession(QStringLiteral("test-impersonate"));

        CompletionPresetEntity preset = makePreset();
        preset.impersonationPrompt = QStringLiteral("IMITATE {{user}}");

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), {}, QString(), preset, WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN"))},
            QString(), QStringLiteral("impersonate"), QStringLiteral("Kirito"));

        QCOMPARE(countText(out, QStringLiteral("IMITATE Kirito")), 1);
    }

    // ST openai.js:1420-1421：worldInfoBefore/After 都要先过 formatWorldInfo(wi_format)。
    void worldInfoFormatWrapsBeforeAndAfter()
    {
        PromptBuilder::instance().setVariableSession(QStringLiteral("test-wi-format"));

        CompletionPresetEntity preset = makePreset();
        preset.wiFormat = QStringLiteral("<World_Lore_Database>\n{0}\n</World_Lore_Database>");

        WorldInfoResult wi;
        wi.worldInfoBefore = QStringLiteral("BEFORE");
        wi.worldInfoAfter = QStringLiteral("AFTER");

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), {}, QString(), preset, wi,
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN"))});

        const QStringList texts = plainTexts(out);
        QVERIFY(texts.contains(
            QStringLiteral("<World_Lore_Database>\nBEFORE\n</World_Lore_Database>")));
        QVERIFY(texts.contains(
            QStringLiteral("<World_Lore_Database>\nAFTER\n</World_Lore_Database>")));

        // format 为空串 → 原样输出（ST 的 default_wi_format 是 {0}，等价于不包）
        preset.wiFormat.clear();
        const QList<ChatMessage> raw = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), {}, QString(), preset, wi,
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN"))});
        QVERIFY(plainTexts(raw).contains(QStringLiteral("BEFORE")));
        QVERIFY(plainTexts(raw).contains(QStringLiteral("AFTER")));

        // format 里没有 {0} → 只剩 format 本身（ST stringFormat 的语义，别自作聪明）
        preset.wiFormat = QStringLiteral("<WI/>");
        const QList<ChatMessage> noPlaceholder = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), {}, QString(), preset, wi,
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN"))});
        QVERIFY(plainTexts(noPlaceholder).contains(QStringLiteral("<WI/>")));
        QVERIFY(!plainTexts(noPlaceholder).contains(QStringLiteral("BEFORE")));
    }

    // ST openai.js:893-895：new_chat_prompt 只过 substituteParams，空串时 getChat() 丢掉。
    // 我们以前把"显式空串"回退成默认值，狐神抚这种预设就会凭空多一条 [Start a new Chat]。
    void emptyNewChatPromptIsNotSent()
    {
        PromptBuilder::instance().setVariableSession(QStringLiteral("test-empty-new-chat"));

        CompletionPresetEntity preset = makePreset();
        preset.newChatPrompt.clear();

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), {}, QString(), preset, WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN"))});
        QVERIFY(!plainTexts(out).contains(QStringLiteral("[Start a new Chat]")));

        // 实体默认值（= ST default_new_chat_prompt）仍然要发
        const QList<ChatMessage> defaults = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), {}, QString(), makePreset(), WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN"))});
        QVERIFY(plainTexts(defaults).contains(QStringLiteral("[Start a new Chat]")));
    }

    // new_example_chat_prompt 同理：空串只丢标记，示例正文照发。
    void emptyExampleMarkerKeepsExamples()
    {
        PromptBuilder::instance().setVariableSession(QStringLiteral("test-empty-example-marker"));

        CompletionPresetEntity preset = makePreset();
        preset.newExampleChatPrompt.clear();

        CharacterEntity character = makeCharacter();
        character.exampleDialogue = QStringLiteral(
            "<START>\n{{user}}: 你好\n{{char}}: 你好呀\n");

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            character, {}, QString(), preset, WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN"))});

        const QStringList texts = plainTexts(out);
        QVERIFY(!texts.contains(QStringLiteral("[Example Chat]")));
        QVERIFY(texts.contains(QStringLiteral("你好")));
        QVERIFY(texts.contains(QStringLiteral("你好呀")));
    }

    // ST openai.js:1410-1411：scenario/personality 有格式模板时正文用模板。
    void scenarioAndPersonalityFormatsAreApplied()
    {
        PromptBuilder::instance().setVariableSession(QStringLiteral("test-scenario-format"));

        CompletionPresetEntity preset = makePreset();
        preset.scenarioFormat = QStringLiteral("<SCENARIO>{{scenario}}</SCENARIO>");
        preset.personalityFormat = QStringLiteral("<PERSONALITY>{{personality}}</PERSONALITY>");

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), {}, QString(), preset, WorldInfoResult{},
            {makePrompt(QStringLiteral("main"), QStringLiteral("MAIN"))});

        const QStringList texts = plainTexts(out);
        QVERIFY(texts.contains(QStringLiteral("<SCENARIO>图书馆</SCENARIO>")));
        QVERIFY(texts.contains(QStringLiteral("<PERSONALITY>温柔</PERSONALITY>")));
    }

    // ============================================================
    // 按 狐神抚 的形状回归：setvar 只在启用条目里生效 + 顺序 + 无残留宏 + lastUserMessage
    // ============================================================
    void fushinLikePresetMatchesSillyTavernShape()
    {
        PromptBuilder::instance().setVariableSession(QStringLiteral("test-fushin-shape"));

        CompletionPresetEntity preset = makePreset();
        preset.promptOrder = {
            QStringLiteral("vars-reset"),
            QStringLiteral("vars-disabled"),
            QStringLiteral("pre-history"),
            QStringLiteral("dialogueExamples"),
            QStringLiteral("chatHistory"),
            QStringLiteral("post-history"),
            QStringLiteral("assistant-prefill"),
        };

        const QList<ChatMessageEntity> history = {
            makeMessage(GlobalConstants::MessageType::Assistant,
                        QStringLiteral("{{user}} 的制服裙摆轻轻飘动")),
            makeMessage(GlobalConstants::MessageType::User, QStringLiteral("你好")),
        };

        const QList<PromptEntity> prompts = {
            makePrompt(QStringLiteral("vars-reset"),
                       QStringLiteral("{{setvar::style::}}{{setvar::leak::}}")),
            makePrompt(QStringLiteral("vars-disabled"),
                       QStringLiteral("{{setvar::leak::DISABLED-LEAK}}"),
                       GlobalConstants::MessageType::System, 0, false),
            makePrompt(QStringLiteral("pre-history"),
                       QStringLiteral("<pov>{{getvar::leak}}|{{getvar::style}}</pov>\n"
                                      "<req>{{lastUserMessage}}</req>")),
            makePrompt(QStringLiteral("dialogueExamples"), QString()),
            makePrompt(QStringLiteral("chatHistory"), QString()),
            makePrompt(QStringLiteral("post-history"), QStringLiteral("<after>END</after>")),
            makePrompt(QStringLiteral("assistant-prefill"), QStringLiteral("OUTPUT\n<think>"),
                       GlobalConstants::MessageType::Assistant),
        };

        const QList<ChatMessage> out = PromptBuilder::instance().buildChatMessages(
            makeCharacter(), history, QStringLiteral("你好"), preset, WorldInfoResult{}, prompts,
            QString(), QStringLiteral("normal"), QStringLiteral("Kirito"));

        const QStringList texts = plainTexts(out);
        // 顺序：pre-history → New Chat → 历史 → post-history → assistant 预填
        const int preIdx = texts.indexOf(QStringLiteral("<pov>|</pov>\n<req>你好</req>"));
        const int newChatIdx = texts.indexOf(QStringLiteral("[Start a new Chat]"));
        const int postIdx = texts.indexOf(QStringLiteral("<after>END</after>"));
        const int prefillIdx = texts.indexOf(QStringLiteral("OUTPUT\n<think>"));
        QVERIFY(preIdx >= 0);
        QVERIFY(newChatIdx > preIdx);
        QVERIFY(postIdx > newChatIdx);
        QVERIFY(prefillIdx > postIdx);

        // 禁用条目的 setvar 不生效，整条请求里也没有残留宏
        for (const QString& text : texts) {
            QVERIFY(!text.contains(QStringLiteral("DISABLED-LEAK")));
            QVERIFY2(!text.contains(QStringLiteral("{{")),
                     qPrintable(QStringLiteral("残留宏: ") + text));
        }
    }
};

QTEST_GUILESS_MAIN(PromptBuilderTest)
#include "PromptBuilderTest.moc"
