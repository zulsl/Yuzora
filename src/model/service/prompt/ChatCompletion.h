#ifndef CHATCOMPLETION_H
#define CHATCOMPLETION_H

#include <QStringList>
#include <QDebug>

#include "ChatMessage.h"
#include "ChatMessageGroup.h"

/**
 * @brief 提示词组集合 + token 预算
 *
 * 关键设计（2026-11 重构，修 ST 顺序对齐问题）：
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

    // 上下文模型自身的 BOS 等开销，ST 里也是单独预留的
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
    void insert(const SMessage& m, const QString& identifier,
                Pos pos = Pos::End)
    {
        if (m.content.isEmpty()) return;

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
     */
    QList<ChatMessage> build(const QStringList& order) const
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
        for (const QString& id : sequence) {
            const ChatMessageGroup* group = findGroup(id);
            if (!group) continue;
            result.append(group->getChat());
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
