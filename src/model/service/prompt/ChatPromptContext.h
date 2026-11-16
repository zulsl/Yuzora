#ifndef CHATPROMPTCONTEXT_H
#define CHATPROMPTCONTEXT_H

#include <QList>
#include <QString>

#include "ChatMessage.h"

class CharacterService;
class ChatMessageDao;

// 提示词上下文装配：角色卡 + 历史消息 + 预设 + 世界书 → 发给模型的 chat messages。
//
// 从 ChatService 里抽出来，因为它是"取数 + 拼装"的纯流程，跟聊天运行时（会话状态、
// 网络请求）没有关系。抽出来之后：
//   1. ChatService 不再直接引用 CompletionPresetService / WorldInfoService /
//      WorldInfoMatcher / PromptBuilder 四个单例；
//   2. 单测里用一份假实现替换这个函数，就能在没有预设、没有世界书、没有网络的情况下
//      测"发送消息"这条链路。
//
// generationType: "normal" / "continue" / "regenerate" / "impersonate" / "quiet"，
// 决定 PromptBuilder 走哪条分支（continue 的 nudge/prefill、impersonate 的模仿提示等）。
QList<ChatMessage> buildChatPromptContext(
        const QString& characterId,const QString& sessionId,
        const QString& userInput,ChatMessageDao* messageDao,CharacterService* characterService,
        const QString& generationType = QStringLiteral("normal"));

#endif // CHATPROMPTCONTEXT_H
