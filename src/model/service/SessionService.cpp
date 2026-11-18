#include "SessionService.h"

#include <QDateTime>
#include <QRegularExpression>

#include "CharacterDao.h"
#include "CharacterEntity.h"
#include "ChatSessionDao.h"
#include "GlobalConstants.h"
#include "SettingsService.h"
#include "Utils.h"

namespace {
constexpr int kAutoTitleMaxLength = 24;

// 标题只显示一行：折叠换行/连续空白，超长才加省略号。
// 这里不解析 Markdown / HTML，标题按纯文本显示（QML 侧也统一 textFormat: PlainText）。
QString makeSingleLineTitle(const QString& raw)
{
    static const QRegularExpression whitespace(QStringLiteral("\\s+"));
    QString text = raw;
    text.replace(whitespace, QStringLiteral(" "));
    text = text.trimmed();

    if (text.size() > kAutoTitleMaxLength)
        text = text.left(kAutoTitleMaxLength).trimmed() + QStringLiteral("…");
    return text;
}
}

SessionService::SessionService(ChatSessionDao* sessionDao, CharacterDao* characterDao,
                               SettingsService* settingsService, QObject* parent)
    : QObject(parent)
    , m_sessionDao(sessionDao)
    , m_characterDao(characterDao)
    , m_settingsService(settingsService)
{
}

// ======================================
// 当前会话
// ======================================
ChatSessionEntity SessionService::current() const
{
    if (m_currentSessionId.isEmpty()) return {};
    return m_sessionDao->selectById(m_currentSessionId);
}

// ======================================
// 查询
// ======================================
ChatSessionEntity SessionService::entity(const QString& sessionId) const
{
    if (sessionId.isEmpty()) return {};
    return m_sessionDao->selectById(sessionId);
}

bool SessionService::exists(const QString& sessionId) const
{
    return m_sessionDao->exists(sessionId);
}

QList<ChatSessionEntity> SessionService::listForCharacter(const QString& characterId)
{
    if (characterId.isEmpty()) return {};
    return m_sessionDao->selectByCharacterId(characterId);
}

QList<ChatSessionEntity> SessionService::listForCurrentCharacter()
{
    if (m_currentCharacterId.isEmpty()) {
        Utils::writeLog("ERROR", "SessionService", "获取角色会话失败,当前角色为空");
        return {};
    }
    return m_sessionDao->selectByCharacterId(m_currentCharacterId);
}

QString SessionService::mostRecentSessionId(const QString& characterId)
{
    const QList<ChatSessionEntity> sessions = listForCharacter(characterId);
    QString newestId;
    QDateTime newest;
    for (const ChatSessionEntity& candidate : sessions) {
        if (newestId.isEmpty() || candidate.lastModified > newest) {
            newestId = candidate.id;
            newest = candidate.lastModified;
        }
    }
    return newestId;
}

QString SessionService::lastSessionIdForCharacter(const QString& characterId)
{
    if (characterId.isEmpty()) return {};
    return m_characterDao->selectById(characterId).lastSessionId;
}

// ======================================
// 写入
// ======================================
QString SessionService::create(const QString& characterId)
{
    if (characterId.isEmpty()) return {};

    ChatSessionEntity session;
    session.id = Utils::generateUuid();
    session.characterId = characterId;
    session.title = QStringLiteral("新对话");
    session.createdAt = QDateTime::currentDateTime();
    session.lastModified = QDateTime::currentDateTime();

    if (!m_sessionDao->insert(session)) {
        Utils::writeLog("ERROR", "SessionService", "创建聊天会话失败");
        return {};
    }
    return session.id;
}

bool SessionService::remove(const QString& sessionId)
{
    if (sessionId.isEmpty()) return false;
    if (!m_sessionDao->remove(sessionId)) {
        Utils::writeLog("ERROR", "SessionService", "删除会话失败: " + sessionId);
        return false;
    }
    return true;
}

bool SessionService::removeAllForCharacter(const QString& characterId)
{
    if (characterId.isEmpty()) return false;
    if (!m_sessionDao->removeByCharacterId(characterId)) {
        Utils::writeLog("ERROR", "SessionService", "删除全部会话失败");
        return false;
    }
    return true;
}

bool SessionService::rename(const QString& sessionId, const QString& title)
{
    if (sessionId.isEmpty()) return false;

    const QString trimmed = title.trimmed();
    if (trimmed.isEmpty()) return false;

    ChatSessionEntity session = m_sessionDao->selectById(sessionId);
    if (session.id.isEmpty()) return false;

    // 标题会进抽屉行和页面副标题，限长避免把布局挤爆。
    // 注意：不改 lastModified —— 改名不该让会话在列表里跳到最前。
    session.title = trimmed.left(40);
    if (!m_sessionDao->update(session)) {
        Utils::writeLog("ERROR", "SessionService", "重命名会话失败: " + sessionId);
        return false;
    }
    return true;
}

// ======================================
// 切换与持久化
// ======================================
ChatSessionEntity SessionService::switchTo(const QString& sessionId)
{
    if (sessionId.isEmpty()) return {};

    const ChatSessionEntity session = m_sessionDao->selectById(sessionId);
    if (session.id.isEmpty()) return {};

    m_currentSessionId = session.id;
    m_currentCharacterId = session.characterId;
    saveLastSession();
    return session;
}

void SessionService::clearCurrent()
{
    m_currentSessionId.clear();
    m_currentCharacterId.clear();
}

void SessionService::saveLastSession()
{
    if (m_currentCharacterId.isEmpty() || m_currentSessionId.isEmpty()) return;
    if (!m_settingsService || !m_characterDao) return;

    // 更新最后会话
    CharacterEntity character = m_characterDao->selectById(m_currentCharacterId);
    character.lastSessionId = m_currentSessionId;
    m_characterDao->update(character);

    // 更新最后角色
    m_settingsService->setValue(GlobalConstants::LAST_ACTIVE_CHARACTER_ID, m_currentCharacterId);
}

QString SessionService::lastActiveCharacterId() const
{
    if (!m_settingsService) return {};
    return m_settingsService->getString(GlobalConstants::LAST_ACTIVE_CHARACTER_ID);
}

void SessionService::clearLastActiveCharacter()
{
    if (m_settingsService)
        m_settingsService->setValue(GlobalConstants::LAST_ACTIVE_CHARACTER_ID, "");
}

void SessionService::touch(const QString& sessionId)
{
    if (sessionId.isEmpty()) return;
    m_sessionDao->updateLastModified(sessionId, QDateTime::currentDateTime());
}

bool SessionService::autoTitleFromUserMessage(const QString& sessionId, const QString& text,
                                               bool overwriteExisting)
{
    if (sessionId.isEmpty()) return false;

    ChatSessionEntity session = m_sessionDao->selectById(sessionId);
    if (session.id.isEmpty()) return false;

    // 用户手动改过名就不覆盖（目前没有 title_is_manual 列，只能用默认标题判断）
    if (!overwriteExisting && session.title != QStringLiteral("新对话")) return false;

    const QString title = makeSingleLineTitle(text);
    if (title.isEmpty()) return false;

    session.title = title;
    if (!m_sessionDao->update(session)) {
        Utils::writeLog("ERROR", "SessionService", QStringLiteral("标题更新失败:%1").arg(sessionId));
        return false;
    }
    return true;
}
