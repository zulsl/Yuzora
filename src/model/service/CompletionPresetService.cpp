#include "CompletionPresetService.h"
#include "Utils.h"
#include "ScopeTimer.h"

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
    initBuiltinPresets();
    m_currentActivePreset=m_completionPresetDao->selectDefault();
}

CompletionPresetService::~CompletionPresetService() = default;

void CompletionPresetService::initBuiltinPresets()
{
    if (!m_completionPresetDao->selectAll().isEmpty()) return;

    // 内置1：平衡模式（默认）
    CompletionPresetEntity balanced;
    balanced.id = Utils::generateUuid();
    balanced.name = "平衡模式 (默认)";
    balanced.isDefault = true;
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
    PromptEntity item1;
    item1.id=Utils::generateUuid();
    item1.identifier = "main";
    item1.presetId=presetId;
    item1.name = "主提示词";
    item1.system_prompt = true;
    item1.marker = true;
    item1.content = "推进剧情";
    item1.injection_depth = 4;
    item1.forbid_overrides = false;
    createPrompt(item1);

    //顺序条目
    QString entity1;
    entity1="main";
    promptOrderList.append(entity1);

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
    return m_currentActivePreset;
    // return m_completionPresetDao->selectDefault();
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
    if (preset.isDefault) {
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
    newPreset.isDefault = false;
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
    if (success) m_currentActivePreset=m_completionPresetDao->selectDefault();
    else emit operationError("设置默认预设失败");
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

    return m_promptDao->selectPromptsByPresetId(id);
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

CompletionPresetEntity CompletionPresetService::parsePresetDataToEntity(const QJsonObject& stJson,const QString& fileName,QList<PromptEntity>& promptList)
{
    CompletionPresetEntity preset;
    preset.id = Utils::generateUuid();
    preset.name = fileName;
    preset.isDefault = false;

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

    //实用提示词
    preset.impersonationPrompt = stJson.value("impersonation_prompt").toString();
    preset.wiFormat = stJson.value("wi_format").toString();
    preset.scenarioFormat = stJson.value("scenario_format").toString();
    preset.personalityFormat = stJson.value("personality_format").toString();
    preset.groupNudgePrompt = stJson.value("group_nudge_prompt").toString();
    preset.newChatPrompt = stJson.value("new_chat_prompt").toString();
    preset.newGroupChatPrompt = stJson.value("new_group_chat_prompt").toString();
    preset.newExampleChatPrompt = stJson.value("new_example_chat_prompt").toString();
    preset.continueNudgePrompt = stJson.value("continue_nudge_prompt").toString();

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

    //读取并解析json
    QList<PromptEntity> promptList;
    CompletionPresetEntity entity=parsePresetDataToEntity(doc.object(),fileName,promptList);
    createPreset(entity);
    createPrompts(promptList);

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