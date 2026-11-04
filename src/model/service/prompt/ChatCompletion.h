#ifndef CHATCOMPLETION_H
#define CHATCOMPLETION_H

#include <QObject>
#include <QJsonArray>
#include <QVariantMap>

#include "ChatMessage.h"
#include "ChatMessageGroup.h"

class ChatCompletion {
public:
    ChatCompletion(int context, int response)
        : m_budget(context - response) {}

    // 添加整个 消息组 到指定位置（-1 表示末尾）并消费预算
    void add(const ChatMessageGroup& c, int position = -1) {
        if (position >= 0 && position < m_collections.size()) {
            m_collections[position] = c;
        } else {
            m_collections.append(c);
        }
        m_budget -= c.getTokens();
    }

    // 在指定 identifier 的 collection 中插入单条消息
    void insert(const SMessage& m, const QString& identifier,
                const QString& pos = QStringLiteral("end")) {
        const int idx = findMessageIndex(identifier);
        if (m.content.isEmpty() || m.tokens == 0) return;

        auto& coll = m_collections[idx].collection;
        if (pos == QStringLiteral("start")) {
            coll.prepend(m);
        } else if (pos == QStringLiteral("end")) {
            coll.append(m);
        }
        m_budget -= m.tokens;
    }

    bool has(const QString& identifier) const {
        return findMessageIndex(identifier) >= 0;
    }

    bool canAfford(int tokens) const { return m_budget >= tokens; }

    bool canAffordAll(const QList<SMessage>& ms) const {
        int total = 0;
        for (const auto& m : ms) total += m.tokens;
        return m_budget >= total;
    }

    QList<ChatMessage> getChat() const {
        QList<ChatMessage> result;
        for (const auto& c : m_collections) {
            result.append(c.getChat());
        }
        return result;
    }

    int findMessageIndex(const QString& identifier) const {
        for (int i = 0; i < m_collections.size(); ++i) {
            if (m_collections[i].identifier == identifier) return i;
        }
        return -1;
    }

    int remaining() const { return m_budget; }
    void reserveBudget(int tokens) { m_budget -= tokens; }//预扣/预留的预算
    void freeBudget(int tokens)    { m_budget += tokens; }//释放/归还之前预留的的预算

    //设置超限提示
    void setOverriddenPrompts(const QSet<QString>& ids) { m_overridden = ids; }
    QSet<QString> getOverriddenPrompts() const { return m_overridden; }

private:
    int m_budget;//总共可用预算
    QList<ChatMessageGroup> m_collections;
    QSet<QString> m_overridden;
};
#endif