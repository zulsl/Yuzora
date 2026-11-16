#include "ChatPromptContext.h"

#include <QSet>
#include <algorithm>

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
        ChatMessageDao* messageDao,CharacterService* characterService,
        const QString& generationType)
{
    if (!messageDao || !characterService)
        return {};

    //角色卡
    const CharacterEntity character = characterService->getCharacterById(characterId);
    if (character.id.isEmpty()) return {};

    //历史消息
    const QList<ChatMessageEntity> allMsgs =
        messageDao->selectLatestBySessionId(sessionId, 1000);

    // 提取有效历史消息(仅正常完成、未隐藏)
    QList<ChatMessageEntity> validHistory;
    for (const ChatMessageEntity& msg : allMsgs) {
        if (msg.status == GlobalConstants::MessageGenerationStatus::Normal && !msg.isHidden)
            validHistory.append(msg);
    }

    //预设
    const CompletionPresetEntity preset = CompletionPresetService::instance().getActivePreset();

    // 世界书：所有启用的文集 + 角色绑定的全部文集。
    QList<WorldInfoEntryEntity> worldEntries =
            WorldInfoService::instance().getEntryForEnabledBooks();//获取启用的世界书
    //添加角色书
    if(!character.worldBookIds.isEmpty()){
        QList<WorldInfoBookEntity> books=WorldInfoService::instance().getEnabledBooks();
        WorldInfoBookEntity cBook=
                WorldInfoService::instance().getBookById(character.worldBookIds.first());
        bool exist =
            std::any_of(books.cbegin(), books.cend(),[&cBook](const WorldInfoBookEntity &b) {
                return b.id == cBook.id;
            });
        if(!exist){
            QList<WorldInfoEntryEntity> cEntries=WorldInfoService::instance().getEntriesByBook(cBook.id);
            worldEntries.append(cEntries);
        }
    }

    const WorldInfoResult wir =
            WorldInfoMatcher::instance().matchAndResolve(allMsgs,worldEntries,userInput,character);

    // 长期记忆注入（预留扩展）与 Persona 是两条不同的东西，别再共用同一个形参：
    // memorySection 目前没有生产者，personaDesc 也还没有入库，这里都先传空。
    const QString personaDesc;

    // 预设里的提示词表由这里取，PromptBuilder 保持无 DB 依赖（方便单测）
    const QList<PromptEntity> allPrompts =
            CompletionPresetService::instance().getPromptsByPresetId(preset.id);

    return PromptBuilder::instance().buildChatMessages(
            character, validHistory, userInput, preset, wir, allPrompts, personaDesc, generationType);
}
