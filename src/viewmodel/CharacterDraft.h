#ifndef CHARACTERDRAFT_H
#define CHARACTERDRAFT_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QDateTime>
#include <QtQml/qqmlregistration.h>

#include "CharacterEntity.h"

/**
 * @brief 角色编辑草稿（强类型）
 *
 * 替代 QVariantMap：QML 直接读写 typed property，setter 负责去重、发通知
 * 和通知 ViewModel 重新计算 dirty。字段与 CharacterEntity 一一对应。
 */
class CharacterDraft : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QString firstMessage READ firstMessage WRITE setFirstMessage NOTIFY firstMessageChanged)
    Q_PROPERTY(QStringList alternateGreetings READ alternateGreetings WRITE setAlternateGreetings NOTIFY alternateGreetingsChanged)
    Q_PROPERTY(QString creatorNotes READ creatorNotes WRITE setCreatorNotes NOTIFY creatorNotesChanged)
    Q_PROPERTY(QString creator READ creator WRITE setCreator NOTIFY creatorChanged)
    Q_PROPERTY(QString personality READ personality WRITE setPersonality NOTIFY personalityChanged)
    Q_PROPERTY(QString scenario READ scenario WRITE setScenario NOTIFY scenarioChanged)
    Q_PROPERTY(QString exampleDialogue READ exampleDialogue WRITE setExampleDialogue NOTIFY exampleDialogueChanged)
    Q_PROPERTY(QString characterVersion READ characterVersion WRITE setCharacterVersion NOTIFY characterVersionChanged)
    Q_PROPERTY(QStringList tags READ tags WRITE setTags NOTIFY tagsChanged)
    Q_PROPERTY(QString systemPrompt READ systemPrompt WRITE setSystemPrompt NOTIFY systemPromptChanged)
    Q_PROPERTY(QString postHistoryInstructions READ postHistoryInstructions WRITE setPostHistoryInstructions NOTIFY postHistoryInstructionsChanged)
    Q_PROPERTY(QString avatarPath READ avatarPath WRITE setAvatarPath NOTIFY avatarPathChanged)
    Q_PROPERTY(QStringList worldBookIds READ worldBookIds WRITE setWorldBookIds NOTIFY worldBookIdsChanged)
    Q_PROPERTY(QString lastSessionId READ lastSessionId WRITE setLastSessionId NOTIFY lastSessionIdChanged)
    Q_PROPERTY(int cardFormatVersion READ cardFormatVersion WRITE setCardFormatVersion NOTIFY cardFormatVersionChanged)
    Q_PROPERTY(bool isFavorite READ isFavorite WRITE setIsFavorite NOTIFY isFavoriteChanged)
    Q_PROPERTY(QVariantMap extensions READ extensions WRITE setExtensions NOTIFY extensionsChanged)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt NOTIFY createdAtChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt NOTIFY updatedAtChanged)

public:
    explicit CharacterDraft(QObject* parent = nullptr);

    // QML 兼容入口：Repeater 仍可能用字段名动态读写。
    Q_INVOKABLE QVariant fieldValue(const QString& field) const;
    Q_INVOKABLE bool setField(const QString& field, const QVariant& value);

    void setEntity(const CharacterEntity& entity);
    CharacterEntity toEntity() const;

    QString id() const { return m_id; }
    QString name() const { return m_name; }
    QString description() const { return m_description; }
    QString firstMessage() const { return m_firstMessage; }
    QStringList alternateGreetings() const { return m_alternateGreetings; }
    QString creatorNotes() const { return m_creatorNotes; }
    QString creator() const { return m_creator; }
    QString personality() const { return m_personality; }
    QString scenario() const { return m_scenario; }
    QString exampleDialogue() const { return m_exampleDialogue; }
    QString characterVersion() const { return m_characterVersion; }
    QStringList tags() const { return m_tags; }
    QString systemPrompt() const { return m_systemPrompt; }
    QString postHistoryInstructions() const { return m_postHistoryInstructions; }
    QString avatarPath() const { return m_avatarPath; }
    QStringList worldBookIds() const { return m_worldBookIds; }
    QString lastSessionId() const { return m_lastSessionId; }
    int cardFormatVersion() const { return m_cardFormatVersion; }
    bool isFavorite() const { return m_isFavorite; }
    QVariantMap extensions() const { return m_extensions; }
    QDateTime createdAt() const { return m_createdAt; }
    QDateTime updatedAt() const { return m_updatedAt; }

public slots:
    void setId(const QString& value);
    void setName(const QString& value);
    void setDescription(const QString& value);
    void setFirstMessage(const QString& value);
    void setAlternateGreetings(const QStringList& value);
    void setCreatorNotes(const QString& value);
    void setCreator(const QString& value);
    void setPersonality(const QString& value);
    void setScenario(const QString& value);
    void setExampleDialogue(const QString& value);
    void setCharacterVersion(const QString& value);
    void setTags(const QStringList& value);
    void setSystemPrompt(const QString& value);
    void setPostHistoryInstructions(const QString& value);
    void setAvatarPath(const QString& value);
    void setWorldBookIds(const QStringList& value);
    void setLastSessionId(const QString& value);
    void setCardFormatVersion(int value);
    void setIsFavorite(bool value);
    void setExtensions(const QVariantMap& value);
    void setCreatedAt(const QDateTime& value);
    void setUpdatedAt(const QDateTime& value);

signals:
    void changed();
    void idChanged();
    void nameChanged();
    void descriptionChanged();
    void firstMessageChanged();
    void alternateGreetingsChanged();
    void creatorNotesChanged();
    void creatorChanged();
    void personalityChanged();
    void scenarioChanged();
    void exampleDialogueChanged();
    void characterVersionChanged();
    void tagsChanged();
    void systemPromptChanged();
    void postHistoryInstructionsChanged();
    void avatarPathChanged();
    void worldBookIdsChanged();
    void lastSessionIdChanged();
    void cardFormatVersionChanged();
    void isFavoriteChanged();
    void extensionsChanged();
    void createdAtChanged();
    void updatedAtChanged();

private:
    void emitAllChanged();

    bool m_updating = false;
    QString m_id;
    QString m_name;
    QString m_description;
    QString m_firstMessage;
    QStringList m_alternateGreetings;
    QString m_creatorNotes;
    QString m_creator;
    QString m_personality;
    QString m_scenario;
    QString m_exampleDialogue;
    QString m_characterVersion;
    QStringList m_tags;
    QString m_systemPrompt;
    QString m_postHistoryInstructions;
    QString m_avatarPath;
    QStringList m_worldBookIds;
    QString m_lastSessionId;
    int m_cardFormatVersion = 2;
    bool m_isFavorite = false;
    QVariantMap m_extensions;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
};

#endif // CHARACTERDRAFT_H
