#ifndef SESSIONSERVICE_H
#define SESSIONSERVICE_H

#include <QList>
#include <QObject>
#include <QString>

#include "ChatSessionEntity.h"

class CharacterDao;
class ChatSessionDao;
class SettingsService;

// 会话：会话表的增删改查 + "当前会话 / 当前角色"指针 + 最近会话的持久化。
//
// 从 ChatService 抽出来，因为这块是纯数据与状态维护，跟生成、开场白、消息内容无关。
// 抽出来之后 ChatService 从"既管会话又管消息和生成"变成"编排者"：
// 它仍然决定"新建会话后要插开场白""删掉当前会话后落到哪一条"，
// 但不再直接碰 chat_sessions 表和 settings 里的最后角色。
//
// 约定：不持有 DAO / 服务的所有权，生命周期由外面的组合根负责。
class SessionService : public QObject {
    Q_OBJECT
public:
    SessionService(ChatSessionDao* sessionDao, CharacterDao* characterDao,
                   SettingsService* settingsService, QObject* parent = nullptr);

    // ========== 当前会话 ==========
    QString currentSessionId() const { return m_currentSessionId; }
    QString currentCharacterId() const { return m_currentCharacterId; }
    ChatSessionEntity current() const;//按 id 现查（改完标题立刻读得到）

    // ========== 查询 ==========
    ChatSessionEntity entity(const QString& sessionId) const;
    bool exists(const QString& sessionId) const;
    QList<ChatSessionEntity> listForCharacter(const QString& characterId);
    QList<ChatSessionEntity> listForCurrentCharacter();
    QString mostRecentSessionId(const QString& characterId);//last_modified 最新的那条
    QString lastSessionIdForCharacter(const QString& characterId);//角色记住的上次会话

    // ========== 写入 ==========
    // 只插会话行（标题"新对话"）；开场白由调用方补，会话服务不该知道有开场白这回事
    QString create(const QString& characterId);
    bool remove(const QString& sessionId);
    bool removeAllForCharacter(const QString& characterId);
    bool rename(const QString& sessionId, const QString& title);//trim + 截断 40

    // ========== 切换与持久化 ==========
    // 更新"当前会话/当前角色"指针，并把最近会话写回角色与设置；返回空实体 = 不存在
    ChatSessionEntity switchTo(const QString& sessionId);
    void clearCurrent();
    void saveLastSession();
    QString lastActiveCharacterId() const;
    void clearLastActiveCharacter();

    // 会话被改过（发消息、切开场白）后刷新 last_modified
    void touch(const QString& sessionId);
    // 标题还是"新对话"时，用回复的前 10 个字符自动生成标题
    void autoTitleFromReply(const QString& sessionId, const QString& replyText);

private:
    ChatSessionDao* m_sessionDao = nullptr;
    CharacterDao* m_characterDao = nullptr;
    SettingsService* m_settingsService = nullptr;

    QString m_currentSessionId;
    QString m_currentCharacterId;
};

#endif // SESSIONSERVICE_H
