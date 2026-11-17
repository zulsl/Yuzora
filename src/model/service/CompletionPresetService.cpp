#include "CompletionPresetService.h"
#include "Utils.h"
#include "ScopeTimer.h"
#include "DatabaseManager.h"
#include "ConnectionProviderRegistry.h"
#include "IConnectionProvider.h"

#include <QDateTime>
#include <QSet>
#include <QSqlDatabase>

CompletionPresetService& CompletionPresetService::instance()
{
    static CompletionPresetService instance;
    return instance;
}

CompletionPresetService::CompletionPresetService(QObject* parent)
    : QObject(parent)
    , m_completionPresetDao(new CompletionPresetDao(this))
    , m_promptDao(new PromptDao(this))
{
}

CompletionPresetService::CompletionPresetService(CompletionPresetDao* presetDao,
                                                 PromptDao* promptDao,
                                                 QObject* parent)
    : QObject(parent)
    , m_completionPresetDao(presetDao ? presetDao : new CompletionPresetDao(this))
    , m_promptDao(promptDao ? promptDao : new PromptDao(this))
{
}

CompletionPresetService::~CompletionPresetService() = default;

void CompletionPresetService::ensureSeedData()
{
    initBuiltinPresets();
}

void CompletionPresetService::initBuiltinPresets()
{
    if (!m_completionPresetDao->selectAll().isEmpty()) return;

    // 内置1：平衡模式（默认）
    CompletionPresetEntity balanced;
    balanced.id = Utils::generateUuid();
    balanced.name = "平衡模式 (默认)";
    balanced.isActive = true;
    balanced.maxContextToken = 4096;
    balanced.maxResponseToken = 1024;
    balanced.streamEnabled = true;
    CreateDefaultPromptEntries(balanced.id,balanced.promptOrder);
    balanced.createdAt = QDateTime::currentDateTime();
    balanced.updatedAt=QDateTime::currentDateTime();
    createPreset(balanced);
    // 内置2：平衡模式（默认）
}

QStringList CompletionPresetService::parsePromptOrder(
        const QJsonObject &cpJson,QHash<QString, bool>& h_isActive)
{
    QJsonArray prompt_order=cpJson.value("prompt_order").toArray();
    QJsonArray order_100001;
    for (const QJsonValue &value : prompt_order) {
        if (!value.isObject())
            continue;
        QJsonObject obj = value.toObject();
        // 注意：toInt() 适用于 JSON 中是数字的情况
        if (obj.value("character_id").toInt() == 100001) {
            order_100001 = obj.value("order").toArray();
            break;
        }
    }
    QStringList result;
    result.reserve(order_100001.size());
    for (const auto& o : order_100001)
    {
        if (!o.isObject())
            continue;

        QJsonObject obj = o.toObject();

        QString identifier;
        identifier=obj["identifier"].toString();
        bool enabled;
        enabled = obj["enabled"].toBool();

        h_isActive.insert(identifier,enabled);

        result.append(identifier);
    }

    return result;
}

void CompletionPresetService::CreateDefaultPromptEntries(const QString& presetId,QStringList & promptOrderList)
{
    //提示词条目
    PromptEntity main;
    main.id=Utils::generateUuid();
    main.identifier = "main";
    main.presetId=presetId;
    main.name = "主提示词";
    main.system_prompt = true;
    main.marker = false;
    main.content = "Write {{char}}'s next reply in a fictional chat between {{char}} and {{user}}.";
    main.injection_depth = 4;
    main.forbid_overrides = false;

    PromptEntity worldInfoAfter;
    worldInfoAfter.id=Utils::generateUuid();
    worldInfoAfter.identifier = "worldInfoAfter";
    worldInfoAfter.presetId=presetId;
    worldInfoAfter.name = "世界书之后";
    worldInfoAfter.system_prompt = true;
    worldInfoAfter.marker = true;

    PromptEntity personaDescription;
    personaDescription.id=Utils::generateUuid();
    personaDescription.identifier = "personaDescription";
    personaDescription.presetId=presetId;
    personaDescription.name = "(用户描述)";
    personaDescription.system_prompt = true;
    personaDescription.marker = true;

    PromptEntity charDescription;
    charDescription.id=Utils::generateUuid();
    charDescription.identifier = "charDescription";
    charDescription.presetId=presetId;
    charDescription.name = "(角色描述)";
    charDescription.system_prompt = true;
    charDescription.marker = true;

    PromptEntity charPersonality;
    charPersonality.id=Utils::generateUuid();
    charPersonality.identifier = "charPersonality";
    charPersonality.presetId=presetId;
    charPersonality.name = "(角色性格)";
    charPersonality.system_prompt = true;
    charPersonality.marker = true;

    PromptEntity scenario;
    scenario.id=Utils::generateUuid();
    scenario.identifier = "scenario";
    scenario.presetId=presetId;
    scenario.name = "(角色场景)";
    scenario.system_prompt = true;
    scenario.marker = true;

    PromptEntity enhanceDefinitions;
    enhanceDefinitions.id=Utils::generateUuid();
    enhanceDefinitions.identifier = "enhanceDefinitions";
    enhanceDefinitions.content = "If you have more knowledge of {{char}}, add to the character's lore and personality to enhance them but keep the Character Sheet's definitions absolute.";
    enhanceDefinitions.presetId=presetId;
    enhanceDefinitions.name = "(增强定义)";
    enhanceDefinitions.system_prompt = true;
    enhanceDefinitions.marker = false;
    enhanceDefinitions.isActive = false;

    PromptEntity nsfw;
    nsfw.id=Utils::generateUuid();
    nsfw.identifier = "nsfw";
    nsfw.presetId=presetId;
    nsfw.name = "(辅助提示)";
    nsfw.system_prompt = true;
    nsfw.marker = false;

    PromptEntity worldInfoBefore;
    worldInfoBefore.id=Utils::generateUuid();
    worldInfoBefore.identifier = "worldInfoBefore";
    worldInfoBefore.presetId=presetId;
    worldInfoBefore.name = "世界书之前";
    worldInfoBefore.system_prompt = true;
    worldInfoBefore.marker = true;

    PromptEntity dialogueExamples;
    dialogueExamples.id=Utils::generateUuid();
    dialogueExamples.identifier = "dialogueExamples";
    dialogueExamples.presetId=presetId;
    dialogueExamples.name = "对话示例";
    dialogueExamples.system_prompt = true;
    dialogueExamples.marker = true;

    PromptEntity chatHistory;
    chatHistory.id=Utils::generateUuid();
    chatHistory.identifier = "chatHistory";
    chatHistory.presetId=presetId;
    chatHistory.name = "聊天历史";
    chatHistory.system_prompt = true;
    chatHistory.marker = true;

    PromptEntity jailbreak;
    jailbreak.id=Utils::generateUuid();
    jailbreak.identifier = "jailbreak";
    jailbreak.presetId=presetId;
    jailbreak.name = "Post-History Instructions";
    jailbreak.system_prompt = true;


    createPrompt(main);
    createPrompt(worldInfoAfter);
    createPrompt(personaDescription);
    createPrompt(charDescription);
    createPrompt(charPersonality);
    createPrompt(scenario);
    createPrompt(enhanceDefinitions);
    createPrompt(nsfw);
    createPrompt(worldInfoBefore);
    createPrompt(dialogueExamples);
    createPrompt(chatHistory);
    createPrompt(jailbreak);


    //顺序条目
    promptOrderList<<"main"<<"worldInfoBefore"<<"personaDescription"<<"charDescription"
        <<"charPersonality"<<"scenario"<<"enhanceDefinitions"<<"nsfw"<<"worldInfoAfter"
        <<"dialogueExamples"<<"chatHistory"<<"jailbreak";

}

bool CompletionPresetService::validatePreset(const CompletionPresetEntity& preset, QString& outError)
{
    if (preset.name.trimmed().isEmpty()) {
        outError = "预设名称不能为空";
        return false;
    }
    if (preset.maxContextToken <= 0 || preset.maxResponseToken <= 0) {
        outError = "上下文与回复Token必须大于0";
        return false;
    }
    if (preset.temperature < 0 || preset.temperature > 2) {
        outError = "Temperature 取值范围 0-2";
        return false;
    }
    return true;
}

QList<CompletionPresetEntity> CompletionPresetService::getAllPresets()
{
    return m_completionPresetDao->selectAll();
}

CompletionPresetEntity CompletionPresetService::getPresetById(const QString& id)
{
    return m_completionPresetDao->selectById(id);
}

CompletionPresetEntity CompletionPresetService::getActivePreset()const
{
    return m_completionPresetDao->selectDefault();
}

QString CompletionPresetService::createPreset(const CompletionPresetEntity& preset)
{
    QString error;
    if (!validatePreset(preset, error)) {
        emit operationError(error);
        return {};
    }

    CompletionPresetEntity newPreset = preset;
    if (newPreset.id.isEmpty()) newPreset.id = Utils::generateUuid();

    bool success = m_completionPresetDao->insert(newPreset);
    if (!success) {
        emit operationError("创建预设失败");
        return {};
    }
    return newPreset.id;
}

bool CompletionPresetService::updatePreset(const CompletionPresetEntity& preset)
{
    QString error;
    if (!validatePreset(preset, error)) {
        emit operationError(error);
        return false;
    }
    bool success = m_completionPresetDao->update(preset);
    if (success) {
        emit presetUpdated(preset.id);
    } else {
        emit operationError("更新预设失败");
    }
    return success;
}

bool CompletionPresetService::deletePreset(const QString& id)
{
    CompletionPresetEntity preset = m_completionPresetDao->selectById(id);
    if (preset.id.isEmpty()) {
        emit operationError("预设不存在");
        return false;
    }
    if (preset.isActive) {
        emit operationError("无法删除默认预设");
        return false;
    }
    bool success = m_completionPresetDao->remove(id);
    if (success) {
        emit presetDeleted(id);
    }
    return success;
}

QString CompletionPresetService::duplicatePreset(const QString& sourceId)
{
    CompletionPresetEntity source = m_completionPresetDao->selectById(sourceId);
    if (source.id.isEmpty()) {
        emit operationError("源预设不存在");
        return {};
    }
    CompletionPresetEntity newPreset = source;
    newPreset.id = Utils::generateUuid();
    newPreset.name = source.name + " (副本)";
    newPreset.isActive = false;
    bool success = m_completionPresetDao->insert(newPreset);
    if (!success) {
        emit operationError("复制预设失败");
        return {};
    }
    return newPreset.id;
}

bool CompletionPresetService::setActivePreset(const QString& id)
{
    bool success = m_completionPresetDao->setDefault(id);
    if (!success) emit operationError("设置默认预设失败");
    return success;
}

bool CompletionPresetService::renamePreset(const QString& id, const QString& newName)
{
    if (newName.trimmed().isEmpty()) {
        emit operationError("名称不能为空");
        return false;
    }
    bool success = m_completionPresetDao->rename(id, newName.trimmed());
    if (success) {
        emit presetUpdated(id);
    }
    return success;
}

QList<PromptEntity> CompletionPresetService::getPromptsByPresetId(const QString &id)
{
    if (id.isEmpty()) return {};

    QList<PromptEntity> prompts = m_promptDao->selectPromptsByPresetId(id);

    // main 是"主提示词"，内容由用户编辑，不是系统占位。
    // 早期建库时把它也打上了 marker=1，导致预设界面会把它当成占位锁住；
    // 这里统一归一化，避免去改历史数据（marker 只影响编辑态，不影响提示词装配）。
    for (PromptEntity& prompt : prompts) {
        if (prompt.identifier == QStringLiteral("main"))
            prompt.marker = false;
    }

    return prompts;
}

PromptEntity CompletionPresetService::getPromptById(const QString & id)
{
    return PromptEntity();
}

bool CompletionPresetService::createPrompts(const QList<PromptEntity>& prompts)
{
    bool success = m_promptDao->createPrompts(prompts);
    if (!success) {
        emit operationError("创建Prompts失败");
        return false;
    }
    return true;
}

bool CompletionPresetService::createPrompt(const PromptEntity & prompt)
{
    PromptEntity newPrompt = prompt;
    if (newPrompt.id.isEmpty()) newPrompt.id = Utils::generateUuid();

    bool success = m_promptDao->insert(newPrompt);
    if (!success) {
        emit operationError("创建失败");
        return false;
    }
    return true;
}

bool CompletionPresetService::updatePrompt(const PromptEntity &prompt)
{
    bool success = m_promptDao->update(prompt);
    if (success) {
        // emit presetUpdated(preset.id);
    } else {
        emit operationError("更新提示词失败");
    }
    return success;
}

QStringList CompletionPresetService::normalizePromptOrder(const QStringList& order) const
{
    QStringList result;
    QSet<QString> seen;
    for (const QString& identifier : order) {
        if (identifier.isEmpty() || seen.contains(identifier)) continue;
        seen.insert(identifier);
        result.append(identifier);
    }
    return result;
}

bool CompletionPresetService::writePromptChange(
    const CompletionPresetEntity& preset,
    const PromptEntity& prompt,
    const QStringList& promptOrder,
    bool insertPrompt,
    const QString& deletePromptId)
{
    IConnectionProvider* provider = ConnectionProviderRegistry::provider();
    QSqlDatabase db = provider ? provider->connection() : QSqlDatabase();
    if (!db.isOpen() || !db.transaction()) {
        emit operationError("开启提示词事务失败");
        return false;
    }

    bool success = insertPrompt
        ? m_promptDao->insert(prompt)
        : m_promptDao->remove(deletePromptId);

    if (success) {
        CompletionPresetEntity updatedPreset = preset;
        updatedPreset.promptOrder = promptOrder;
        // 删除时若 promptOrder 里本来就没有这个 identifier，预设内容没有变化，
        // 不需要再 UPDATE 一次（否则 BaseDao 会因 0 行受影响而误判失败）。
        if (updatedPreset.promptOrder != preset.promptOrder) {
            success = m_completionPresetDao->update(updatedPreset);
        }
    }

    if (!success) {
        db.rollback();
        emit operationError("保存提示词失败");
        return false;
    }

    if (!db.commit()) {
        db.rollback();
        emit operationError("提交提示词事务失败");
        return false;
    }
    return true;
}

bool CompletionPresetService::addPrompt(const QString& presetId, const PromptEntity& prompt)
{
    CompletionPresetEntity preset = m_completionPresetDao->selectById(presetId);
    if (preset.id.isEmpty()) {
        emit operationError("预设不存在");
        return false;
    }

    PromptEntity newPrompt = prompt;
    if (newPrompt.id.isEmpty()) newPrompt.id = Utils::generateUuid();
    if (newPrompt.identifier.isEmpty()) {
        newPrompt.identifier = newPrompt.id;
    }
    newPrompt.presetId = presetId;

    QStringList order = normalizePromptOrder(preset.promptOrder);
    if (order.contains(newPrompt.identifier)) {
        emit operationError("提示词 identifier 重复");
        return false;
    }

    order.append(newPrompt.identifier);
    return writePromptChange(preset, newPrompt, order, true, QString());
}

bool CompletionPresetService::insertPromptBefore(
    const QString& presetId, const QString& beforeIdentifier, const PromptEntity& prompt)
{
    CompletionPresetEntity preset = m_completionPresetDao->selectById(presetId);
    if (preset.id.isEmpty()) {
        emit operationError("预设不存在");
        return false;
    }

    PromptEntity newPrompt = prompt;
    if (newPrompt.id.isEmpty()) newPrompt.id = Utils::generateUuid();
    if (newPrompt.identifier.isEmpty()) {
        newPrompt.identifier = QStringLiteral("custom-") + newPrompt.id;
    }
    newPrompt.presetId = presetId;

    QStringList order = normalizePromptOrder(preset.promptOrder);
    if (order.contains(newPrompt.identifier)) {
        emit operationError("提示词 identifier 重复");
        return false;
    }

    const int beforeIndex = order.indexOf(beforeIdentifier);
    if (beforeIndex < 0) {
        order.append(newPrompt.identifier);
    } else {
        order.insert(beforeIndex, newPrompt.identifier);
    }

    return writePromptChange(preset, newPrompt, order, true, QString());
}

bool CompletionPresetService::deletePrompt(const QString& presetId, const QString& promptId)
{
    CompletionPresetEntity preset = m_completionPresetDao->selectById(presetId);
    if (preset.id.isEmpty()) {
        emit operationError("预设不存在");
        return false;
    }

    PromptEntity prompt = m_promptDao->selectById(promptId);
    if (prompt.id.isEmpty() || prompt.presetId != presetId) {
        emit operationError("提示词不存在");
        return false;
    }
    if (prompt.system_prompt) {
        emit operationError("系统内置提示词不可删除");
        return false;
    }

    QStringList order = normalizePromptOrder(preset.promptOrder);
    order.removeAll(prompt.identifier);

    return writePromptChange(preset, prompt, order, false, promptId);
}

CompletionPresetEntity CompletionPresetService::parsePresetDataToEntity(const QJsonObject& stJson,const QString& fileName,QList<PromptEntity>& promptList)
{
    CompletionPresetEntity preset;
    preset.id = Utils::generateUuid();
    preset.name = fileName;
    preset.isActive = false;

    // 基础生成
    preset.maxContextToken = stJson.value("openai_max_context").toInt();
    preset.maxResponseToken = stJson.value("openai_max_tokens").toInt();
    preset.streamEnabled = stJson.value("stream_openai").toBool();

    //其他
    preset.maxContextUnlocked = stJson.value("max_context_unlocked").toBool();
    preset.namesBehavior = stJson.value("names_behavior").toInt();
    preset.continuePostfix = stJson.value("continue_postfix").toString();
    preset.continuePrefill = stJson.value("continue_prefill").toBool();
    preset.squashSystemMessages = stJson.value("squash_system_messages").toBool();
    preset.functionCalling = stJson.value("function_calling").toBool();
    preset.showThoughts = stJson.value("show_thoughts").toBool();


    // 采样参数
    preset.temperature = stJson.value("temperature").toDouble();
    preset.topP = stJson.value("top_p").toDouble();
    preset.topK = stJson.value("top_k").toDouble();
    preset.frequencyPenalty = stJson.value("frequency_penalty").toDouble();
    preset.presencePenalty = stJson.value("presence_penalty").toDouble();

    preset.repetitionPenalty = stJson.value("repetition_penalty").toDouble();
    preset.topA = stJson.value("top_a").toDouble();
    preset.minP = stJson.value("min_p").toDouble();

    //实用提示词：字段存在才覆盖；**显式空串是有效值**（"不要 New Chat 标记"），
    // 缺失时才保留实体默认（= ST 的 default_*，openai.js:107-114,428）。
    const auto assignString = [&stJson](const char* key, QString& field) {
        const QLatin1String name(key);
        if (stJson.contains(name))
            field = stJson.value(name).toString();
    };
    assignString("impersonation_prompt", preset.impersonationPrompt);
    assignString("wi_format", preset.wiFormat);
    assignString("scenario_format", preset.scenarioFormat);
    assignString("personality_format", preset.personalityFormat);
    assignString("group_nudge_prompt", preset.groupNudgePrompt);
    assignString("new_chat_prompt", preset.newChatPrompt);
    assignString("new_group_chat_prompt", preset.newGroupChatPrompt);
    assignString("new_example_chat_prompt", preset.newExampleChatPrompt);
    assignString("continue_nudge_prompt", preset.continueNudgePrompt);

    QHash<QString, bool> id_isActive;

    //提示词顺序
    preset.promptOrder = parsePromptOrder(stJson,id_isActive);


    //提示词库
    QJsonArray prompts=stJson.value("prompts").toArray();
    for (const QJsonValue& p : prompts)
    {
        if (!p.isObject())
            continue;
        QJsonObject obj = p.toObject();

        PromptEntity item;
        item.id=Utils::generateUuid();
        item.identifier = obj["identifier"].toString();
        item.presetId=preset.id;
        item.isActive=id_isActive.value(item.identifier);
        item.name = obj["name"].toString();
        item.system_prompt = obj["system_prompt"].toBool();
        item.marker = obj["marker"].toBool();//标记
        item.content = obj["content"].toString();

        QString role=obj["role"].toString();
        if(role == "system")
            item.role = GlobalConstants::MessageType::System;
        else if(role == "user")
            item.role = GlobalConstants::MessageType::User;
        else if(role == "assistant")
            item.role = GlobalConstants::MessageType::Assistant;

        item.injection_position = obj["injection_position"].toInt();
        item.injection_depth = obj["injection_depth"].toInt();
        item.forbid_overrides = obj["forbid_overrides"].toBool();

        promptList.append(item);
    }


    preset.createdAt = QDateTime::currentDateTime();
    preset.updatedAt = QDateTime::currentDateTime();

    return preset;
}

QJsonObject CompletionPresetService::toSillyTavernJson(const CompletionPresetEntity& preset)
{
    QJsonObject obj;
    obj["name"] = preset.name;
    obj["max_context_size"] = preset.maxContextToken;
    obj["max_length"] = preset.maxResponseToken;
    obj["stream"] = preset.streamEnabled;

    obj["temperature"] = preset.temperature;
    obj["top_p"] = preset.topP;
    obj["top_k"] = preset.topK;
    obj["repetition_penalty"] = preset.repetitionPenalty;
    obj["frequency_penalty"] = preset.frequencyPenalty;
    obj["presence_penalty"] = preset.presencePenalty;
    obj["top_a"] = preset.topA;
    obj["min_p"] = preset.minP;

    QJsonObject dynTemp;
    dynTemp["enabled"] = preset.dynamicTempEnabled;
    dynTemp["min"] = preset.dynamicTempMin;
    dynTemp["max"] = preset.dynamicTempMax;
    dynTemp["exponent"] = preset.dynamicTempExponent;
    obj["dynamic_temperature"] = dynTemp;

    QString tmplName;
    switch (preset.promptTemplate) {
        case GlobalConstants::PromptTemplateType::Alpaca: tmplName = "Alpaca"; break;
        case GlobalConstants::PromptTemplateType::ChatML: tmplName = "ChatML"; break;
        case GlobalConstants::PromptTemplateType::Llama2: tmplName = "Llama2"; break;
        case GlobalConstants::PromptTemplateType::Mistral: tmplName = "Mistral"; break;
        case GlobalConstants::PromptTemplateType::GeminiPrompt: tmplName = "Gemini"; break;
        default: tmplName = "Custom"; break;
    }
    obj["instruction_template"] = tmplName;
    obj["system_prompt_prefix"] = preset.systemPromptPrefix;
    obj["system_prompt_suffix"] = preset.systemPromptSuffix;
    obj["input_prefix"] = preset.userMessagePrefix;
    obj["input_suffix"] = preset.userMessageSuffix;
    obj["output_prefix"] = preset.assistantMessagePrefix;
    obj["output_suffix"] = preset.assistantMessageSuffix;
    obj["separator"] = preset.conversationSeparator;

    QJsonArray stopArr;
    for (const auto& s : preset.stoppingStrings) stopArr.append(s);
    obj["stopping_strings"] = stopArr;

    obj["auto_continue"] = preset.autoContinueEnabled;
    obj["auto_continue_max"] = preset.autoContinueMax;

    return obj;
}

int CompletionPresetService::importFromSillyTavern(const QString& filePath,const QString& fileName)
{
    QJsonDocument doc = Utils::readJsonFile(filePath);
    if (doc.isNull()) {
        emit operationError("文件读取失败或格式无效");
        return 0;
    }

    return importFromSillyTavernJson(doc.object(), fileName);
}

int CompletionPresetService::importFromSillyTavernJson(const QJsonObject& stJson,
                                                      const QString& fileName)
{
    if (stJson.isEmpty()) {
        emit operationError("文件读取失败或格式无效");
        return 0;
    }

    //读取并解析json
    QList<PromptEntity> promptList;
    CompletionPresetEntity entity = parsePresetDataToEntity(stJson, fileName, promptList);

    // 同名预设覆盖更新：ST 那边的预设是会变的（用户会开关提示词、改正文）。
    // 如果导入时总是新建一份，库里就会留着一份旧数据——实例：狐神抚的
    // 3a59f94a(19907 字) 在 ST 里已禁用，库里还是启用，请求体里白白多一整块。
    CompletionPresetEntity existing;
    for (const CompletionPresetEntity& preset : m_completionPresetDao->selectAll()) {
        if (preset.name == fileName) {
            existing = preset;
            break;
        }
    }

    if (!existing.id.isEmpty()) {
        entity.id = existing.id;
        entity.isActive = existing.isActive;      // 别把当前激活的预设弄丢
        entity.createdAt = existing.createdAt;
        entity.updatedAt = QDateTime::currentDateTime();

        if (!updatePreset(entity)) return 0;
        if (!m_promptDao->removeByPresetId(entity.id)) {
            emit operationError("旧提示词清理失败");
            return 0;
        }
        for (PromptEntity& prompt : promptList) prompt.presetId = entity.id;
        if (!createPrompts(promptList)) {
            emit operationError("提示词写入失败");
            return 0;
        }
        Utils::writeLog("INFO", "CompletionPresetService", "覆盖更新预设: " + fileName);
        return 1;
    }

    const QString newId = createPreset(entity);
    if (newId.isEmpty()) return 0;          // createPreset 已经 emit 过 operationError
    if (!createPrompts(promptList)) {
        emit operationError("提示词写入失败");
        return 0;
    }

    return 1;
}

bool CompletionPresetService::exportToSillyTavern(const QString& presetId, const QString& savePath)
{
    CompletionPresetEntity preset = m_completionPresetDao->selectById(presetId);
    if (preset.id.isEmpty()) {
        emit operationError("预设不存在");
        return false;
    }
    QJsonObject obj = toSillyTavernJson(preset);
    if (!Utils::writeJsonFile(savePath, QJsonDocument(obj))) {
        emit operationError("导出文件写入失败");
        return false;
    }
    emit exportSuccess();
    return true;
}

bool CompletionPresetService::exportAllToSillyTavern(const QString& savePath)
{
    QList<CompletionPresetEntity> presets = m_completionPresetDao->selectAll();
    if (presets.isEmpty()) {
        emit operationError("无可导出的预设");
        return false;
    }
    QJsonArray arr;
    for (const auto& p : presets) arr.append(toSillyTavernJson(p));
    if (!Utils::writeJsonFile(savePath, QJsonDocument(arr))) {
        emit operationError("导出文件写入失败");
        return false;
    }
    emit exportSuccess();
    return true;
}