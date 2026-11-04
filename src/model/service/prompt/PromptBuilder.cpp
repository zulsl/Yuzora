#include "PromptBuilder.h"
#include "Utils.h"
#include <QDateTime>
#include <QDebug>
#include "CompletionPresetService.h"
#include "GlobalConstants.h"

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
    const QString & personaDesc)
{
    //准备提示词
    QList<PromptEntity> prompts=preparePromptCollection(preset,wi,character);

    ChatCompletion cc(preset.maxContextToken, preset.maxResponseToken);

    //填充聊天消息
    PopulateContext pCtx;
    populateChatCompletion(prompts,cc,preset,history,userInput,character,wi,pCtx);

    QList<ChatMessage> chatMessages= cc.getChat();

    return chatMessages;
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
    const WorldInfoResult & wi,const CharacterEntity & character
    )
{
    QList<PromptEntity> uPrompts=CompletionPresetService::instance().getPromptsByPresetId(preset.id);

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

    // 按 promptOrder 遍历，构建 result
    QList<PromptEntity> result;

    QSet<QString> seenIds;
    QStringList promptOrder=preset.promptOrder;
    // 如果 order 为空，用 prompts 的原始顺序作为 后备方案
    if (promptOrder.isEmpty()) {
        for (const PromptEntity & pe : uPrompts) promptOrder.append(pe.identifier);
    }

    //按顺序取出Prompt
    for (const QString& po : promptOrder) {
        if (seenIds.contains(po)) continue;
        PromptEntity prompt;
        if (promptEntityMap.contains(po)){
            prompt=promptEntityMap.find(po).value();
        }
        // 如果 promptEntityMap 里没有（可能是系统项），
        // 留给下一步 的 systemPrompts 补齐

        // 用 systemPrompts 覆盖 content（★ 核心合并） ---
        for (const PromptEntity& sysP : systemPrompts) {
            if (sysP.identifier == prompt.identifier) {
                prompt.content = sysP.content;
                // role 优先用 PromptEntity 的（用户可配置），
                // 若 PromptEntity 没有 role（空），才用系统默认
                if (prompt.role == GlobalConstants::MessageType::System) {
                    prompt.role = sysP.role;
                }
                prompt.system_prompt = prompt.system_prompt || sysP.system_prompt;
                // prompt.extension = prompt.extension || sysP.extension;
                break;
            }
        }
        seenIds.insert(po);
        result.append(prompt);
    }


    //补齐 systemPrompts 中缺失的
    for (const PromptEntity& sysP : systemPrompts) {
        if (seenIds.contains(sysP.identifier)) continue;
        if (sysP.content.trimmed().isEmpty()) {
            // 空内容的系统项依然加入（作为锚点）
        }
        result.append(sysP);
        seenIds.insert(sysP.identifier);
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
            p.content = "Write {{char}}'s next reply...";
        }
        // enabled 默认 true
        result.append(p);
        seenIds.insert(id);
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


    return result;
}

void PromptBuilder::populateChatCompletion(
    const QList<PromptEntity>& prompts,ChatCompletion& chatCompletion,
    const CompletionPresetEntity & preset,
    const QList<ChatMessageEntity> & history,const QString & userInput,
    const CharacterEntity & character,
    const WorldInfoResult& wi,
    const PopulateContext& pCtx)
{
    // ============================================================
    // 步骤 0：索引 PromptCollection（identifier -> Prompt + index）
    // ============================================================
    QMap<QString, PromptEntity> promptMap;//快速查找
    QMap<QString, int> promptIndex;
    for (int i = 0; i < prompts.size(); ++i) {
        promptMap[prompts[i].identifier] = prompts[i];
        promptIndex[prompts[i].identifier] = i;
    }

    // ============================================================
    // 步骤 1：addToChatCompletion - 单个 prompt 按 collection 位置添加
    // ============================================================
    auto addGroupToChatCompletion = [&](const QString& source, const QString& target = QString()) {
        if (!promptMap.contains(source)) return;

        const PromptEntity& p = promptMap[source];

        // disabled 跳过（main 例外：main 保留空壳作为锚点）
        if (!p.isActive && source != QStringLiteral("main")) return;

        // absolute(绝对注入) 单独处理（跳过这里）
        if (p.injection_position == 1) return;

        //当前来源在列表中的索引
        const int idx = target.isEmpty() ? promptIndex[source] : promptIndex[target];
        if (idx < 0) return;


        SMessage msg;
        msg.identifier = source;
        msg.role = p.role;
        msg.content = p.content.trimmed();
        if (msg.content.isEmpty()) return;
        msg.tokens = estimateTokens(msg.content) + PER_MESSAGE_OVERHEAD;

        ChatMessageGroup cmg(msg.identifier);
        cmg.add(msg);
        chatCompletion.add(cmg, idx);
    };

    // ============================================================
    // 步骤 2：硬编码系统项（ST 顺序）
    // ============================================================
    chatCompletion.reserveBudget(3);   // BOS

    addGroupToChatCompletion(QStringLiteral("worldInfoBefore"));
    addGroupToChatCompletion(QStringLiteral("main"));
    addGroupToChatCompletion(QStringLiteral("worldInfoAfter"));
    addGroupToChatCompletion(QStringLiteral("charDescription"));
    addGroupToChatCompletion(QStringLiteral("charPersonality"));
    addGroupToChatCompletion(QStringLiteral("scenario"));
    addGroupToChatCompletion(QStringLiteral("personaDescription"));


    // ============================================================
    // 步骤 3：controlPrompts (控制提示词)（最后统一插入）
    // ============================================================

    //覆盖提示
    // chatCompletion.setOverriddenPrompts({});   // TODO: 传入 promptManager 覆盖列表

    ChatMessageGroup controlPrompts(QStringLiteral("controlPrompts"));
    // 1. impersonate(模仿) 消息
    if (pCtx.type == QStringLiteral("impersonate")
        &&promptMap.contains(QStringLiteral("impersonate"))) {
        const PromptEntity& p = promptMap[QStringLiteral("impersonate")];
        if (!p.content.trimmed().isEmpty()) {
            SMessage m;
            m.identifier = p.identifier;
            m.role = p.role;
            m.content = p.content.trimmed();
            m.tokens = estimateTokens(m.content) + PER_MESSAGE_OVERHEAD;
            controlPrompts.add(m);
        }
    }
    // 2. quietPrompt 消息 (静默提示)
    if (!pCtx.quietPrompt.trimmed().isEmpty()
        && promptMap.contains(QStringLiteral("quietPrompt"))) {
        const PromptEntity& p = promptMap[QStringLiteral("quietPrompt")];
        SMessage m;
        m.identifier = p.identifier;
        m.role = p.role;
        m.content = pCtx.quietPrompt.trimmed();
        // quietImage 可选处理：m.images.append(ctx.quietImage);
        m.tokens = estimateTokens(m.content) + PER_MESSAGE_OVERHEAD;
        controlPrompts.add(m);
    }
    //预扣
    chatCompletion.reserveBudget(controlPrompts.getTokens());

    // ============================================================
    // 步骤 4：system 相对提示 + user 相对提示
    // ============================================================
    const QStringList systemPrompts = {
        QStringLiteral("nsfw"),
        QStringLiteral("jailbreak"),
    };
    QList<QString> userRelativePrompts;//用户相对
    QList<PromptEntity> absolutePrompts;//绝对位置提示词表
    for (const PromptEntity& p : prompts) {
        if (p.system_prompt) continue;
        if (p.injection_position == 1) {
            absolutePrompts.append(p);
            continue;
        }
        userRelativePrompts.append(p.identifier);
    }
    for (const auto& id : systemPrompts) addGroupToChatCompletion(id);
    for (const auto& id : userRelativePrompts) addGroupToChatCompletion(id);

    // ============================================================
    // 步骤 5：enhanceDefinitions(增强定义)
    // ============================================================
    if (promptMap.contains(QStringLiteral("enhanceDefinitions"))) {
        addGroupToChatCompletion(QStringLiteral("enhanceDefinitions"));
    }
    // ============================================================
    // 步骤 6：bias(偏置)（伪装成 assistant 消息）
    // ============================================================
    if (!pCtx.bias.trimmed().isEmpty()
        && promptMap.contains(QStringLiteral("bias"))) {
        const auto& p = promptMap[QStringLiteral("bias")];
        SMessage m;
        m.identifier = p.identifier;
        m.role = GlobalConstants::MessageType::Assistant;
        m.content = pCtx.bias.trimmed();
        m.tokens = estimateTokens(m.content) + PER_MESSAGE_OVERHEAD;
        ChatMessageGroup coll(QStringLiteral("bias"));
        coll.add(m);
        chatCompletion.add(coll, promptIndex[QStringLiteral("bias")]);
    }

    // ============================================================
    // 步骤 7：injectToMain - 相对注入（summary/AN/vectors）
    // ============================================================
    auto injectToMain = [&](const PromptEntity& p, const QString& position) {
        if (chatCompletion.has(QStringLiteral("main"))) {
            SMessage m;
            m.identifier = p.identifier;
            m.role = p.role;
            m.content = p.content.trimmed();
            if (m.content.isEmpty()) return;
            m.tokens = estimateTokens(m.content) + PER_MESSAGE_OVERHEAD;
            chatCompletion.insert(m, QStringLiteral("main"), position);
        } else {
            // main 不存在：转成 absolute 相对注入（省略，MVP 场景 main 通常存在）
            qWarning() << "[PromptBuilder] main missing, dropping injection:" << p.identifier;
        }
    };

    const QStringList knownPrompts = {//已知提示
        QStringLiteral("summary"),
        QStringLiteral("authorsNote"),
        QStringLiteral("vectorsMemory"),
        QStringLiteral("vectorsDataBank"),
        QStringLiteral("smartContext"),
    };
    for (const QString& key : knownPrompts) {
        if (promptMap.contains(key) && !promptMap[key].injection_position) {
            injectToMain(promptMap[key], promptMap[key].extraPosition);
        }
    }
    // 其他 extension 位置提示
    for (const PromptEntity& p : prompts) {
        if (p.extension && !p.extraPosition.isEmpty()) {
            injectToMain(p, p.extraPosition);
        }
    }

    // ============================================================
    // 步骤 8：tool data 预留（可选）
    // ============================================================
    // 若启用 tool calling，这里预留 token：
    // if (ToolManager::canPerformToolCalls(ctx.type)) { ... }


    //历史消息
    QList<ChatMessage> historyAsMessages = convertHistoryToMessages(history);

    // ============================================================
    // 步骤 9：continue prefill(继续预填充)（可选）
    // ============================================================
    if (pCtx.type == QStringLiteral("continue")
        && preset.continuePrefill
        && !historyAsMessages.isEmpty()) {
        // 把最后一条移出历史，作为 prefilled assistant message
        const ChatMessage lastMsg = historyAsMessages.takeLast();
        SMessage m;
        m.identifier = QStringLiteral("continuePrefill");
        m.role = lastMsg.role;
        m.content = lastMsg.testContent;
        m.tokens = estimateTokens(m.content) + PER_MESSAGE_OVERHEAD;
        controlPrompts.add(m);
        chatCompletion.reserveBudget(m.tokens);
    }

    // ============================================================
    // 步骤 10：in-chat 注入（★ 核心）
    // ============================================================
    QList<ChatMessage> injectedMessages = populationInjectionPrompts(
        absolutePrompts, historyAsMessages, wi);

    // ============================================================
    // 步骤 11：示例 + 历史（顺序取决于 pin_examples）
    // ============================================================
    // if (power_user.pinExamples) {
    //     populateDialogueExamples(pCtx.messageExamples, prompts, chatCompletion,preset, character);
    //     populateChatHistory(injectedMessages, prompts, chatCompletion, preset);
    // } else {
        //填充历史消息
        populateChatHistory(injectedMessages, prompts, chatCompletion, preset);
        populateDialogueExamples(pCtx.messageExamples, prompts, chatCompletion,preset, character);
    // }

    // ============================================================
    // 步骤 12：controlPrompts 最后添加
    // ============================================================
    chatCompletion.freeBudget(controlPrompts.getTokens());
    if (!controlPrompts.isEmpty()) {
        chatCompletion.add(controlPrompts);
    }

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
    prompt.role = role;
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
        m.testContent = item.content.trimmed();
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
    const WorldInfoResult &wi)
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
            p.role, p.content.trimmed()});
    }

    // 2. 世界书 depth 条目（每项 (depth, role) 独立）
    for (const WorldInfoDepthGroup & g : wi.worldInfoDepth) {
        if (g.entries.isEmpty()) continue;
        injections.append({g.depth, 100, g.insertRole, g.entries.join('\n')});
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

        // 按 role 顺序插入：system → user → assistant
        const QList<GlobalConstants::MessageType> roles = {
            GlobalConstants::MessageType::System,
            GlobalConstants::MessageType::User,
            GlobalConstants::MessageType::Assistant,
        };

        int offset = 0;
        for (const GlobalConstants::MessageType & role : roles) {
            if (!byRole.contains(role)) continue;
            ChatMessage m;
            m.role = role;
            m.testContent = byRole[role];
            result.insert(insertIdx + offset, m);
            offset++;
        }
    }

    return result;
}

//填充对话示例
void PromptBuilder::populateDialogueExamples(const QList<QList<ChatMessage>> &messageExamples, const QList<PromptEntity> &prompts, ChatCompletion &chatCompletion, const CompletionPresetEntity &preset, const CharacterEntity &character)
{
    int exIdx = -1;
    for (int i = 0; i < prompts.size(); ++i) {
        if (prompts[i].identifier == QStringLiteral("dialogueExamples")) {
            exIdx = i;
            break;
        }
    }
    if (exIdx < 0) return;

    chatCompletion.add(ChatMessageGroup(QStringLiteral("dialogueExamples")), exIdx);

    // 示例消息可能有 <START> 分隔符
    if (messageExamples.isEmpty()) return;

    SMessage newExampleChat;
    newExampleChat.identifier = QStringLiteral("newChat");
    newExampleChat.role = GlobalConstants::MessageType::System;
    newExampleChat.content = preset.newExampleChatPrompt.isEmpty()
        ? QStringLiteral("[Example Chat]")
        : preset.newExampleChatPrompt;
    newExampleChat.tokens = estimateTokens(newExampleChat.content) + PER_MESSAGE_OVERHEAD;

    for (const QList<ChatMessage>& dialogue : messageExamples) {
        QList<SMessage> block;
        for (int i = 0; i < dialogue.size(); ++i) {
            SMessage m;
            m.identifier = QStringLiteral("dialogueExamples-%1-%2").arg(&dialogue - &messageExamples[0]).arg(i);
            m.role = GlobalConstants::MessageType::System;
            m.content = dialogue[i].testContent;
            m.name = dialogue[i].senderName;
            m.tokens = estimateTokens(m.content) + PER_MESSAGE_OVERHEAD;
            block.append(m);
        }

        // 检查预算
        QList<SMessage> all = block;
        all.prepend(newExampleChat);
        if (!chatCompletion.canAffordAll(all)) break;

        chatCompletion.insert(newExampleChat, QStringLiteral("dialogueExamples"));
        for (const auto& m : block) {
            chatCompletion.insert(m, QStringLiteral("dialogueExamples"));
        }
    }
}

//填充聊天历史记录
void PromptBuilder::populateChatHistory(
    const QList<ChatMessage>& messages, const QList<PromptEntity>& prompts,
    ChatCompletion& chatCompletion,const CompletionPresetEntity& preset)
{
    int chatHistoryIdx = -1;
    for (int i = 0; i < prompts.size(); ++i) {
        if (prompts[i].identifier == QStringLiteral("chatHistory")) {
            chatHistoryIdx = i;
            break;
        }
    }
    if (chatHistoryIdx < 0) return;

    // 添加空的 chatHistory collection 占位
    chatCompletion.add(ChatMessageGroup(QStringLiteral("chatHistory")), chatHistoryIdx);

    // 预留 new_chat_prompt
    const QString newChatPrompt =
        preset.newChatPrompt.isEmpty()
            ? QStringLiteral("[Start a new Chat]")
            : preset.newChatPrompt;
    SMessage newChatMsg;
    newChatMsg.identifier = QStringLiteral("newMainChat");
    newChatMsg.role = GlobalConstants::MessageType::System;
    newChatMsg.content = newChatPrompt;
    newChatMsg.tokens = estimateTokens(newChatMsg.content) + PER_MESSAGE_OVERHEAD;
    chatCompletion.reserveBudget(newChatMsg.tokens);

    // 从新到旧扫描
    QList<SMessage> picked;
    int tempBudget = chatCompletion.remaining();
    for (int i = messages.size() - 1; i >= 0; --i) {
        const ChatMessage & cm = messages[i];
        if (cm.testContent.isEmpty()) continue;

        SMessage m;
        m.identifier = QStringLiteral("chatHistory-%1").arg(i);
        m.role = cm.role;
        m.content = cm.testContent;
        m.name = cm.senderName;
        m.tokens = estimateTokens(m.content) + PER_MESSAGE_OVERHEAD;

        if (tempBudget - m.tokens < 0) break;
        tempBudget -= m.tokens;
        picked.prepend(m);
    }

    // 按顺序插入（insert 内部扣预算）
    for (const auto& m : picked) {
        chatCompletion.insert(m, QStringLiteral("chatHistory"), QStringLiteral("end"));
    }

    // 释放 new_chat 预留并添加
    chatCompletion.freeBudget(newChatMsg.tokens);
    chatCompletion.insert(newChatMsg, QStringLiteral("chatHistory"), QStringLiteral("end"));
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

