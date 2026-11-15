#include "GreetingService.h"

#include <QDateTime>
#include <QVariantMap>

#include "CharacterEntity.h"
#include "CharacterService.h"
#include "ChatMessageDao.h"
#include "ChatMessageEntity.h"
#include "GlobalConstants.h"
#include "Utils.h"

GreetingService::GreetingService(ChatMessageDao* messageDao, CharacterService* characterService,
                                 QObject* parent)
    : QObject(parent)
    , m_messageDao(messageDao)
    , m_characterService(characterService)
{
}

QStringList GreetingService::greetingTextsFor(const QString& characterId) const
{
    QStringList texts;
    if (characterId.isEmpty() || !m_characterService) return texts;

    const CharacterEntity character = m_characterService->getCharacterById(characterId);

    // 首条消息就是第 1 条开场白；备选依次排后。空串没有切换价值，直接过滤。
    if (!character.firstMessage.trimmed().isEmpty())
        texts.append(character.firstMessage);
    for (const QString& greeting : character.alternateGreetings) {
        if (!greeting.trimmed().isEmpty())
            texts.append(greeting);
    }
    return texts;
}

bool GreetingService::insertGreetingMessage(const QString& sessionId, const QString& characterId)
{
    if (sessionId.isEmpty() || !m_messageDao) return false;

    const QStringList texts = greetingTextsFor(characterId);
    if (texts.isEmpty()) return true;   // 角色没有开场白：不是失败

    ChatMessageEntity firstMsg;
    firstMsg.id = Utils::generateUuid();
    firstMsg.sessionId = sessionId;
    firstMsg.type = GlobalConstants::MessageType::Assistant;
    firstMsg.status = GlobalConstants::MessageGenerationStatus::Normal;
    firstMsg.content = texts.first();
    firstMsg.senderName = QStringLiteral("第一条消息");
    firstMsg.timestamp = QDateTime::currentDateTime();
    firstMsg.metadata.insert(QStringLiteral("greetingIndex"), 0);

    return m_messageDao->insert(firstMsg);
}

void GreetingService::refresh(const QString& sessionId, const QString& characterId)
{
    m_messageId.clear();
    m_index = -1;
    m_count = 0;

    if (!sessionId.isEmpty() && m_messageDao) {
        const QStringList texts = greetingTextsFor(characterId);
        m_count = texts.size();

        ChatMessageEntity first = m_messageDao->selectFirstBySessionId(sessionId);
        if (!first.id.isEmpty() && !texts.isEmpty()
            && first.type == GlobalConstants::MessageType::Assistant) {

            int index = first.metadata.value(QStringLiteral("greetingIndex"), -1).toInt();

            // 旧会话没有 metadata：按内容反推一条，命中就补写回去，
            // 这样升级前就存在的对话也能直接用上切换
            if (index < 0) {
                index = texts.indexOf(first.content);
                if (index >= 0) {
                    first.metadata.insert(QStringLiteral("greetingIndex"), index);
                    m_messageDao->update(first);
                }
            }

            if (index >= 0 && index < texts.size()) {
                m_messageId = first.id;
                m_index = index;
            }
        }
    }

    emit stateChanged();
}

GreetingSwitchResult GreetingService::switchBy(const QString& characterId, int delta)
{
    if (m_messageId.isEmpty() || m_count <= 1 || delta == 0 || !m_messageDao)
        return {};

    // 循环取模；负数也要落到合法区间
    const int next = ((m_index + delta) % m_count + m_count) % m_count;
    if (next == m_index) return {};

    const QStringList texts = greetingTextsFor(characterId);
    if (next < 0 || next >= texts.size()) return {};

    ChatMessageEntity message = m_messageDao->selectById(m_messageId);
    if (message.id.isEmpty()) return {};

    message.content = texts.at(next);
    message.metadata.insert(QStringLiteral("greetingIndex"), next);
    // 切换开场白不是"用户编辑内容"，不要动 isEdited

    if (!m_messageDao->update(message)) {
        Utils::writeLog("ERROR", "GreetingService", "切换开场白失败: " + message.id);
        return {};
    }

    m_index = next;
    emit stateChanged();
    return {message.id, message.content};
}
