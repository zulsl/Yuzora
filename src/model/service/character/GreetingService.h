#ifndef GREETINGSERVICE_H
#define GREETINGSERVICE_H

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

class CharacterService;
class ChatMessageDao;

// 切换结果：被改写的消息 id 与新内容（空 id = 没有发生切换）
struct GreetingSwitchResult {
    QString messageId;
    QString content;
};

// 开场白：角色卡上的"首条消息 + 备选开场白"集合，与某个会话里承载它的那条 assistant 消息。
//
// 从 ChatService 抽出来的原因：它只依赖"角色卡 + 消息表"，跟生成、网络、会话 CRUD 无关，
// 却是 ChatService 里最密的一段状态（消息 id / 序号 / 总数）。抽出来之后：
//   - 开场白的三件套状态只在这里维护，别的代码没法把它改乱；
//   - 单测可以直接对着一个 GreetingService 测切换、循环、旧会话内容反推。
//
// 约定：会话归属的角色由调用方传入（ChatService 知道当前是哪个角色）。
class GreetingService : public QObject {
    Q_OBJECT
public:
    explicit GreetingService(ChatMessageDao* messageDao, CharacterService* characterService,
                             QObject* parent = nullptr);

    // 承载开场白的那条消息 id（空 = 当前会话没有可切换的开场白）
    QString messageId() const { return m_messageId; }
    // 当前用的是第几条（从 0 开始；-1 = 无）
    int index() const { return m_index; }
    // 开场白总数（首条消息 + 备选，已过滤空串）
    int count() const { return m_count; }

    // 新建会话：把第 1 条开场白写成会话的第一条消息；角色没有开场白时返回 true 且什么都不做。
    // 写库失败才返回 false。
    bool insertGreetingMessage(const QString& sessionId, const QString& characterId);

    // 会话切换 / 开场白消息被删后重新定位状态（旧会话没有 metadata 时按内容反推并补写）
    void refresh(const QString& sessionId, const QString& characterId);

    // 循环切换（+1 下一条 / -1 上一条），发生在承载消息上：只改内容，不新增消息
    GreetingSwitchResult switchBy(const QString& characterId, int delta);

signals:
    void stateChanged();//承载消息 / 序号 / 总数任意一项变化

private:
    // 角色卡上的开场白集合：首条消息在前，备选依次排后，空串过滤掉
    QStringList greetingTextsFor(const QString& characterId) const;

    ChatMessageDao* m_messageDao = nullptr;
    CharacterService* m_characterService = nullptr;

    QString m_messageId;
    int m_index = -1;
    int m_count = 0;
};

#endif // GREETINGSERVICE_H
