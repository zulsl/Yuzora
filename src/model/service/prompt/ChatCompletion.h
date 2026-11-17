#ifndef CHATCOMPLETION_H
#define CHATCOMPLETION_H

#include <QStringList>
#include <QDebug>

#include "ChatMessage.h"
#include "ChatMessageGroup.h"

/**
 * @brief 提示词组集合 + token 预算
 *
 * 关键设计（重构，修 ST 顺序对齐问题）：
 *   组只按 identifier 存在，**不按下标占位**。
 *
 * 旧实现是 add(group, promptIndex)，其中 promptIndex 是"预设顺序表里的下标"，
 * 而集合里的位置是"已经加进去几个组"。两者一旦不相等：
 *   - position < size 时 `m_collections[position] = c` 是**覆盖** ——
 *     worldInfoBefore 被 main 顶掉、dialogueExamples 顶掉别人的提示词；
 *   - position >= size 时是 append —— 所有系统项排到所有用户项前面，顺序塌缩。
 *
 * 现在的做法：组按 identifier 建（幂等），扁平化时由 build(order) 按顺序表排列，
 * 顺序表里没有的组按创建顺序接在尾部。
 *
 * 预算也不再在 add/insert 时逐笔扣（那样 reserve/free 不对称就会算错），
 * 而是随时由"预算 - 当前所有组的 token 和"算出来。
 */
class ChatCompletion {
public:
    enum class Pos { Start, End };

    // 上下文模型自身的 BOS 等开销
    static constexpr int BOS_RESERVE = 3;

    ChatCompletion(int context, int response)
        : m_budget(context - response - BOS_RESERVE) {}

    // 建组（幂等）：重复调用不会清空已有内容
    void ensureGroup(const QString& identifier)
    {
        if (findGroup(identifier)) return;
        m_collections.append(ChatMessageGroup(identifier));
    }

    bool has(const QString& identifier) const
    {
        return findGroup(identifier) != nullptr;
    }

    // 往某个组里插消息。组不存在时自动建，避免调用点漏 ensureGroup
    //
    // 空内容：system 的直接丢（ST 的 squashSystemMessages 里也这么干）；
    // user/assistant 的要留着当"合并屏障"——ST 里哪怕内容为空，它也会打断
    // 连续 system 消息的合并，最后发请求时再被 getChat() 丢掉。
    void insert(const SMessage& m, const QString& identifier,
                Pos pos = Pos::End)
    {
        if (m.content.isEmpty() && m.role == GlobalConstants::MessageType::System) return;

        ChatMessageGroup* group = findGroup(identifier);
        if (!group) {
            qWarning() << "[ChatCompletion] 组不存在，自动创建:" << identifier;
            ensureGroup(identifier);
            group = findGroup(identifier);
            if (!group) return;
        }

        if (pos == Pos::Start) group->collection.prepend(m);
        else                   group->collection.append(m);
    }

    int groupTokens(const QString& identifier) const
    {
        const ChatMessageGroup* group = findGroup(identifier);
        return group ? group->getTokens() : 0;
    }

    // 除指定组以外，其它所有组已占用的 token（算历史预算时用）
    int tokensExcept(const QString& identifier) const
    {
        int total = 0;
        for (const auto& group : m_collections) {
            if (group.identifier == identifier) continue;
            total += group.getTokens();
        }
        return total;
    }

    int totalTokens() const
    {
        int total = 0;
        for (const auto& group : m_collections) total += group.getTokens();
        return total;
    }

    // 剩余预算（可能为负：系统提示本身已经超了，此时历史一条都放不下）
    int remaining() const { return m_budget - totalTokens(); }

    // 总预算（context - response - BOS），报错文案用
    int budget() const { return m_budget; }

    // ST 在"提示词本身装不进预算"时直接中止生成（openai.js checkTokenBudget）。
    // 这里只暴露状态，报错方式交给调用方（PromptBuilder 抛 AppException）。
    bool isOverBudget() const { return remaining() < 0; }

    bool canAfford(int tokens) const { return remaining() >= tokens; }

    bool canAffordAll(const QList<SMessage>& ms) const
    {
        int total = 0;
        for (const auto& m : ms) total += m.tokens;
        return remaining() >= total;
    }

    /**
     * @brief 扁平化成最终消息序列
     * @param order 期望的组顺序（通常就是 preparePromptCollection 的结果）
     *              order 里没有的组按创建顺序追加在末尾（如 controlPrompts）
     * @param squashSystemMessages 是否合并连续的 system 消息（ST 的 squash_system_messages）
     * @param outIdentifiers 非空时按输出顺序记录每条消息来源的组 identifier
     *                       （squash 续接的消息不重复记录）。给 prompt trace / 测试用。
     *
     * ST 的 squashSystemMessages()（openai.js:3983-4015）：
     *   - 只合并 role == system **且没有 name** 的消息；
     *   - identifier 在 {newMainChat, newChat, groupNudge} 里的不合并（历史/示例/群 nudge 的边界）；
     *   - 内容是空串的 system 消息直接丢掉；
     *   - 合并方式是把后一条的 content 用 '\n' 接到前一条上（不是 '\n\n'）。
     * A.U.T.O. 这类预设把一大段系统提示拆成了很多条 prompt，ST 默认
     * squash_system_messages=true，靠这一步把它们拼回一条 system 消息。
     */
    QList<ChatMessage> build(const QStringList& order, bool squashSystemMessages = false,
                             QStringList* outIdentifiers = nullptr) const
    {
        QStringList sequence;
        sequence.reserve(order.size() + m_collections.size());
        for (const QString& id : order) {
            if (!sequence.contains(id)) sequence.append(id);
        }
        for (const auto& group : m_collections) {
            if (!sequence.contains(group.identifier))
                sequence.append(group.identifier);
        }

        QList<ChatMessage> result;
        bool lastSquashable = false;
        for (const QString& id : sequence) {
            const ChatMessageGroup* group = findGroup(id);
            if (!group) continue;
            for (const SMessage& m : group->collection) {
                if (m.content.isEmpty()) {
                    // system 空消息：丢掉，且不打断合并
                    // user/assistant 空消息：不发送，但会打断合并（ST 的语义）
                    if (m.role != GlobalConstants::MessageType::System) lastSquashable = false;
                    continue;
                }

                const bool squashable = squashSystemMessages
                    && m.role == GlobalConstants::MessageType::System
                    && m.name.isEmpty()
                    && m.identifier != QStringLiteral("newMainChat")
                    && m.identifier != QStringLiteral("newChat")
                    && m.identifier != QStringLiteral("groupNudge");

                if (squashable && lastSquashable && !result.isEmpty()) {
                    ChatMessage& last = result.last();
                    if (last.parts.isEmpty()) last.appendText(m.content);
                    else last.parts.last().text += QLatin1Char('\n') + m.content;
                    continue;
                }

                ChatMessage c;
                c.role = m.role;
                c.appendText(m.content);
                c.senderName = m.name;
                c.includeName = !m.name.isEmpty();
                result.append(c);
                if (outIdentifiers) outIdentifiers->append(id);
                lastSquashable = squashable;
            }
        }
        return result;
    }

private:
    ChatMessageGroup* findGroup(const QString& identifier)
    {
        for (auto& group : m_collections) {
            if (group.identifier == identifier) return &group;
        }
        return nullptr;
    }

    const ChatMessageGroup* findGroup(const QString& identifier) const
    {
        for (const auto& group : m_collections) {
            if (group.identifier == identifier) return &group;
        }
        return nullptr;
    }

    int m_budget = 0;
    QList<ChatMessageGroup> m_collections;
};

#endif // CHATCOMPLETION_H
