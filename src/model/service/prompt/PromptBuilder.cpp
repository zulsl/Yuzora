#include "PromptBuilder.h"
#include "Utils.h"
#include <QDateTime>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <limits>
#include "GlobalConstants.h"
#include "AppException.h"

namespace {

// ST openai.js:789-801 formatWorldInfo(value, {wiFormat})：
//   - value 为空 → ''
//   - format 去空白后为空 → value 原样（默认 '{0}' 等价于不包装）
//   - 否则把所有 {0} 替换成 value；format 里没有 {0} 时就只剩 format 本身，
//     ST 的 stringFormat 也是这个行为，不要自作聪明改成"没占位符就不包"。
QString formatWorldInfo(const QString& value, const QString& format)
{
    if (value.isEmpty()) return {};
    if (format.trimmed().isEmpty()) return value;

    QString out = format;
    out.replace(QStringLiteral("{0}"), value);
    return out;
}

} // namespace

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
    const QString & generationType,
    const QString & userName)
{
    // Persona 还没有落到设置里，变量表先按 "User" 走；接入 persona 后改 getPromptVariables
    Q_UNUSED(personaDesc)

    const QString type = generationType.trimmed().isEmpty()
        ? QStringLiteral("normal") : generationType.trimmed();

    // 变量表：角色字段 + 用户 + 聊天级宏（lastUserMessage …）。
    // 只算一次，下面所有步骤共用同一份，避免各函数各自 getPromptVariables 得到不一致的变量表。
    QVariantMap vars = getPromptVariables(character, userName);
    const QVariantMap chatMacros = getChatMacros(history, userInput);
    for (auto it = chatMacros.cbegin(); it != chatMacros.cend(); ++it)
        vars.insert(it.key(), it.value());

    //准备提示词（generationType 参与 injection_trigger 过滤）
    QList<PromptEntity> prompts = preparePromptCollection(
        preset, wi, character, allPrompts, type, vars);

    ChatCompletion cc(preset.maxContextToken, preset.maxResponseToken);

    //打包填充数据
    PopulateContext pCtx;
    pCtx.wi = &wi;
    pCtx.userInput = userInput;//已添加至聊天历史
    pCtx.type = type;
    pCtx.userName = userName;//可扩展为用户实体
    // 示例区：角色卡示例 + 世界书 EMTop/EMBottom（ST 把 EM 条目并进示例文本再一起解析）
    pCtx.messageExamples = parseExampleDialogue(character, wi, vars);

    QStringList messageIdentifiers;
    QList<ChatMessage> built = populateChatCompletion(
        prompts, cc, preset, history, userInput, character, wi, pCtx, vars,
        &messageIdentifiers);

    writePromptTrace(preset, allPrompts, type, built, messageIdentifiers);
    return built;
}


QList<PromptEntity> PromptBuilder::preparePromptCollection(
    const CompletionPresetEntity & preset,
    const WorldInfoResult & wi,const CharacterEntity & character,
    const QList<PromptEntity> & uPrompts,
    const QString & generationType,const QVariantMap & vars)
{
    QList<PromptEntity> systemPrompts;//临时系统提示词集合

    // ============================================================
    // 构造 systemPrompts 数组
    // ============================================================
    // ST openai.js:1420-1421：worldInfoBefore/After 会先过 formatWorldInfo(wi_format)。
    // 本预设的 wi_format 是 "<World_Lore_Database>\n{0}\n</World_Lore_Database>"，
    // 且 order 里 before/after 紧挨着 → ST 请求里就是两个 <World_Lore_Database> 块。
    appendToCollection(systemPrompts,formatWorldInfo(wi.worldInfoBefore,preset.wiFormat),"worldInfoBefore");
    appendToCollection(systemPrompts,formatWorldInfo(wi.worldInfoAfter,preset.wiFormat),"worldInfoAfter");
    appendToCollection(systemPrompts,character.description,"charDescription");
    // ST openai.js:1410-1411：scenario/personality 有格式模板时，正文用模板
    //（模板里的 {{scenario}} / {{personality}} 由下面的统一宏替换解析）。
    const QString charPersonalityText =
        !character.personality.isEmpty() && !preset.personalityFormat.trimmed().isEmpty()
            ? preset.personalityFormat : character.personality;
    const QString scenarioText =
        !character.scenario.isEmpty() && !preset.scenarioFormat.trimmed().isEmpty()
            ? preset.scenarioFormat : character.scenario;
    appendToCollection(systemPrompts,charPersonalityText,"charPersonality");//个性
    appendToCollection(systemPrompts,scenarioText,"scenario");//场景

    // ST openai.js:1417-1431：这几个锚点的正文来自 oai_settings，不是提示词表里的 content
    appendToCollection(systemPrompts,preset.impersonationPrompt,"impersonate");//(模仿)
    appendToCollection(systemPrompts,"","quietPrompt");//(静默提示)
    appendToCollection(systemPrompts,preset.groupNudgePrompt,"groupNudge");//群聊 nudge
    appendToCollection(systemPrompts,"","bias",GlobalConstants::MessageType::Assistant);//偏置

    // 两个运行时占位锚点：正文由 populateChatHistory / populateDialogueExamples 填。
    // 这里等价地在装配时按 canonical 位置补上——否则这两个组不在 order 里，
    // build() 只能按"创建顺序"把它们追加到末尾（历史和示例会跑到 controlPrompts 后面）。
    appendToCollection(systemPrompts,"","dialogueExamples",
                       GlobalConstants::MessageType::System,QString(),false);
    appendToCollection(systemPrompts,"","chatHistory",
                       GlobalConstants::MessageType::System,QString(),false);


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

    QList<PromptEntity> result;//排序完毕的提示词列表
    QSet<QString> seenIds;//已处理的条目

    // 按顺序表取出用户配置的提示词。
    // 顺序表里出现、但预设里没有对应提示词的项（系统锚点/未知项）先跳过，
    // 交给下面的 systemPrompts / mandatory 补齐——否则会留下 identifier 为空的
    // 空壳，并把 seenIds 占掉，让真正的锚点永远补不进来。
    QStringList promptOrder = preset.promptOrder;
    // ST：prompt_order 为空时由 sanitizeServiceSettings 补默认顺序（PromptManager.js），
    // 这里退化成"提示词表原始顺序"，不再抛异常把整条发送链路打断。
    if (promptOrder.isEmpty()) {
        for (const PromptEntity & pe : uPrompts) promptOrder.append(pe.identifier);
    }
    //按顺序取出Prompt
    for (const QString& po : promptOrder) {
        if (seenIds.contains(po)) continue;
        if (!promptEntityMap.contains(po)) continue;
        const PromptEntity& pe = promptEntityMap.value(po);
        // ST getPromptCollection：injection_trigger 非空时，只在列出的生成类型下生效
        if (!pe.injection_trigger.isEmpty() && !pe.injection_trigger.contains(generationType)) {
            continue;
        }
        result.append(pe);
        seenIds.insert(po);
    }

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
        insertByCanonical(result,seenIds,sysP);
    }

    //补齐 强制提示词,强制注入预设提示词条目
    // const QStringList mandatory = {
    //     "main", "nsfw", "jailbreak", "chatHistory", "dialogueExamples"
    // };
    // for (const QString& id : mandatory) {
    //     if (seenIds.contains(id)) continue;
    //     PromptEntity p;
    //     p.identifier = id;
    //     p.system_prompt = true;
    //     if (id == "chatHistory" || id == "dialogueExamples") {
    //         p.marker = true;
    //     } else if (promptEntityMap.contains(id)) {
    //         const auto& pe = promptEntityMap[id];
    //         p.content = pe.content;
    //         p.role = pe.role;
    //     } else if (id == "main") {
    //         p.content = QStringLiteral(
    //             "Write {{char}}'s next reply in a fictional chat between {{char}} and {{user}}.");
    //     }
    //     // enabled 默认 true
    //     insertByCanonical(result,seenIds,p);
    // }


    // ===== 应用角色卡覆盖（main 和 jailbreak）=====
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


    // 宏替换（{{char}} / {{user}} / {{description}} / {{setvar}} …）。
    // 关键：ST 只为「集合里的提示词」调用 preparePrompt（PromptManager.js:1537-1556），
    // 而被禁用的提示词**不在集合里**——它们既不发送，也**不执行 {{setvar}}**。
    // 以前这里无条件替换所有 order 条目，导致「关掉的提示词」里的 setvar 仍然生效，
    // 把 {{getvar}} 渲染出 ST 根本不会有的文本（实测：actingsystem / hugou / huying / kill1）。
    // main 是特例：ST 即使它被禁用也会保留一个空壳条目。
    for (PromptEntity& p : result) {
        if (p.content.isEmpty()) continue;
        if (!p.isActive && p.identifier != QLatin1String("main")) continue;
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
    const PopulateContext& pCtx,
    const QVariantMap& vars,
    QStringList* outIdentifiers)
{
    Q_UNUSED(userInput)
    Q_UNUSED(character)

    // 变量表由 buildChatMessages 统一算好传进来（角色字段 + 聊天宏）；
    // absolute 提示词在 preparePromptCollection 里已经替换过一次，这里不重复替换（ST 同样只替换一次）。
    // 世界书深度注入的条目仍要用当前（含 setvar 结果）的变量表替换。

    // ============================================================
    // 步骤 1：按 prompts 的顺序把内容填进同名组
    // ============================================================
    // 关键点：不再做任何下标运算。组顺序由 order 决定，内容按 identifier 落位。
    // 旧实现用 promptIndex 去 add()，顺序表下标和集合下标不一致时会覆盖或塌缩，
    // 导致 worldInfoBefore 被 main 顶掉、dialogueExamples 顶掉别人的提示词。
    QStringList order;
    QMap<QString, PromptEntity> promptMap;
    QList<PromptEntity> absolutePrompts;//(聊天中)注入列表

    for (const PromptEntity& p : prompts) {
        if (p.identifier.isEmpty()) continue;
        if (!order.contains(p.identifier)) order.append(p.identifier);
        promptMap[p.identifier] = p;

        // absolute(绝对注入) 交给 populationInjectionPrompts 按深度插入历史
        if (p.injection_position == 1) {
            absolutePrompts.append(p);
            continue;
        }

        // ST 的 control 提示不按集合位置发：
        //   impersonate / quietPrompt → controlPrompts（always last）
        //   bias → 作为 assistant 消息追加在最后
        //   groupNudge → 群聊时并进 chatHistory
        // 这里统一跳过，交给下面步骤 2 处理，避免同一条发两遍。
        if (p.identifier == QLatin1String("impersonate")
            || p.identifier == QLatin1String("quietPrompt")
            || p.identifier == QLatin1String("groupNudge")
            || p.identifier == QLatin1String("bias")) {
            continue;
        }

        // 被禁用的提示词不发送
        if (!p.isActive) continue;

        // 内容按原样进组（ST 不 trim prompt 正文：预设里用 " " 占位的条目
        // 在 ST 里真的会发出去，squash 之后表现为一条消息里的空行）
        const QString& content = p.content;
        // 空的 system 提示直接丢；空的 user/assistant 提示要留着当合并屏障（ST 语义）
        if (content.isEmpty() && p.role == GlobalConstants::MessageType::System) continue;

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
    // ST 从集合里取 impersonate（openai.js:1240）：被禁用时集合里没有它 → 不发。
    if (pCtx.type == QStringLiteral("impersonate")
        && promptMap.contains(QStringLiteral("impersonate"))) {
        const PromptEntity& p = promptMap[QStringLiteral("impersonate")];
        const QString content = p.isActive ? p.content.trimmed() : QString();
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
    // 步骤 4：continue 相关（prefill / nudge）
    // ============================================================
    if (pCtx.type == QStringLiteral("continue")) {
        // prefill：把**最新一条**消息（不限角色，ST 就是 messages.shift()）移出历史，
        // 作为预填追加到 controlPrompts 末尾。旧实现要求必须是 assistant，用户消息时行为不同。
        if (preset.continuePrefill && !historyAsMessages.isEmpty()) {
            const ChatMessage lastMsg = historyAsMessages.takeLast();
            SMessage m;
            m.identifier = QStringLiteral("continuePrefill");
            m.role = lastMsg.role;
            m.content = lastMsg.textContent();
            m.tokens = estimateTokens(m.content) + PER_MESSAGE_OVERHEAD;
            chatCompletion.insert(m, QStringLiteral("controlPrompts"));
        } else if (!preset.continueNudgePrompt.trimmed().isEmpty()) {
            // 非 prefill：ST 把最新一条消息从历史里摘出来，和 nudge 一起放进独立的
            // continueNudge 集合，整体排在 controlPrompts 之前（openai.js:911-931）。
            const ChatMessage lastMsg = historyAsMessages.isEmpty()
                ? ChatMessage() : historyAsMessages.takeLast();
            const QString lastText = lastMsg.textContent().trimmed();

            chatCompletion.ensureGroup(QStringLiteral("continueNudge"));
            if (!lastText.isEmpty()) {
                SMessage lm;
                lm.identifier = QStringLiteral("continueNudge-last");
                lm.role = lastMsg.role;
                lm.content = lastText;
                lm.tokens = estimateTokens(lm.content) + PER_MESSAGE_OVERHEAD;
                chatCompletion.insert(lm, QStringLiteral("continueNudge"));
            }

            // ST 支持 {{lastChatMessage}}
            QVariantMap nudgeVars = vars;
            nudgeVars.insert(QStringLiteral("lastChatMessage"), lastText);
            SMessage m;
            m.identifier = QStringLiteral("continueNudge");
            m.role = GlobalConstants::MessageType::System;
            m.content = substituteParams(preset.continueNudgePrompt.trimmed(), nudgeVars).trimmed();
            m.tokens = estimateTokens(m.content) + PER_MESSAGE_OVERHEAD;
            chatCompletion.insert(m, QStringLiteral("continueNudge"));

            const int controlIdx = order.indexOf(QStringLiteral("controlPrompts"));
            order.insert(controlIdx < 0 ? order.size() : controlIdx,
                         QStringLiteral("continueNudge"));
        }
    }

    // 在"固定提示词本身装不进预算"时直接中止本次生成
    //（openai.js add() → checkTokenBudget() → TokenBudgetExceededError），
    // 这里对齐成抛异常，由 ChatService 转成 generationFailed。
    if (chatCompletion.isOverBudget()) {
        THROW(PromptBuilderError, QStringLiteral(
            "提示词超出上下文预算：固定提示词占 %1 tokens，预算只有 %2 tokens。"
            "请调大上下文长度，或禁用部分提示词。")
                  .arg(chatCompletion.totalTokens())
                  .arg(chatCompletion.budget()));
    }

    // ============================================================
    // 步骤 5：in-chat 注入（绝对位置提示词 + 世界书 depth 条目）
    // ============================================================
    QList<ChatMessage> injectedMessages = populationInjectionPrompts(
        absolutePrompts, historyAsMessages, wi, vars);

    // ============================================================
    // 步骤 6：历史 + 示例（各自填自己的组，相对顺序由 prompts 决定）
    // ST 的顺序：默认先历史后示例（pin_examples 时反过来，这里还没实现那个开关）。
    // 某一段对应的提示词被禁用时整段不发送（ST 的两个 populate 函数开头就 return）。
    // 提示词表里完全没有这两条时按"没配"处理、仍然发送，避免残破预设把整段历史丢掉。
    // ============================================================
    const auto sectionEnabled = [&promptMap](const QString& identifier) {
        const auto it = promptMap.constFind(identifier);
        return it == promptMap.constEnd() || it->isActive;
    };

    if (sectionEnabled(QStringLiteral("chatHistory")))
        populateChatHistory(injectedMessages, chatCompletion, preset, vars);
    if (sectionEnabled(QStringLiteral("dialogueExamples")))
        populateDialogueExamples(pCtx.messageExamples, chatCompletion, preset, character, vars);

    // ============================================================
    // 步骤 7：扁平化
    // ============================================================
    // ST 默认 squash_system_messages=true（预设里的字段），把连续的 system 消息
    // 用 '\n' 拼成一条。A.U.T.O. 这类"一条提示拆成几十条 prompt"的预设全靠它。
    return chatCompletion.build(order, preset.squashSystemMessages, outIdentifiers);
}

// ============================================================
// 变量替换（SillyTavern 宏语义，见 MacroEngine.h / docs/MACRO_ENGINE.md）
// ============================================================
QString PromptBuilder::substituteParams(const QString& input, const QVariantMap& vars)
{
    if (input.isEmpty()) return input;

    MacroEnv env;
    env.userName = vars.value(QStringLiteral("user")).toString();
    env.charName = vars.value(QStringLiteral("char")).toString();

    for (auto it = vars.cbegin(); it != vars.cend(); ++it)
        env.values.insert(it.key().toLower(), it.value().toString());

    // 局部变量按会话分桶；会话 id 没设时退化成一个空 key 的桶（单测/预览场景）
    env.localVars = &m_localVariables[m_variableSession];
    env.globalVars = &m_globalVariables;

    return MacroEngine::evaluate(input, env);
}

// ======================================
// 聊天级宏（ST scripts/macros/definitions/chat-macros.js:103-116）
//   lastMessage      = 聊天里最后一条消息的正文（任意角色）
//   lastUserMessage  = 最后一条 user 消息（is_user && !is_system）
//   lastCharMessage  = 最后一条非 user / 非 system 消息
// ST 读的是聊天数组里的 mes（原始文本），这里以 history 为准；
// history 为空时（预览/单测）退回本轮用户输入。
// ======================================
QVariantMap PromptBuilder::getChatMacros(const QList<ChatMessageEntity>& history,
                                         const QString& userInput) const
{
    QString lastMessage;
    QString lastUserMessage;
    QString lastCharMessage;
    bool hasHistory = false;
    bool sawUser = false;

    for (const ChatMessageEntity& msg : history) {
        if (msg.isHidden) continue;
        if (msg.status != GlobalConstants::MessageGenerationStatus::Normal) continue;
        hasHistory = true;
        lastMessage = msg.content;
        if (msg.type == GlobalConstants::MessageType::User) {
            lastUserMessage = msg.content;
            sawUser = true;
        } else if (msg.type == GlobalConstants::MessageType::Assistant) {
            lastCharMessage = msg.content;
        }
    }

    if (!hasHistory && !userInput.isEmpty()) lastMessage = userInput;
    if (!sawUser && !userInput.isEmpty()) lastUserMessage = userInput;

    QVariantMap vars;
    vars.insert(QStringLiteral("lastMessage"), lastMessage);
    vars.insert(QStringLiteral("lastUserMessage"), lastUserMessage);
    vars.insert(QStringLiteral("lastCharMessage"), lastCharMessage);
    return vars;
}

// ======================================
// Prompt trace：YUZORA_TRACE_PROMPT=1 时把本次装配落成 JSON
// （identifiers 是我们和 ST 唯一能逐条对齐的东西；ST 的预设里也用同一批 identifier）
// ======================================
void PromptBuilder::writePromptTrace(const CompletionPresetEntity& preset,
                                     const QList<PromptEntity>& allPrompts,
                                     const QString& generationType,
                                     const QList<ChatMessage>& messages,
                                     const QStringList& messageIdentifiers) const
{
    if (!qEnvironmentVariableIsSet("YUZORA_TRACE_PROMPT")) return;

    const bool full = qEnvironmentVariableIsSet("YUZORA_TRACE_FULL");
    const auto sha1 = [](const QString& text) {
        return QString::fromLatin1(
            QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha1).toHex());
    };
    const auto roleName = [](GlobalConstants::MessageType role) {
        switch (role) {
        case GlobalConstants::MessageType::User: return QStringLiteral("user");
        case GlobalConstants::MessageType::Assistant: return QStringLiteral("assistant");
        default: return QStringLiteral("system");
        }
    };

    QMap<QString, const PromptEntity*> lookup;
    for (const PromptEntity& p : allPrompts) lookup.insert(p.identifier, &p);

    QJsonArray orderArr;
    QJsonArray entries;
    for (int i = 0; i < preset.promptOrder.size(); ++i) {
        const QString id = preset.promptOrder.at(i);
        orderArr.append(id);

        QJsonObject entry;
        entry["index"] = i;
        entry["identifier"] = id;
        const PromptEntity* p = lookup.value(id, nullptr);
        if (!p) {
            entry["missing"] = true;
        } else {
            entry["name"] = p->name;
            entry["role"] = roleName(p->role);
            entry["active"] = p->isActive;
            entry["rawLen"] = static_cast<int>(p->content.size());
            // 原始内容的哈希：用来对比「库里的预设 vs ST 现在的预设文件」有没有漂移
            entry["rawSha1"] = sha1(p->content);
        }
        entries.append(entry);
    }

    QJsonObject presetObj;
    presetObj["id"] = preset.id;
    presetObj["name"] = preset.name;
    presetObj["squashSystemMessages"] = preset.squashSystemMessages;
    presetObj["maxContextToken"] = preset.maxContextToken;
    presetObj["maxResponseToken"] = preset.maxResponseToken;
    // 这些字段决定"空标记要不要发"和"世界书包不包格式"，
    // tools/st_preset_diff.py --trace 靠它们核对 ST 的 new_chat_prompt / wi_format。
    presetObj["wiFormat"] = preset.wiFormat;
    presetObj["scenarioFormat"] = preset.scenarioFormat;
    presetObj["personalityFormat"] = preset.personalityFormat;
    presetObj["newChatPrompt"] = preset.newChatPrompt;
    presetObj["newExampleChatPrompt"] = preset.newExampleChatPrompt;

    QJsonArray msgs;
    for (int i = 0; i < messages.size(); ++i) {
        const ChatMessage& m = messages.at(i);
        const QString text = m.textContent();

        QJsonObject obj;
        obj["index"] = i;
        obj["identifier"] = messageIdentifiers.value(i);
        obj["role"] = roleName(m.role);
        if (!m.senderName.isEmpty()) obj["name"] = m.senderName;
        obj["len"] = static_cast<int>(text.size());
        obj["sha1"] = sha1(text);
        obj["containsMacro"] = text.contains(QStringLiteral("{{"));
        obj["head"] = text.left(120);
        obj["tail"] = text.right(120);
        if (full) obj["content"] = text;
        msgs.append(obj);
    }

    QJsonObject root;
    root["version"] = 1;
    root["preset"] = presetObj;
    root["generationType"] = generationType;
    root["promptOrder"] = orderArr;
    root["entries"] = entries;
    root["messages"] = msgs;

    const QString path = QDir::current().filePath(QStringLiteral("prompt_trace_last.json"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "[PromptBuilder] prompt trace 写入失败:" << path << file.errorString();
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    qInfo().noquote() << "[PromptBuilder] prompt trace 已写入:" << path
                      << "消息数:" << messages.size();
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


QVariantMap PromptBuilder::getPromptVariables(const CharacterEntity& character,
                                             const QString& userName)
{
    QVariantMap vars;
    vars.insert("char", character.name);
    vars.insert("character", character.name);
    vars.insert("description", character.description);
    vars.insert("personality", character.personality);
    vars.insert("scenario", character.scenario);
    vars.insert("first_mes", character.firstMessage);
    vars.insert("example_dialogue", character.exampleDialogue);

    // ST 一侧的宏名（script.js:2924-2954 的 environment）：
    // 角色卡字段有专门的名字，不要和上面这几个自造名混用
    vars.insert("charprompt", character.systemPrompt);
    vars.insert("charinstruction", character.postHistoryInstructions);
    vars.insert("charjailbreak", character.postHistoryInstructions);
    vars.insert("chardepthprompt", QString());
    vars.insert("mesexamples", character.exampleDialogue);
    vars.insert("mesexamplesraw", character.exampleDialogue);
    vars.insert("creatornotes", character.creatorNotes);
    vars.insert("charversion", character.characterVersion);
    vars.insert("char_version", character.characterVersion);
    vars.insert("persona", QString());   // Persona 还没入库

    // {{user}} 由调用方注入（ChatPromptContext 从 SettingsService 取 USER_NAME）。
    // PromptBuilder 保持无单例/无 DB 依赖，单测里一份 fixture 就能断言消息序列。
    const QString trimmedUser = userName.trimmed();
    const QString finalUser = trimmedUser.isEmpty() ? QStringLiteral("User") : trimmedUser;
    vars.insert("user", finalUser);
    vars.insert("notchar", finalUser);
    vars.insert("group", character.name);
    vars.insert("charifnotgroup", character.name);
    vars.insert("groupnotmuted", character.name);
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
    // 与 ST openai.js:814-879 populationInjectionPrompts 对齐：
    //  - depth 0 = 最新一条之后，depth 越大越靠近历史开头；插入位置 = size - depth，
    //    从深到浅插，索引不会漂移。
    //  - 同一 depth 内先按 injection_order 分桶，再按 role 合并成一条：
    //    预设 absolute 提示词（按顺序表顺序）在前，世界书 depth 条目在后 ——
    //    后者在 ST 里是 extension prompt，只并进 order=100 那一桶（openai.js:859-862）。
    //  - ST 把 [system, user, assistant] 追加进 newest-first 数组后整体 reverse，
    //    最终自上而下是 assistant → user → system，且 order 小的桶在上面；
    //    这里直接按"order 升序、role = Assistant/User/System"落位，结果等价。
    using Role = GlobalConstants::MessageType;

    //禁止多余角色,只有 system/user/assistant 三种注入角色
    auto normalizeRole = [](Role role) {
        if (role == Role::User || role == Role::Assistant) return role;
        return Role::System;
    };

    struct RoleParts {
        QStringList preset;      // absolute 提示词正文（按顺序表顺序）
        QStringList extension;   // 世界书 depth 条目
    };
    // depth -> order -> role -> parts
    QMap<int, QMap<int, QMap<Role, RoleParts>>> byDepth;

    // 1. 预设的 absolute(绝对) 条目：内容在 preparePromptCollection 里已做过宏替换
    for (const PromptEntity & p : absolutePrompts) {
        if (!p.isActive) continue;
        const QString content = p.content.trimmed();
        if (content.isEmpty()) continue;
        byDepth[p.injection_depth][p.injection_order][normalizeRole(p.role)].preset.append(content);
    }

    // 2. 世界书 depth 条目：ST 把它们当 order=100 的 extension prompt
    for (const WorldInfoDepthGroup & g : wi.worldInfoDepth) {
        if (g.entries.isEmpty()) continue;
        QStringList parts;
        for (const QString& entry : g.entries) {
            const QString content = substituteParams(entry, vars).trimmed();
            if (!content.isEmpty()) parts.append(content);
        }
        if (parts.isEmpty()) continue;
        // 即使这一桶只有世界书条目，也要把 order=100 建出来（下面按桶遍历）
        byDepth[g.depth][100];
        byDepth[g.depth][100][normalizeRole(g.insertRole)].extension.append(parts.join('\n'));
    }

    if (byDepth.isEmpty()) return messages;

    QList<ChatMessage> result = messages;

    // 3. 从深到浅插入，索引不会因为前面的插入而漂移
    for (auto depthIt = byDepth.end(); depthIt != byDepth.begin();) {
        --depthIt;
        const int depth = depthIt.key();
        const QMap<int, QMap<Role, RoleParts>> & orders = depthIt.value();

        QList<ChatMessage> block;
        for (auto orderIt = orders.cbegin(); orderIt != orders.cend(); ++orderIt) {
            const int order = orderIt.key();
            const QMap<Role, RoleParts> & roles = orderIt.value();
            const QList<Role> roleOrder = {Role::Assistant, Role::User, Role::System};
            for (const Role role : roleOrder) {
                const auto roleIt = roles.constFind(role);
                if (roleIt == roles.constEnd()) continue;
                QStringList parts = roleIt->preset;
                if (order == 100 && !roleIt->extension.isEmpty())
                    parts += roleIt->extension;
                const QString content = parts.join('\n').trimmed();
                if (content.isEmpty()) continue;
                ChatMessage m;
                m.role = role;
                m.appendText(content);
                block.append(m);
            }
        }
        if (block.isEmpty()) continue;

        int insertIdx = result.size() - depth;
        if (insertIdx < 0) insertIdx = 0;
        if (insertIdx > result.size()) insertIdx = result.size();
        for (int i = 0; i < block.size(); ++i)
            result.insert(insertIdx + i, block[i]);
    }

    return result;
}

//填充对话示例（组名 dialogueExamples，位置由 prompts 顺序决定）
void PromptBuilder::populateDialogueExamples(
    const QList<QList<ChatMessage>> &messageExamples,
    ChatCompletion &chatCompletion,
    const CompletionPresetEntity &preset, const CharacterEntity &character,
    const QVariantMap& vars)
{
    Q_UNUSED(character)

    if (messageExamples.isEmpty()) return;

    chatCompletion.ensureGroup(QStringLiteral("dialogueExamples"));

    // ST：每个示例块前面单独插一条 New Example Chat 提示。
    // ST openai.js:1108：内容只过 substituteParams，空串时该 Message 会被 getChat() 丢掉，
    // 但块里的示例消息照发。所以这里不再把空串回退成 "[Example Chat]"。
    const QString markerText = substituteParams(preset.newExampleChatPrompt, vars);
    const bool hasMarker = !markerText.isEmpty();

    for (int b = 0; b < messageExamples.size(); ++b) {
        QList<SMessage> block;

        if (hasMarker) {
            SMessage marker;
            // ST 的 identifier 是 'newChat'（openai.js:1108），并且在 squashSystemMessages 的
            // 例外名单里——换了名字就会被并进上一条 system 消息。
            marker.identifier = QStringLiteral("newChat");
            marker.role = GlobalConstants::MessageType::System;
            marker.content = markerText;
            marker.tokens = estimateTokens(markerText) + PER_MESSAGE_OVERHEAD;
            block.append(marker);
        }

        for (int i = 0; i < messageExamples[b].size(); ++i) {
            const ChatMessage& cm = messageExamples[b][i];
            SMessage m;
            m.identifier = QStringLiteral("dialogueExamples-%1-%2").arg(b).arg(i);
            // ST：示例一律以 system 角色发送，说话人靠 name 字段区分（openai.js:1119-1125）
            m.role = GlobalConstants::MessageType::System;
            m.content = cm.textContent().trimmed();
            if (m.content.isEmpty()) continue;
            m.name = cm.senderName;
            m.tokens = estimateTokens(m.content) + PER_MESSAGE_OVERHEAD;
            block.append(m);
        }

        // 既没标记也没示例正文，这一块就什么都不发
        if (block.isEmpty()) continue;
        if (!chatCompletion.canAffordAll(block)) break;

        for (const SMessage& m : block)
            chatCompletion.insert(m, QStringLiteral("dialogueExamples"));
    }
}

//填充聊天历史记录
void PromptBuilder::populateChatHistory(
    const QList<ChatMessage>& messages,
    ChatCompletion& chatCompletion,
    const CompletionPresetEntity& preset,
    const QVariantMap& vars)
{
    chatCompletion.ensureGroup(QStringLiteral("chatHistory"));

    // New Chat 提示：ST 把它放在聊天记录的**开头**
    //（"sent before the chat history …where background information ends and chat history begins"）。
    // 旧实现用 "end" 插在最新一条之后，等于每轮都在告诉模型"这是新对话的开头"。
    // ST openai.js:893-895：内容只过 substituteParams，**不 trim、不补默认值**；
    // 空串时 Message 内容是空，getChat()（openai.js:4186）会把它丢掉 ——
    // 也就是说预设里显式写 "" 时 ST 根本不发这条。
    // 默认值属于"没设置"的语义，放在 CompletionPresetEntity/DAO 那一层；
    // 这里若把显式空串回退成 [Start a new Chat]，狐神抚这类预设就会凭空多一条。
    const QString newChatPrompt = substituteParams(preset.newChatPrompt, vars);
    SMessage newChatMsg;
    newChatMsg.identifier = QStringLiteral("newMainChat");
    newChatMsg.role = GlobalConstants::System;
    newChatMsg.content = newChatPrompt;
    newChatMsg.tokens = newChatPrompt.isEmpty()
        ? 0
        : estimateTokens(newChatPrompt) + PER_MESSAGE_OVERHEAD;

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
        // ST 的 populateChatHistory 对每条历史消息都调 preparePrompt（openai.js:955），
        // 历史里的 {{user}} / {{char}} / {{getvar}} 同样要被替换。
        m.content = substituteParams(cm.textContent(), vars).trimmed();
        if (m.content.isEmpty()) continue;
        // ST 默认不给历史消息带 name（只有示例区带）；命名行为属于 names_behavior，尚未实现
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

    // 空内容不插：ST 端 Message.createAsync('system', '') 会被 getChat() 丢掉。
    // 这里显式跳过，预算与 trace/outIdentifiers 都不再出现 newMainChat。
    if (!newChatPrompt.isEmpty())
        chatCompletion.insert(newChatMsg, QStringLiteral("chatHistory"), ChatCompletion::Pos::Start);
    for (const auto& m : picked)
        chatCompletion.insert(m, QStringLiteral("chatHistory"), ChatCompletion::Pos::End);
}

// 角色卡 example_dialogue + 世界书 EMTop/EMBottom → <START> 分隔的示例块
QList<QList<ChatMessage>> PromptBuilder::parseExampleDialogue(
        const CharacterEntity& character, const WorldInfoResult& wi,
         const QVariantMap& vars)
{
    QList<QList<ChatMessage>> blocks;

    // ST：世界书 EMTop/EMBottom 条目会先并进"示例对话"文本、再一起解析
    //（script.js:4638-4658）。EMTop 用 unshift，后出现的条目排更前；EMBottom 用 push。
    for (const WorldInfoExample& example : wi.worldInfoExamples) {
        if (example.position != WorldInfoEntryEntity::EMTop) continue;
        const QList<QList<ChatMessage>> parsed =
            parseExampleText(substituteParams(example.content, vars), character, vars);
        if (parsed.isEmpty()) continue;
        QList<QList<ChatMessage>> merged = parsed;
        merged.append(blocks);
        blocks = merged;
    }

    blocks.append(parseExampleText(character.exampleDialogue, character, vars));

    for (const WorldInfoExample& example : wi.worldInfoExamples) {
        if (example.position != WorldInfoEntryEntity::EMBottom) continue;
        blocks.append(parseExampleText(substituteParams(example.content, vars), character, vars));
    }

    return blocks;
}

// 解析一段示例文本（ST：<START> 分块，行首 "{{user}}:" / "{{char}}:" 切分说话人）。
// 保留 user/assistant 角色只为了记录说话人；发送时统一按 system + name。
QList<QList<ChatMessage>> PromptBuilder::parseExampleText(const QString& raw,
                                                          const CharacterEntity& character,
                                                          const QVariantMap& vars)
{
    QList<QList<ChatMessage>> blocks;

    if (raw.trimmed().isEmpty()) return blocks;

    const QString userName = vars.value(QStringLiteral("user")).toString();

    const QStringList userPrefixes = {
        QStringLiteral("{{user}}"),
        QStringLiteral("user"),
        userName,
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
            ChatMessage msg = ChatMessage::makeText(GlobalConstants::MessageType::User,
                                                    substituteParams(rest, vars));
            msg.senderName = userName;
            current.append(msg);
            continue;
        }
        if (matchPrefix(line, charPrefixes, rest)) {
            ChatMessage msg = ChatMessage::makeText(GlobalConstants::MessageType::Assistant,
                                                    substituteParams(rest, vars));
            msg.senderName = character.name;
            current.append(msg);
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
    m_guaranteedPrompts = {
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

void PromptBuilder::insertByCanonical(
        QList<PromptEntity> & result,QSet<QString> &seenIds,const PromptEntity& p)
{
    // ST 的默认顺序。顺序表里缺的系统锚点按这个顺序**插到对应位置**，
    // 而不是一律追加到末尾——否则 worldInfoAfter 会跑到角色描述前面、
    // jailbreak 会跑到聊天记录前面（都不是 ST 的语义）。
    // 按 canonical 顺序插入一条提示词：插到"最后一个 canonical 不比它靠后"的项之后。
    // 这样即使用户在顺序表里插了自定义提示词（canonical 未知），锚点也会落在
    // 它应该挨着的那条附近，而不是一律跑到末尾。
    auto canonicalIndex = [this](const QString& id) {
        const int idx = m_guaranteedPrompts.indexOf(id);
        return idx < 0 ? std::numeric_limits<int>::max() : idx;
    };
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
}


