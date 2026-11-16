#include "PromptBuilder.h"
#include "Utils.h"
#include <QDateTime>
#include <QDebug>
#include <algorithm>
#include <limits>
#include "GlobalConstants.h"
#include "AppException.h".h"

// ======================================
// 单例实现
// ======================================
PromptBuilder& PromptBuilder::instance()
{
    static PromptBuilder instance;
    return instance;
}

PromptBuilder::PromptBuilder(QObject* parent)
    : QObject(parent)
    , m_estimator([](const QString& text) {
        // 默认粗略估算：ASCII 4 字符/token，非 ASCII 2 字符/token
        // - 英文 / 半角：大约 4 个字符算 1 token
        // - 中文：大约 2 个汉字算 1 token
        int ascii = 0, nonAscii = 0;
        for (QChar c : text) {
            if (c.unicode() < 128) ascii++;
            else nonAscii++;
        }
        return (ascii + 3) / 4 + (nonAscii + 1) / 2;
    })
{
    loadDefaultTemplates();
    loadDefaultPromptsAndOrder();   // 新增：初始化酒馆风格的提示词库与顺序
}

// ======================================
// 核心方法：构建完整聊天消息
// ======================================
QList<ChatMessage> PromptBuilder::buildChatMessages(
    const CharacterEntity & character,
    const QList<ChatMessageEntity> & history,
    const QString & userInput,
    const CompletionPresetEntity & preset,
    const WorldInfoResult & wi,
    const QList<PromptEntity> & allPrompts,
    const QString & personaDesc,
    const QString & generationType)
{
    // Persona 还没有落到设置里，变量表先按 "User" 走；接入 persona 后改 getPromptVariables
    Q_UNUSED(personaDesc)

    //准备提示词
    QList<PromptEntity> prompts=preparePromptCollection(preset,wi,character,allPrompts);

    ChatCompletion cc(preset.maxContextToken, preset.maxResponseToken);

    //填充聊天消息
    PopulateContext pCtx;
    pCtx.wi = &wi;
    pCtx.userInput = userInput;
    pCtx.type = generationType.trimmed().isEmpty()
        ? QStringLiteral("normal") : generationType.trimmed();
    pCtx.messageExamples = parseExampleDialogue(character);

    return populateChatCompletion(prompts,cc,preset,history,userInput,character,wi,pCtx);
    //废弃
    // QList<ChatMessage> result;
    // addFixedMessage(result, budget, wi.worldInfoBefore);
    // addFixedMessage(result, budget, resolvePromptContent(preset, "main", character.systemPrompt));
    // addFixedMessage(result, budget, wi.worldInfoAfter);
    // addFixedMessage(result, budget, character.description);
    // addFixedMessage(result, budget, character.personality);
    // addFixedMessage(result, budget, character.scenario);
    // addFixedMessage(result, budget, personaDesc);
    // ============================================================
    // 阶段 2：nsfw
    // ============================================================
    // addFixedMessage(result, budget, resolvePromptContent(preset, "nsfw"));
    // ============================================================
    // 阶段 3：用户自定义的相对位置提示（systemPrompt=false 且 Relative）
    // ============================================================
    // QSet<QString> builtinIds = {"main", "nsfw", "jailbreak"};
    // for (const auto& p : m_prompts) {
    //     if (p.system_prompt) continue;
    //     if (!p.isActive) continue;
    //     if (p.injection_position == 1) continue;
    //     if (builtinIds.contains(p.identifier)) continue;
    //     addFixedMessage(result, budget, p.content, p.role, QString());
    // }
    // for (const auto& p : preset.prompts) {
    // ============================================================
    // 阶段 4：对话示例（可选，放历史前）
    // ============================================================
    // if (!character.exampleDialogue.isEmpty()) {
    //     addDialogueExamples(result, budget,character.exampleDialogue,character.name,"User");
    // }
    // ============================================================
    // 阶段 5：对话历史（带预算，从新到旧填充）
    // ============================================================
    // 有 userInput 且最后一条不是 user 时，补一条 user 消息
    // bool needSyntheticUser = !userInput.isEmpty() && (history.isEmpty() || history.last().type
    //     != GlobalConstants::MessageType::User);
    // addChatHistory(result, budget, history, userInput, /*addNewChatPrompt=*/false);
    // // 若历史被裁剪、最后一条不是 user，则补上 userInput
    // if (needSyntheticUser && budget.canAfford(estimateTokens(userInput) + PER_MESSAGE_OVERHEAD)) {
    //     ChatMessage um;
    //     um.role = GlobalConstants::MessageType::User;
    //     um.appendText(userInput);
    //     int t = estimateTokens(um.parts.first().data) + PER_MESSAGE_OVERHEAD;
    //     result.append(um);
    //     budget.consume(t);
    // }
    // // ============================================================
    // // 阶段 6：in-chat 绝对注入（预设 Absolute + 世界书 depth）
    // // ============================================================
    // applyInChatInjections(result, budget, preset, wi);
    // // ============================================================
    // // 阶段 7：jailbreak（默认最后）
    // // ============================================================
    // addFixedMessage(result, budget,
    //     resolvePromptContent(preset, "jailbreak",character.postHistoryInstructions));
    // return result;
}


QList<PromptEntity> PromptBuilder::preparePromptCollection(
    const CompletionPresetEntity & preset,
    const WorldInfoResult & wi,const CharacterEntity & character,
    const QList<PromptEntity> & uPrompts)
{
    QList<PromptEntity> systemPrompts;//临时系统提示词集合

    // ============================================================
    // 构造 systemPrompts 数组
    // ============================================================
    appendToCollection(systemPrompts,wi.worldInfoBefore,"worldInfoBefore");
    appendToCollection(systemPrompts,wi.worldInfoAfter,"worldInfoAfter");
    appendToCollection(systemPrompts,character.description,"charDescription");
    appendToCollection(systemPrompts,character.personality,"charPersonality");//个性
    appendToCollection(systemPrompts,character.scenario,"scenario");//场景

    appendToCollection(systemPrompts,"","impersonate");//(模仿)
    appendToCollection(systemPrompts,"","quietPrompt");//(静默提示)
    appendToCollection(systemPrompts,"","groupNudge");//
    appendToCollection(systemPrompts,"","bias",GlobalConstants::MessageType::Assistant);//偏置


    // ============================================================
    // 处理扩展提示（extensionPrompts）
    //'Summary',总结,1_memory
    // appendToCollection(collection,"","Summary");
    //'authorsNote',作者注释,2_floating_prompt
    // appendToCollection(collection,"","authorsNote");
    //'vectorsMemory',向量记忆,3_vectors
    // appendToCollection(collection,"","vectorsMemory");
    //'vectorsDataBank',向量数据库,4_vectors_data_bank
    // appendToCollection(collection,"","vectorsDataBank");
    //'smartContext',智能上下文,chromadb
    // appendToCollection(collection,"","smartContext");
    //'personaDescription',用户描述
    // appendToCollection(collection,"","personaDescription");


    //第三方自定义扩展提示处理
    //...


    // ============================================================
    // ==================开始合并================
    // ============================================================
    // 建立 identifier -> PromptEntity 的映射
    QMap<QString, PromptEntity> promptEntityMap;
    for (const PromptEntity & pe : uPrompts) {
        promptEntityMap[pe.identifier] = pe;
    }

    // ST 的默认顺序。顺序表里缺的系统锚点按这个顺序**插到对应位置**，
    // 而不是一律追加到末尾——否则 worldInfoAfter 会跑到角色描述前面、
    // jailbreak 会跑到聊天记录前面（都不是 ST 的语义）。
    static const QStringList kCanonicalOrder = {
        QStringLiteral("main"),
        QStringLiteral("worldInfoBefore"),
        QStringLiteral("personaDescription"),
        QStringLiteral("charDescription"),
        QStringLiteral("charPersonality"),
        QStringLiteral("scenario"),
        QStringLiteral("enhanceDefinitions"),
        QStringLiteral("nsfw"),
        QStringLiteral("worldInfoAfter"),
        QStringLiteral("dialogueExamples"),
        QStringLiteral("chatHistory"),
        QStringLiteral("jailbreak"),
    };
    auto canonicalIndex = [](const QString& id) {
        const int idx = kCanonicalOrder.indexOf(id);
        return idx < 0 ? std::numeric_limits<int>::max() : idx;
    };

    QList<PromptEntity> result;//排序完毕的提示词列表
    QSet<QString> seenIds;

    // 按顺序表取出用户配置的提示词。
    // 顺序表里出现、但预设里没有对应提示词的项（系统锚点/未知项）先跳过，
    // 交给下面的 systemPrompts / mandatory 补齐——否则会留下 identifier 为空的
    // 空壳，并把 seenIds 占掉，让真正的锚点永远补不进来。
    QStringList promptOrder = preset.promptOrder;
    // 如果 order 为空，用 prompts 的原始顺序作为 后备方案
    if (promptOrder.isEmpty()) {
        THROW(UnknownError,"promptOrder为空");
        for (const PromptEntity & pe : uPrompts) promptOrder.append(pe.identifier);
    }
    for (const QString& po : promptOrder) {
        if (seenIds.contains(po)) continue;
        if (!promptEntityMap.contains(po)) continue;
        result.append(promptEntityMap.value(po));
        seenIds.insert(po);
    }

    // 按 canonical 顺序插入一条提示词：插到"最后一个 canonical 不比它靠后"的项之后。
    // 这样即使用户在顺序表里插了自定义提示词（canonical 未知），锚点也会落在
    // 它应该挨着的那条附近，而不是一律跑到末尾。
    auto insertByCanonical = [&](const PromptEntity& p) {
        const int myIdx = canonicalIndex(p.identifier);
        int insertAt = 0;
        bool found = false;
        for (int i = 0; i < result.size(); ++i) {
            if (canonicalIndex(result[i].identifier) <= myIdx) {
                insertAt = i + 1;
                found = true;
            }
        }
        if (!found) insertAt = 0;
        result.insert(insertAt, p);
        seenIds.insert(p.identifier);
    };

    // systemPrompts：顺序表里已配同名提示词的只覆盖内容（角色卡字段以实体为准），
    // 没配过的按 canonical 位置补进去。
    for (const PromptEntity& sysP : systemPrompts) {
        if (seenIds.contains(sysP.identifier)) {
            for (PromptEntity& p : result) {
                if (p.identifier != sysP.identifier) continue;
                p.content = sysP.content;
                // role 优先用用户配置的，用户没配（System）才用系统默认
                if (p.role == GlobalConstants::MessageType::System)
                    p.role = sysP.role;
                p.system_prompt = p.system_prompt || sysP.system_prompt;
                p.marker = p.marker || sysP.marker;
                break;
            }
            continue;
        }
        insertByCanonical(sysP);
    }

    //补齐 mandatory
    const QStringList mandatory = {
        "main", "nsfw", "jailbreak", "chatHistory", "dialogueExamples"
    };
    for (const QString& id : mandatory) {
        if (seenIds.contains(id)) continue;
        PromptEntity p;
        p.identifier = id;
        p.system_prompt = true;
        if (id == "chatHistory" || id == "dialogueExamples") {
            p.marker = true;
        } else if (promptEntityMap.contains(id)) {
            const auto& pe = promptEntityMap[id];
            p.content = pe.content;
            p.role = pe.role;
        } else if (id == "main") {
            p.content = QStringLiteral(
                "Write {{char}}'s next reply in a fictional chat between {{char}} and {{user}}.");
        }
        // enabled 默认 true
        insertByCanonical(p);
    }


    //== 应用角色卡覆盖（main 和 jailbreak）==
    if (!character.systemPrompt.isEmpty()) {
        for (auto& p : result) {
            if (p.identifier == "main" && !p.forbid_overrides) {
                p.content = character.systemPrompt;
                break;
            }
        }
    }
    if (!character.postHistoryInstructions.isEmpty()) {
        for (auto& p : result) {
            if (p.identifier == "jailbreak" && !p.forbid_overrides) {
                p.content = character.postHistoryInstructions;
                break;
            }
        }
    }

    // 主提示词兜底：顺序表里有 main、但内容是空串（或被禁用）时，用 ST 的默认文案。
    // 否则模型收到的系统区里完全没有"你扮演谁、怎么回复"的指令。
    {
        bool hasMainContent = false;
        for (const PromptEntity& p : result) {
            if (p.identifier == QStringLiteral("main") && !p.content.trimmed().isEmpty()) {
                hasMainContent = true;
                break;
            }
        }
        if (!hasMainContent) {
            for (PromptEntity& p : result) {
                if (p.identifier != QStringLiteral("main")) continue;
                p.content = QStringLiteral(
                    "Write {{char}}'s next reply in a fictional chat between {{char}} and {{user}}.");
                p.isActive = true;
                break;
            }
        }
    }

    // 宏替换（{{char}} / {{user}} / {{description}} …）。
    // ST 对系统区、角色卡字段、世界书内容都做同样的替换；这里统一收口在返回前。
    const QVariantMap vars = getPromptVariables(character);
    for (PromptEntity& p : result) {
        if (!p.content.isEmpty())
            p.content = substituteParams(p.content, vars);
    }

    return result;
}

QList<ChatMessage> PromptBuilder::populateChatCompletion(
    const QList<PromptEntity>& prompts,ChatCompletion& chatCompletion,
    const CompletionPresetEntity & preset,
    const QList<ChatMessageEntity> & history,const QString & userInput,
    const CharacterEntity & character,
    const WorldInfoResult& wi,
    const PopulateContext& pCtx)
{
    Q_UNUSED(userInput)

    // 变量表：世界书深度注入里的 {{char}} 之类也要替换
    const QVariantMap vars = getPromptVariables(character);

    // ============================================================
    // 步骤 1：按 prompts 的顺序把内容填进同名组
    // ============================================================
    // 关键点：不再做任何下标运算。组顺序由 order 决定，内容按 identifier 落位。
    // 旧实现用 promptIndex 去 add()，顺序表下标和集合下标不一致时会覆盖或塌缩，
    // 导致 worldInfoBefore 被 main 顶掉、dialogueExamples 顶掉别人的提示词。
    QStringList order;
    QMap<QString, PromptEntity> promptMap;
    QList<PromptEntity> absolutePrompts;

    for (const PromptEntity& p : prompts) {
        if (p.identifier.isEmpty()) continue;
        if (!order.contains(p.identifier)) order.append(p.identifier);
        promptMap[p.identifier] = p;

        // absolute(绝对注入) 交给 populationInjectionPrompts 按深度插入历史
        if (p.injection_position == 1) {
            absolutePrompts.append(p);
            continue;
        }

        // 被禁用的提示词不发送（main 也在内：空内容时下面有默认文案兜底）
        if (!p.isActive) continue;

        const QString content = p.content.trimmed();
        if (content.isEmpty()) continue;

        SMessage msg;
        msg.identifier = p.identifier;
        msg.role = p.role;
        msg.content = content;
        msg.tokens = estimateTokens(content) + PER_MESSAGE_OVERHEAD;
        chatCompletion.ensureGroup(p.identifier);
        chatCompletion.insert(msg, p.identifier);
    }

    // ============================================================
    // 步骤 2：controlPrompts（ST 里排在最后）
    // ============================================================
    chatCompletion.ensureGroup(QStringLiteral("controlPrompts"));
    if (!order.contains(QStringLiteral("controlPrompts")))
        order.append(QStringLiteral("controlPrompts"));

    // 2.1 impersonate(模仿)
    if (pCtx.type == QStringLiteral("impersonate")
        && promptMap.contains(QStringLiteral("impersonate"))) {
        const PromptEntity& p = promptMap[QStringLiteral("impersonate")];
        const QString content = p.content.trimmed();
        if (!content.isEmpty()) {
            SMessage m;
            m.identifier = p.identifier;
            m.role = p.role;
            m.content = content;
            m.tokens = estimateTokens(content) + PER_MESSAGE_OVERHEAD;
            chatCompletion.insert(m, QStringLiteral("controlPrompts"));
        }
    }
    // 2.2 quietPrompt(静默提示，给扩展做后台生成用)
    if (!pCtx.quietPrompt.trimmed().isEmpty()) {
        SMessage m;
        m.identifier = QStringLiteral("quietPrompt");
        m.role = GlobalConstants::MessageType::System;
        m.content = pCtx.quietPrompt.trimmed();
        m.tokens = estimateTokens(m.content) + PER_MESSAGE_OVERHEAD;
        chatCompletion.insert(m, QStringLiteral("controlPrompts"));
    }
    // 2.3 bias(偏置，伪装成 assistant 消息)
    if (!pCtx.bias.trimmed().isEmpty()) {
        SMessage m;
        m.identifier = QStringLiteral("bias");
        m.role = GlobalConstants::MessageType::Assistant;
        m.content = pCtx.bias.trimmed();
        m.tokens = estimateTokens(m.content) + PER_MESSAGE_OVERHEAD;
        chatCompletion.insert(m, QStringLiteral("controlPrompts"));
    }

    // ============================================================
    // 步骤 3：历史消息
    // ============================================================
    QList<ChatMessage> historyAsMessages = convertHistoryToMessages(history);

    // ============================================================
    // 步骤 4：continue 相关（nudge / prefill）
    // ============================================================
    if (pCtx.type == QStringLiteral("continue")) {
        // continuePrefill：把最后一条 assistant 消息移出历史，作为 assistant 角色预填。
        // 旧实现直接 takeLast()，最后一条可能是用户消息，等于把用户发言伪装成 AI 的续写。
        if (preset.continuePrefill
            && !historyAsMessages.isEmpty()
            && historyAsMessages.last().role == GlobalConstants::MessageType::Assistant) {
            const ChatMessage lastMsg = historyAsMessages.takeLast();
            SMessage m;
            m.identifier = QStringLiteral("continuePrefill");
            m.role = lastMsg.role;
            m.content = lastMsg.textContent();
            m.tokens = estimateTokens(m.content) + PER_MESSAGE_OVERHEAD;
            chatCompletion.insert(m, QStringLiteral("controlPrompts"));
        } else if (!preset.continueNudgePrompt.trimmed().isEmpty()) {
            // 非 prefill 模式：用 continue nudge 提示模型接着写
            SMessage m;
            m.identifier = QStringLiteral("continueNudge");
            m.role = GlobalConstants::MessageType::System;
            m.content = preset.continueNudgePrompt.trimmed();
            m.tokens = estimateTokens(m.content) + PER_MESSAGE_OVERHEAD;
            chatCompletion.insert(m, QStringLiteral("controlPrompts"));
        }
    }

    // ============================================================
    // 步骤 5：in-chat 注入（绝对位置提示词 + 世界书 depth 条目）
    // ============================================================
    QList<ChatMessage> injectedMessages = populationInjectionPrompts(
        absolutePrompts, historyAsMessages, wi, vars);

    // ============================================================
    // 步骤 6：示例 + 历史（各自填自己的组，相对顺序由 prompts 决定）
    // ============================================================
    populateChatHistory(injectedMessages, chatCompletion, preset);
    populateDialogueExamples(pCtx.messageExamples, chatCompletion, preset, character);

    // 世界书里位置是"示例消息之前/之后"的条目：旧实现匹配出来后无人消费，直接丢了。
    // 这里把它们塞进示例区（EMTop 前插、EMBottom 后插）。
    for (const WorldInfoExample& example : wi.worldInfoExamples) {
        const QString content = substituteParams(example.content, vars).trimmed();
        if (content.isEmpty()) continue;

        SMessage m;
        m.identifier = QStringLiteral("worldInfoExample");
        m.role = GlobalConstants::MessageType::System;
        m.content = content;
        m.tokens = estimateTokens(content) + PER_MESSAGE_OVERHEAD;

        chatCompletion.ensureGroup(QStringLiteral("dialogueExamples"));
        if (!order.contains(QStringLiteral("dialogueExamples")))
            order.append(QStringLiteral("dialogueExamples"));
        chatCompletion.insert(
            m, QStringLiteral("dialogueExamples"),
            example.position == WorldInfoEntryEntity::EMTop ? ChatCompletion::Pos::Start
                                                            : ChatCompletion::Pos::End);
    }

    // ============================================================
    // 步骤 7：扁平化
    // ============================================================
    return chatCompletion.build(order);
}

// ============================================================
// 变量替换
// ============================================================
QString PromptBuilder::substituteParams(const QString& input, const QVariantMap& vars) const
{
    return Utils::replacePromptVariables(input, vars);
}

// ======================================
// Token 估算工具
// ======================================
int PromptBuilder::estimateMessagesToken(
    const QList<ChatMessageEntity>& messages,
    GlobalConstants::PromptTemplateType templateType)
{
    int totalToken = 0;
    int perMsgOverhead = 2;

    switch (templateType) {
    case GlobalConstants::PromptTemplateType::ChatML:
        perMsgOverhead = 4;
        break;
    case GlobalConstants::PromptTemplateType::Llama2:
    case GlobalConstants::PromptTemplateType::Mistral:
        perMsgOverhead = 5;
        break;
    case GlobalConstants::PromptTemplateType::Alpaca:
    case GlobalConstants::PromptTemplateType::GeminiPrompt:
        perMsgOverhead = 6;
        break;
    default:
        break;
    }

    for (const ChatMessageEntity& msg : messages) {
        if (msg.isHidden) continue;
        totalToken += Utils::calculateToken(msg.content, GlobalConstants::ModelType::OpenAI);
        totalToken += perMsgOverhead;
    }

    return totalToken;
}

// ======================================
// 模板与变量工具
// ======================================
QString PromptBuilder::getPromptTemplate(GlobalConstants::PromptTemplateType type)
{
    return m_templates.value(type);
}

QVariantMap PromptBuilder::getPromptVariables(const CharacterEntity& character)
{
    QVariantMap vars;
    vars.insert("char", character.name);
    vars.insert("character", character.name);
    vars.insert("description", character.description);
    vars.insert("personality", character.personality);
    vars.insert("scenario", character.scenario);
    vars.insert("first_mes", character.firstMessage);
    vars.insert("example_dialogue", character.exampleDialogue);

    vars.insert("user", "User");
    vars.insert("date", QDateTime::currentDateTime().toString("yyyy-MM-dd"));
    vars.insert("time", QDateTime::currentDateTime().toString("HH:mm:ss"));
    vars.insert("weekday", QDateTime::currentDateTime().toString("dddd"));

    // 酒馆特有变量可扩展
    return vars;
}

QString PromptBuilder::buildSystemPrompt(const CharacterEntity& character)
{
    QStringList sections;

    if (!character.description.isEmpty()) {
        sections.append(character.description);
    }
    if (!character.personality.isEmpty()) {
        sections.append(QString("Personality: %1").arg(character.personality));
    }
    if (!character.scenario.isEmpty()) {
        sections.append(QString("Scenario: %1").arg(character.scenario));
    }
    if (!character.creatorNotes.isEmpty()) {
        sections.append(QString("Creator Notes: %1").arg(character.creatorNotes));
    }
    if (!character.exampleDialogue.isEmpty()) {
        sections.append(QString("Example Dialogue:\n%1").arg(character.exampleDialogue));
    }
    if (!character.systemPrompt.isEmpty()) {
        sections.append(character.systemPrompt);
    }

    return sections.join("\n\n");
}


// ============================================================
// 根据 Token 截断上下文
// ============================================================
QList<ChatMessageEntity> PromptBuilder::truncateContext(
    const QList<ChatMessageEntity>& context,
    int systemPromptTokens,
    int userInputTokens,
    int maxTotalToken,
    GlobalConstants::PromptTemplateType templateType)
{
    Q_UNUSED(templateType);
    const int bufferTokens = 80;
    int available = maxTotalToken - systemPromptTokens - userInputTokens - bufferTokens;

    if (available <= 0) {
        return {};
    }

    QList<ChatMessageEntity> result;
    int currentUsage = 0;

    // 从最新消息开始往前（倒序）
    for (auto it = context.rbegin(); it != context.rend(); ++it) {
        const ChatMessageEntity& msg = *it;
        if (msg.isHidden) continue;

        int msgCost = Utils::calculateToken(msg.content, GlobalConstants::ModelType::OpenAI) + 4;

        if (currentUsage + msgCost > available) {
            break;
        }

        result.prepend(msg);
        currentUsage += msgCost;
    }

    return result;
}


void PromptBuilder::appendToCollection(
    QList<PromptEntity> &collection,
    const QString &content,
    const QString& identifier,
    GlobalConstants::MessageType role,
    const QString &name,
    bool skipIfEmpty)
{
    const QString trimmed = content.trimmed();
    if (skipIfEmpty && trimmed.isEmpty()) return;


    PromptEntity prompt;
    prompt.identifier = identifier;
    prompt.role = role;//默认为系统角色
    prompt.content = content;
    //其他多余内容补充
    prompt.system_prompt = true;
    prompt.marker = true;
    prompt.injection_position = 0; // 默认相对位置，但后续会被用户配置覆盖
    prompt.injection_depth = 4;
    prompt.injection_order = 100;
    prompt.forbid_overrides = false;
    collection.append(prompt);
}

//将历史记录转换为消息
QList<ChatMessage> PromptBuilder::convertHistoryToMessages(const QList<ChatMessageEntity> &history)
{
    QList<ChatMessage> result;
    for (const ChatMessageEntity & item : history) {
        if (item.status != GlobalConstants::MessageGenerationStatus::Normal) continue;
        if (item.isHidden) continue;
        if (item.content.trimmed().isEmpty()) continue;

        ChatMessage m;
        m.role = item.type;
        m.appendText(item.content.trimmed());
        m.senderName = item.senderName;
        result.append(m);
    }
    return result;
}

// ============================================================
// 将注入提示按深度插入到历史消息中
// ============================================================
QList<ChatMessage> PromptBuilder::populationInjectionPrompts(
    const QList<PromptEntity> &absolutePrompts,
    const QList<ChatMessage> &messages,
    const WorldInfoResult &wi,
    const QVariantMap &vars)
{
    // 收集所有需要 in-chat 注入的条目
    struct Injection {
        int depth;
        int order;
        GlobalConstants::MessageType role;
        QString content;
    };
    QList<Injection> injections;

    // 1. 预设的 absolute(绝对) 条目
    for (const PromptEntity & p : absolutePrompts) {
        if (!p.isActive) continue;
        if (p.content.trimmed().isEmpty()) continue;
        injections.append(
            {p.injection_depth, p.injection_order,
            p.role, substituteParams(p.content, vars).trimmed()});
    }

    // 2. 世界书 depth 条目（每项 (depth, role) 独立）
    for (const WorldInfoDepthGroup & g : wi.worldInfoDepth) {
        if (g.entries.isEmpty()) continue;
        QStringList parts;
        for (const QString& entry : g.entries)
            parts.append(substituteParams(entry, vars).trimmed());
        injections.append({g.depth, 100, g.insertRole, parts.join('\n')});
    }

    if (injections.isEmpty()) return messages;

    // 3. 按 depth 分组
    QMap<int, QList<Injection>> byDepth;
    for (const Injection& inj : injections) byDepth[inj.depth].append(inj);

    // 4. 从深到浅插入（避免索引漂移）
    QList<int> depths = byDepth.keys();
    std::sort(depths.begin(), depths.end(), std::greater<int>());

    QList<ChatMessage> result = messages;

    for (int depth : depths) {
        QList<Injection> & list = byDepth[depth];
        std::sort(list.begin(), list.end(),
            [](const Injection& a, const Injection& b) {
                return a.order < b.order;});

        // 同 depth 同 role 合并
        QMap<GlobalConstants::MessageType, QString> byRole;
        for (const Injection & inj : list) {
            if (!byRole[inj.role].isEmpty()) byRole[inj.role] += '\n';
            byRole[inj.role] += inj.content;
        }

        // 插入位置：正序下 size - depth
        int insertIdx = result.size() - depth;
        if (insertIdx < 0) insertIdx = 0;
        if (insertIdx > result.size()) insertIdx = result.size();

        // 按 role 顺序插入：ST 的规则是同 depth 同 role 的组按 User → Assistant → System
        // 自上而下排列（见 Prompt Manager 文档 "Order" 一节），旧实现是反的。
        const QList<GlobalConstants::MessageType> roles = {
            GlobalConstants::MessageType::User,
            GlobalConstants::MessageType::Assistant,
            GlobalConstants::MessageType::System,
        };

        int offset = 0;
        for (const GlobalConstants::MessageType & role : roles) {
            if (!byRole.contains(role)) continue;
            ChatMessage m;
            m.role = role;
            m.appendText(byRole[role]);
            result.insert(insertIdx + offset, m);
            offset++;
        }
    }

    return result;
}

//填充对话示例（组名 dialogueExamples，位置由 prompts 顺序决定）
void PromptBuilder::populateDialogueExamples(
    const QList<QList<ChatMessage>> &messageExamples,
    ChatCompletion &chatCompletion,
    const CompletionPresetEntity &preset, const CharacterEntity &character)
{
    Q_UNUSED(character)

    if (messageExamples.isEmpty()) return;

    chatCompletion.ensureGroup(QStringLiteral("dialogueExamples"));

    // ST：每个示例块前面单独插一条 New Example Chat 提示。
    // 旧实现把这条标记放在 block 循环内部，多块时会重复插入多份。
    const QString markerText = preset.newExampleChatPrompt.trimmed().isEmpty()
        ? QStringLiteral("[Example Chat]")
        : preset.newExampleChatPrompt.trimmed();

    for (int b = 0; b < messageExamples.size(); ++b) {
        QList<SMessage> block;

        SMessage marker;
        marker.identifier = QStringLiteral("newExampleChat");
        marker.role = GlobalConstants::MessageType::System;
        marker.content = markerText;
        marker.tokens = estimateTokens(markerText) + PER_MESSAGE_OVERHEAD;
        block.append(marker);

        for (int i = 0; i < messageExamples[b].size(); ++i) {
            const ChatMessage& cm = messageExamples[b][i];
            SMessage m;
            m.identifier = QStringLiteral("dialogueExamples-%1-%2").arg(b).arg(i);
            // 示例自身的角色要保留（user/assistant 交替），旧实现一律当 system
            m.role = cm.role;
            m.content = cm.textContent().trimmed();
            if (m.content.isEmpty()) continue;
            m.name = cm.senderName;
            m.tokens = estimateTokens(m.content) + PER_MESSAGE_OVERHEAD;
            block.append(m);
        }

        // 只有标记、没有实际内容就跳过
        if (block.size() <= 1) continue;
        if (!chatCompletion.canAffordAll(block)) break;

        for (const SMessage& m : block)
            chatCompletion.insert(m, QStringLiteral("dialogueExamples"));
    }
}

//填充聊天历史记录
void PromptBuilder::populateChatHistory(
    const QList<ChatMessage>& messages,
    ChatCompletion& chatCompletion,
    const CompletionPresetEntity& preset)
{
    chatCompletion.ensureGroup(QStringLiteral("chatHistory"));

    // New Chat 提示：ST 把它放在聊天记录的**开头**
    //（"sent before the chat history …where background information ends and chat history begins"）。
    // 旧实现用 "end" 插在最新一条之后，等于每轮都在告诉模型"这是新对话的开头"。
    const QString newChatPrompt =
        preset.newChatPrompt.trimmed().isEmpty()
            ? QStringLiteral("[Start a new Chat]")
            : preset.newChatPrompt.trimmed();
    SMessage newChatMsg;
    newChatMsg.identifier = QStringLiteral("newMainChat");
    newChatMsg.role = GlobalConstants::MessageType::System;
    newChatMsg.content = newChatPrompt;
    newChatMsg.tokens = estimateTokens(newChatMsg.content) + PER_MESSAGE_OVERHEAD;

    // 历史可用预算 = 其它组占用后剩下的，再扣掉 New Chat 标记本身
    int budget = chatCompletion.remaining() - newChatMsg.tokens;

    // 从新到旧扫描，能装多少装多少
    QList<SMessage> picked;
    for (int i = messages.size() - 1; i >= 0; --i) {
        const ChatMessage & cm = messages[i];
        if (cm.parts.isEmpty()) continue;

        SMessage m;
        m.identifier = QStringLiteral("chatHistory-%1").arg(i);
        m.role = cm.role;
        m.content = cm.textContent().trimmed();
        if (m.content.isEmpty()) continue;
        m.name = cm.senderName;
        m.tokens = estimateTokens(m.content) + PER_MESSAGE_OVERHEAD;

        if (m.tokens > budget) {
            // 最新一条就放不下时仍然保留：把它丢掉等于模型完全没有上下文，
            // 比略微超出预算更糟（旧实现直接 break，整段历史一起消失）。
            if (picked.isEmpty()) {
                picked.prepend(m);
                qWarning() << "[PromptBuilder] 最新一条消息自身超出预算，仍然保留:"
                           << m.tokens << "可用:" << budget;
            }
            break;
        }
        budget -= m.tokens;
        picked.prepend(m);
    }

    chatCompletion.insert(newChatMsg, QStringLiteral("chatHistory"), ChatCompletion::Pos::Start);
    for (const auto& m : picked)
        chatCompletion.insert(m, QStringLiteral("chatHistory"), ChatCompletion::Pos::End);
}

// 角色卡 example_dialogue → <START> 分隔的示例块
QList<QList<ChatMessage>> PromptBuilder::parseExampleDialogue(const CharacterEntity& character)
{
    QList<QList<ChatMessage>> blocks;

    const QString raw = character.exampleDialogue;
    if (raw.trimmed().isEmpty()) return blocks;

    const QVariantMap vars = getPromptVariables(character);

    // ST 的示例对话格式：<START> 分块，每行以 "{{user}}:" / "{{char}}:" 开头
    const QStringList userPrefixes = {
        QStringLiteral("{{user}}"),
        QStringLiteral("user"),
    };
    const QStringList charPrefixes = {
        QStringLiteral("{{char}}"),
        character.name,
    };

    auto matchPrefix = [](const QString& line, const QStringList& prefixes, QString& rest) {
        for (const QString& prefix : prefixes) {
            if (prefix.trimmed().isEmpty()) continue;
            if (line.startsWith(prefix + QLatin1Char(':'), Qt::CaseInsensitive)) {
                rest = line.mid(prefix.length() + 1).trimmed();
                return true;
            }
        }
        return false;
    };

    QList<ChatMessage> current;
    auto flushBlock = [&]() {
        if (!current.isEmpty()) blocks.append(current);
        current.clear();
    };

    const QStringList lines = raw.split(QLatin1Char('\n'));
    for (const QString& rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty()) continue;

        if (line.compare(QStringLiteral("<START>"), Qt::CaseInsensitive) == 0) {
            flushBlock();
            continue;
        }

        QString rest;
        if (matchPrefix(line, userPrefixes, rest)) {
            current.append(ChatMessage::makeText(GlobalConstants::MessageType::User,
                                                 substituteParams(rest, vars)));
            continue;
        }
        if (matchPrefix(line, charPrefixes, rest)) {
            current.append(ChatMessage::makeText(GlobalConstants::MessageType::Assistant,
                                                 substituteParams(rest, vars)));
            continue;
        }

        // 没有前缀：当成上一条的续行；块内第一条则按 narrator/system 处理
        if (current.isEmpty()) {
            current.append(ChatMessage::makeText(GlobalConstants::MessageType::System,
                                                 substituteParams(line, vars)));
        } else {
            ChatMessage& last = current.last();
            if (!last.parts.isEmpty())
                last.parts.last().text += QLatin1Char('\n') + substituteParams(line, vars);
        }
    }
    flushBlock();

    return blocks;
}

//估算tokens
int PromptBuilder::estimateTokens(const QString &text) const
{
    if (text.isEmpty()) return 0;
    return m_estimator(text);
}

// ============================================================
// 加载默认模板
// ============================================================
void PromptBuilder::loadDefaultTemplates()
{
    m_templates.insert(GlobalConstants::PromptTemplateType::ChatML, "");

    m_templates.insert(GlobalConstants::PromptTemplateType::Alpaca,
       "### Instruction:\n"
       "{{system_prompt}}\n\n"
       "### Conversation History:\n"
       "{{history}}\n\n"
       "### Input:\n"
       "{{user_input}}\n\n"
       "### Response:");

    m_templates.insert(GlobalConstants::PromptTemplateType::Llama2,
       "<s>[INST] <<SYS>>\n"
       "{{system_prompt}}\n"
       "<</SYS>>\n\n"
       "{{history}}\n"
       "{{user_input}} [/INST]");

    m_templates.insert(GlobalConstants::PromptTemplateType::Mistral,
       "<s>[INST] {{system_prompt}}\n\n"
       "{{history}}\n"
       "{{user_input}} [/INST]");

    m_templates.insert(GlobalConstants::PromptTemplateType::GeminiPrompt,
       "{{system_prompt}}\n\n"
       "{{history}}\n"
       "User: {{user_input}}\n"
       "Model: ");

    m_templates.insert(GlobalConstants::PromptTemplateType::CustomPrompt, "");
}

// ============================================================
// 加载默认提示词库与顺序（酒馆风格）
// ============================================================
void PromptBuilder::loadDefaultPromptsAndOrder()
{
    // // 1. 定义默认提示词库
    // m_prompts = {
    //     { "main", "Main Prompt", "system",
    //       "Write {{char}}'s next reply in a fictional chat between {{charIfNotGroup}} and {{user}}.",
    //       true, false, 0, 4, 100, false, {} },
    //     { "nsfw", "Auxiliary Prompt", "system", "", true, false, 0, 4, 100, false, {} },
    //     { "jailbreak", "Post-History Instructions", "system", "", true, false, 0, 4, 100, false, {} },
    //     { "enhanceDefinitions", "Enhance Definitions", "system",
    //       "If you have more knowledge of {{char}}, add to the character's lore and personality to enhance them but keep the Character Sheet's definitions absolute.",
    //       true, false, 0, 4, 100, false, {} },
    //     // 标记型提示词（内容由系统填充）
    //     { "charDescription", "Char Description", "system", "", true, true, 0, 4, 100, false, {} },
    //     { "charPersonality", "Char Personality", "system", "", true, true, 0, 4, 100, false, {} },
    //     { "scenario", "Scenario", "system", "", true, true, 0, 4, 100, false, {} },
    //     { "personaDescription", "Persona Description", "system", "", true, true, 0, 4, 100, false, {} },
    //     { "worldInfoBefore", "World Info (before)", "system", "", true, true, 0, 4, 100, false, {} },
    //     { "worldInfoAfter", "World Info (after)", "system", "", true, true, 0, 4, 100, false, {} },
    //     { "dialogueExamples", "Chat Examples", "system", "", true, true, 0, 4, 100, false, {} },
    //     { "chatHistory", "Chat History", "system", "", true, true, 0, 4, 100, false, {} }
    // };

    // // 建立快速查找表
    // m_promptLookup.clear();
    // for (const auto& p : m_prompts) {
    //     m_promptLookup.insert(p.identifier, p);
    // }

    // // 2. 定义默认顺序（与酒馆默认一致）
    // m_promptOrder = {
    //     { "main", true },
    //     { "worldInfoBefore", true },
    //     { "personaDescription", true },
    //     { "charDescription", true },
    //     { "charPersonality", true },
    //     { "scenario", true },
    //     { "enhanceDefinitions", false },
    //     { "nsfw", true },
    //     { "worldInfoAfter", true },
    //     { "dialogueExamples", true },
    //     { "chatHistory", true },
    //     { "jailbreak", true }
    // };
}

QList<ChatMessage> PromptBuilder::buildChatMessages(
    const CharacterEntity& character,
    const QList<ChatMessageEntity>& history,
    const QString& userInput,
    GlobalConstants::PromptTemplateType templateType,
    int maxContextToken,
    const QString& customTemplate,
    const QString& worldInfoSection,
    const QString& memorySection,
    const QString& generationType,//生成类型："normal", "continue", "impersonate", "quiet"
    const QString& systemPromptOverride,
    const QString& jailbreakPromptOverride,
    const QList<ExtensionPrompt>& extensionPrompts,//扩展提示列表（来自插件）
    const QString& cyclePrompt,
    const QList<ChatMessageEntity>& messageExamples
    )
{
    Q_UNUSED(templateType);
    Q_UNUSED(customTemplate);
    return QList<ChatMessage>{};

    // 1. 设置当前生成类型（用于过滤 injection_trigger）(废弃)
    // m_currentGenerationType = generationType.isEmpty() ? "normal" : generationType;

    // 2. 提取角色信息
    // QString charDesc = character.description;
    // QString charPers = character.personality;
    // QString scenarioText = character.scenario;
    // QString personaDesc = ""; // 可扩展

    // // 3. 准备提示集合（合并系统提示与用户配置，应用覆盖）
    // QList<Prompt> promptList = preparePromptCollection(
    //     character,
    //     worldInfoSection,     // worldInfoBefore
    //     "",                   // worldInfoAfter (暂不使用)
    //     charDesc,
    //     charPers,
    //     scenarioText,
    //     personaDesc,
    //     extensionPrompts,
    //     systemPromptOverride,
    //     jailbreakPromptOverride,
    //     generationType
    //     );

    // // 4. 获取变量映射
    // QVariantMap vars = getPromptVariables(character);

    // // 5. 分离系统提示（injection_position == 0）和注入提示（injection_position == 1）
    // QList<ChatMessage> systemMessages;
    // QList<Prompt> injectionPrompts;
    // for (const Prompt& p : promptList) {
    //     if (p.content.isEmpty()) continue;
    //     if (p.injection_position == 1) {
    //         injectionPrompts.append(p);
    //     } else {
    //         QString content = substituteParams(p.content, vars);
    //         systemMessages.append(ChatMessage::makeText(mapRole(p.role), content));
    //     }
    // }

    // // 6. 计算系统提示和注入提示的总 Token（用于截断历史时预留）
    // int systemTokens = 0;
    // for (const auto& msg : systemMessages) {
    //     systemTokens += Utils::calculateToken(msg.textContent(), GlobalConstants::ModelType::OpenAI) + 4;
    // }
    // // 注入提示的 Token 也要计入，因为它们会插入到历史中
    // for (const auto& p : injectionPrompts) {
    //     QString content = substituteParams(p.content, vars);
    //     systemTokens += Utils::calculateToken(content, GlobalConstants::ModelType::OpenAI) + 4;
    // }

    // // 7. 处理用户输入（变量替换）
    // QString processedInput = substituteParams(userInput, vars);
    // int inputTokens = Utils::calculateToken(processedInput, GlobalConstants::ModelType::OpenAI) + 4;

    // // 8. 截断原始历史消息（只截断 context，不考虑注入提示，因为注入提示会单独插入）
    // QList<ChatMessageEntity> trimmedContext = truncateContext(
    //     context, systemTokens, inputTokens, maxContextToken, templateType);

    // // 9. 构建历史消息列表（从截断后的 context）
    // QList<ChatMessage> historyMessages;
    // for (const auto& entity : trimmedContext) {
    //     if (entity.isHidden) continue;
    //     historyMessages.append(entityToMessage(entity));
    // }

    // // 10. 将注入提示按深度插入到历史消息中
    // applyInjectionPrompts(historyMessages, injectionPrompts, vars);

    // // 11. 插入示例对话（如果有），放在历史消息最前面（酒馆行为）
    // if (!messageExamples.isEmpty()) {
    //     QList<ChatMessage> exampleMessages;
    //     for (const auto& entity : messageExamples) {
    //         if (entity.isHidden) continue;
    //         exampleMessages.append(entityToMessage(entity));
    //     }
    //     // 可以在示例前加一个标记（如 "Example Chat"），但这里省略
    //     historyMessages = exampleMessages + historyMessages;
    // }

    // // 12. 处理继续生成（type == "continue"）
    // if (m_currentGenerationType == "continue" && !cyclePrompt.isEmpty()) {
    //     // 酒馆中 continue 会将最后一条 assistant 消息的内容替换为原始内容 + 续写提示
    //     // 但这里我们简单处理：如果最后一条是 assistant，则追加一条 assistant 消息（内容为 cyclePrompt）
    //     // 更准确的做法是找到最后一条 assistant，修改其内容并添加后缀
    //     // 我们实现一个简化版：追加 assistant 消息
    //     historyMessages.append(ChatMessage::makeText(ChatMessage::Role::Assistant, cyclePrompt));
    // }

    // // 13. 组装最终消息：系统消息 + 历史消息（含注入和示例） + 用户输入
    // QList<ChatMessage> finalMessages;
    // finalMessages.append(systemMessages);
    // finalMessages.append(historyMessages);
    // finalMessages.append(ChatMessage::makeText(ChatMessage::Role::User, processedInput));

    // return finalMessages;

}

