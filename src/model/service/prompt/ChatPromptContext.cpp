#include "ChatPromptContext.h"

#include "CharacterEntity.h"
#include "CharacterService.h"
#include "ChatMessageDao.h"
#include "ChatMessageEntity.h"
#include "CompletionPresetEntity.h"
#include "CompletionPresetService.h"
#include "GlobalConstants.h"
#include "PromptBuilder.h"
#include "WorldInfoEntryEntity.h"
#include "WorldInfoMatcher.h"
#include "WorldInfoService.h"

// 历史和世界书的取数顺序与原来 ChatService::buildPromptContext 完全一致：
// 取最近 1000 条 → 只保留"正常完成且未隐藏" → 世界书按全量消息扫描 → 交给 PromptBuilder。
QList<ChatMessage> buildChatPromptContext(
        const QString& characterId,const QString& sessionId,const QString& userInput,
        ChatMessageDao* messageDao,CharacterService* characterService)
{
    if (!messageDao || !characterService)
        return {};

    //角色卡
    const CharacterEntity character = characterService->getCharacterById(characterId);
    if (character.id.isEmpty()) return {};

    //历史消息
    const QList<ChatMessageEntity> allMsgs =
        messageDao->selectLatestBySessionId(sessionId, 1000);

    // 提取有效历史消息（仅正常完成、未隐藏）(可优化)
    QList<ChatMessageEntity> validHistory;
    for (const ChatMessageEntity& msg : allMsgs) {
        if (msg.status == GlobalConstants::MessageGenerationStatus::Normal && !msg.isHidden)
            validHistory.append(msg);
    }

    const CompletionPresetEntity preset = CompletionPresetService::instance().getActivePreset();

    // 世界书匹配
    const QList<WorldInfoEntryEntity> wibr =
            WorldInfoService::instance().getEntryForEnabledBooks();
    WorldInfoResult wir =
            WorldInfoMatcher::instance().matchAndResolve(allMsgs,wibr,userInput,character);

    // 长期记忆注入（预留扩展）
    const QString memorySection;

    // 填充优先级从高到低大致为：
    // 系统核心提示 → 角色描述 / 性格 / 场景 → 世界信息 → 对话示例 → 最新的聊天历史。
    // 当 token 预算不足时，会从优先级最低的内容（最久远的聊天记录）开始裁剪，
    // 直到总 token 符合预算。
    return PromptBuilder::instance().buildChatMessages(
            character, validHistory, userInput,preset, wir, memorySection);
}
