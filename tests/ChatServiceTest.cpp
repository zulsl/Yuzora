#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include <QUuid>

#include "ChatService.h"
#include "SessionService.h"
#include "TransactionScope.h"
#include "ChatSessionDao.h"
#include "ChatMessageDao.h"
#include "CharacterDao.h"
#include "SettingsDao.h"
#include "CharacterService.h"
#include "SettingsService.h"
#include "ConnectionProviderRegistry.h"
#include "IConnectionProvider.h"
#include "Utils.h"
#include "AppException.h"
#include "DatabaseManager.h"
#include "WorldInfoService.h"
#include "ChatMessage.h"
#include "completion/ChatCompletionRunner.h"
#include "greeting/GreetingService.h"
#include "prompt/ChatPromptContext.h"

// ======================================
// 链接桩
// ======================================
// 本测试只跑会话 / 消息 / 开场白 / 生成落库，下面这些依赖不在测试路径上执行，
// 提供符号即可（与 CharacterServiceTest 的做法一致）。
void Utils::writeLog(const QString&, const QString&, const QString&) {}

QString Utils::stringListToJson(const QStringList& values)
{
    return QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(values))
                                 .toJson(QJsonDocument::Compact));
}

QStringList Utils::jsonToStringList(const QVariant& value)
{
    QStringList result;
    const QJsonDocument doc = QJsonDocument::fromJson(value.toString().toUtf8());
    for (const QJsonValue& item : doc.array())
        result.append(item.toString());
    return result;
}

QString Utils::variantMapToJson(const QVariantMap& map)
{
    return QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(map))
                                 .toJson(QJsonDocument::Compact));
}

QVariantMap Utils::jsonToVariantMap(const QVariant& value)
{
    return QJsonDocument::fromJson(value.toString().toUtf8()).object().toVariantMap();
}

QString Utils::getAvatarDirPath() { return QStringLiteral("."); }
QJsonObject Utils::extractPngMetadata(const QString&) { return {}; }
bool Utils::injectPngMetadata(const QString&, const QJsonObject&) { return false; }
QJsonDocument Utils::readJsonFile(const QString&) { return {}; }
bool Utils::writeJsonFile(const QString&, const QJsonDocument&) { return false; }

AppException::AppException(ErrorCode code, const QString& message)
    : m_code(code), m_message(message), m_whatBuffer(message.toUtf8()) {}
const char* AppException::what() const noexcept { return m_whatBuffer.constData(); }
QString AppException::getMessage() const { return m_message; }

DatabaseManager& DatabaseManager::instance()
{
    qFatal("DatabaseManager::instance() should not be called in this test");
    static DatabaseManager* invalid = nullptr;
    return *invalid;
}
bool DatabaseManager::beginTransaction() { return false; }
bool DatabaseManager::commitTransaction() { return false; }
bool DatabaseManager::rollbackTransaction() { return false; }

WorldInfoService& WorldInfoService::instance()
{
    qFatal("WorldInfoService::instance() should not be called in this test");
    static WorldInfoService* invalid = nullptr;
    return *invalid;
}
QString WorldInfoService::importWorldBook(QJsonObject&) { return {}; }

// 提示词装配整块替换：本测试不校验提示词内容，返回空列表即可。
// 正因为 buildPromptContext 已经被抽成自由函数，这里才能这么替换。
QList<ChatMessage> buildChatPromptContext(
        const QString&, const QString&, const QString&, ChatMessageDao*, CharacterService*,
        const QString&)
{
    return {};
}

// ======================================
// 测试替身
// ======================================
// 假补全器：记录请求，并允许测试主动驱动"完成 / 失败"
class FakeCompletionRunner : public ChatCompletionRunner
{
    Q_OBJECT
public:
    using ChatCompletionRunner::ChatCompletionRunner;

    void start(const QList<ChatMessage>& messages) override
    {
        requests.append(messages);
        running = true;
        starts += 1;
    }
    void cancel() override
    {
        running = false;
        cancels += 1;
    }
    bool isRunning() const override { return running; }

    void completeWith(const QString& text)
    {
        running = false;
        emit finished(text);
    }
    void failWith(const QString& error)
    {
        running = false;
        emit failed(error);
    }

    QList<QList<ChatMessage>> requests;
    bool running = false;
    int starts = 0;
    int cancels = 0;
};

// 假事务作用域：只记录调用次数，不做真事务
class FakeTransactionScope : public TransactionScope
{
public:
    bool begin() override { ++begins; return true; }
    bool commit() override { ++commits; return true; }
    void rollback() override { ++rollbacks; }

    int begins = 0;
    int commits = 0;
    int rollbacks = 0;
};

// ======================================
// 内存库
// ======================================
class ChatServiceTestProvider : public IConnectionProvider
{
public:
    ChatServiceTestProvider()
    {
        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                       QStringLiteral("chat-service-test"));
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());
        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral(R"(
            CREATE TABLE characters (
                id TEXT NOT NULL PRIMARY KEY,
                name TEXT NOT NULL,
                description TEXT DEFAULT '',
                first_message TEXT DEFAULT '',
                alternate_greetings TEXT DEFAULT '[]',
                creator_notes TEXT DEFAULT '',
                creator TEXT DEFAULT '',
                personality TEXT DEFAULT '',
                scenario TEXT DEFAULT '',
                example_dialogue TEXT DEFAULT '',
                system_prompt TEXT DEFAULT '',
                post_history_instructions TEXT DEFAULT '',
                character_version TEXT DEFAULT '1.0.0',
                tags TEXT DEFAULT '[]',
                card_format_version INTEGER DEFAULT 2,
                avatar_path TEXT DEFAULT '',
                world_book_ids TEXT DEFAULT '[]',
                last_session_id TEXT,
                is_favorite INTEGER DEFAULT 0,
                extensions TEXT DEFAULT '{}',
                created_at TEXT NOT NULL,
                updated_at TEXT NOT NULL
            ))")));
        QVERIFY(query.exec(QStringLiteral(R"(
            CREATE TABLE chat_sessions (
                id TEXT NOT NULL PRIMARY KEY,
                character_id TEXT NOT NULL,
                title TEXT,
                created_at TEXT,
                last_modified TEXT
            ))")));
        QVERIFY(query.exec(QStringLiteral(R"(
            CREATE TABLE chat_messages (
                id TEXT NOT NULL PRIMARY KEY,
                session_id TEXT NOT NULL,
                message_type INTEGER NOT NULL,
                status INTEGER NOT NULL,
                content TEXT,
                sender_name TEXT,
                timestamp TEXT,
                metadata TEXT,
                is_edited INTEGER DEFAULT 0,
                is_hidden INTEGER DEFAULT 0
            ))")));
        QVERIFY(query.exec(QStringLiteral(
            "CREATE TABLE settings ("
            "setting_key TEXT PRIMARY KEY, setting_value TEXT, value_type INTEGER,"
            "description TEXT, updated_at TEXT)")));
    }

    ~ChatServiceTestProvider() override
    {
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(QStringLiteral("chat-service-test"));
    }

    QSqlDatabase connection() override { return db; }

private:
    QSqlDatabase db;
};

// ======================================
// 夹具：ChatService 的全部协作者都指向内存库 / 测试替身
// ======================================
struct Fixture
{
    ChatServiceTestProvider provider;
    ChatSessionDao sessionDao;
    ChatMessageDao messageDao;
    CharacterDao characterDao;
    SettingsDao settingsDao;
    CharacterService characterService;
    SettingsService settingsService;
    SessionService sessions;
    FakeCompletionRunner completion;
    FakeTransactionScope transaction;
    GreetingService greeting;
    ChatService chatService;

    Fixture()
        : characterService(&characterDao, static_cast<WorldInfoService*>(nullptr))
        , settingsService(&settingsDao)
        , sessions(&sessionDao, &characterDao, &settingsService)
        , greeting(&messageDao, &characterService)
        , chatService(ChatServiceDependencies{&sessions, &messageDao, &characterDao,
                                              &characterService, &settingsService,
                                              &completion, &transaction, &greeting})
    {
        sessionDao.setConnectionProvider(&provider);
        messageDao.setConnectionProvider(&provider);
        characterDao.setConnectionProvider(&provider);
        settingsDao.setConnectionProvider(&provider);
    }

    CharacterEntity makeCharacter(const QString& firstMessage,
                                  const QStringList& alternateGreetings)
    {
        CharacterEntity entity;
        entity.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        entity.name = QStringLiteral("测试角色");
        entity.firstMessage = firstMessage;
        entity.alternateGreetings = alternateGreetings;
        entity.createdAt = QDateTime::currentDateTime();
        entity.updatedAt = entity.createdAt;
        return entity;
    }
};

class ChatServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void createSessionInsertsGreetingAsFirstMessage();
    void switchSessionExposesGreetingState();
    void characterWithoutGreetingStartsEmpty();
    void greetingSwitchesAndWrapsAround();
    void tiedTimestampsKeepInsertionOrder();
    void deleteCurrentSessionFallsBackToRemaining();
    void renameSessionTrimsAndClamps();
    void sendMessageAppendsUserBeforeAssistantAndStartsGeneration();
    void finishingGenerationWritesContentAndAutoTitles();
    void failingGenerationMarksMessageFailed();
    void sessionSwitchPersistsLastActiveCharacterAndSession();
    void autoTitleSkipsRenamedSession();
};

// ======================================
// 用例
// ======================================
void ChatServiceTest::createSessionInsertsGreetingAsFirstMessage()
{
    Fixture f;
    const CharacterEntity character =
        f.makeCharacter(QStringLiteral("默认开场白"), {QStringLiteral("备选一"),
                                                    QStringLiteral("备选二")});
    QVERIFY(f.characterDao.insert(character));

    const QString sessionId = f.chatService.createSession(character.id);
    QVERIFY(!sessionId.isEmpty());

    const QList<ChatMessageEntity> messages = f.chatService.loadLatestMessages(sessionId, 10);
    QCOMPARE(messages.size(), 1);
    QCOMPARE(messages.first().content, QStringLiteral("默认开场白"));
    QCOMPARE(static_cast<int>(messages.first().type),
             static_cast<int>(GlobalConstants::MessageType::Assistant));
    QCOMPARE(messages.first().metadata.value(QStringLiteral("greetingIndex")).toInt(), 0);
}

void ChatServiceTest::switchSessionExposesGreetingState()
{
    Fixture f;
    const CharacterEntity character =
        f.makeCharacter(QStringLiteral("默认开场白"), {QStringLiteral("备选一"),
                                                    QStringLiteral("备选二")});
    QVERIFY(f.characterDao.insert(character));

    const QString sessionId = f.chatService.createSession(character.id);
    f.chatService.switchSession(sessionId);

    QCOMPARE(f.chatService.greetingCount(), 3);
    QCOMPARE(f.chatService.greetingIndex(), 0);
    QVERIFY(!f.chatService.greetingMessageId().isEmpty());

    // 切到下一条：承载消息就地换内容，序号跟着走（不新增消息）
    f.chatService.switchGreeting(1);
    QCOMPARE(f.chatService.greetingIndex(), 1);

    const QList<ChatMessageEntity> messages = f.chatService.loadLatestMessages(sessionId, 10);
    QCOMPARE(messages.size(), 1);
    QCOMPARE(messages.first().content, QStringLiteral("备选一"));
    QCOMPARE(messages.first().metadata.value(QStringLiteral("greetingIndex")).toInt(), 1);
}

void ChatServiceTest::characterWithoutGreetingStartsEmpty()
{
    Fixture f;
    const CharacterEntity character = f.makeCharacter(QString(), {});
    QVERIFY(f.characterDao.insert(character));

    const QString sessionId = f.chatService.createSession(character.id);
    QVERIFY(!sessionId.isEmpty());
    QVERIFY(f.chatService.loadLatestMessages(sessionId, 10).isEmpty());

    f.chatService.switchSession(sessionId);
    QCOMPARE(f.chatService.greetingCount(), 0);
    QCOMPARE(f.chatService.greetingIndex(), -1);
    QVERIFY(f.chatService.greetingMessageId().isEmpty());
}

// 切换：+1 前进、-1 从第 0 条回绕到最后一条，内容就地替换
void ChatServiceTest::greetingSwitchesAndWrapsAround()
{
    Fixture f;
    const CharacterEntity character =
        f.makeCharacter(QStringLiteral("默认开场白"), {QStringLiteral("备选一"),
                                                    QStringLiteral("备选二")});
    QVERIFY(f.characterDao.insert(character));
    const QString sessionId = f.chatService.createSession(character.id);
    f.chatService.switchSession(sessionId);
    QCOMPARE(f.chatService.greetingIndex(), 0);

    f.chatService.switchGreeting(-1);
    QCOMPARE(f.chatService.greetingIndex(), 2);
    QList<ChatMessageEntity> messages = f.chatService.loadLatestMessages(sessionId, 10);
    QCOMPARE(messages.size(), 1);
    QCOMPARE(messages.first().content, QStringLiteral("备选二"));

    f.chatService.switchGreeting(1);
    QCOMPARE(f.chatService.greetingIndex(), 0);
    messages = f.chatService.loadLatestMessages(sessionId, 10);
    QCOMPARE(messages.first().content, QStringLiteral("默认开场白"));
}

// 回归：时间戳只有秒精度，用户消息和它的助手占位消息必然同秒，
// 排序必须靠 rowid（插入顺序），否则重启后 AI 回复会跑到用户消息上面。
void ChatServiceTest::tiedTimestampsKeepInsertionOrder()
{
    Fixture f;
    const CharacterEntity character = f.makeCharacter(QString(), {});
    QVERIFY(f.characterDao.insert(character));
    const QString sessionId = f.chatService.createSession(character.id);
    f.chatService.switchSession(sessionId);

    const QString sameTimestamp = QStringLiteral("2026-01-01T00:00:00");

    ChatMessageEntity userMessage;
    userMessage.id = Utils::generateUuid();
    userMessage.sessionId = sessionId;
    userMessage.type = GlobalConstants::MessageType::User;
    userMessage.status = GlobalConstants::MessageGenerationStatus::Normal;
    userMessage.content = QStringLiteral("用户先说的话");
    userMessage.timestamp = QDateTime::fromString(sameTimestamp, Qt::ISODate);
    QVERIFY(f.messageDao.insert(userMessage));

    ChatMessageEntity assistantMessage;
    assistantMessage.id = Utils::generateUuid();
    assistantMessage.sessionId = sessionId;
    assistantMessage.type = GlobalConstants::MessageType::Assistant;
    assistantMessage.status = GlobalConstants::MessageGenerationStatus::Normal;
    assistantMessage.content = QStringLiteral("AI 的回复");
    assistantMessage.timestamp = userMessage.timestamp;
    QVERIFY(f.messageDao.insert(assistantMessage));

    const QList<ChatMessageEntity> messages = f.chatService.loadLatestMessages(sessionId, 10);
    QCOMPARE(messages.size(), 2);
    QCOMPARE(messages.first().id, userMessage.id);
    QCOMPARE(messages.last().id, assistantMessage.id);

    QCOMPARE(f.messageDao.selectFirstBySessionId(sessionId).id, userMessage.id);
    QCOMPARE(f.messageDao.selectLastBySessionId(sessionId).id, assistantMessage.id);
}

void ChatServiceTest::deleteCurrentSessionFallsBackToRemaining()
{
    Fixture f;
    const CharacterEntity character = f.makeCharacter(QString(), {});
    QVERIFY(f.characterDao.insert(character));

    const QString keptSessionId = f.chatService.createSession(character.id);
    const QString removedSessionId = f.chatService.createSession(character.id);
    f.chatService.switchSession(removedSessionId);

    f.chatService.deleteSession(removedSessionId);
    QCOMPARE(f.chatService.currentSessionId(), keptSessionId);
}

void ChatServiceTest::renameSessionTrimsAndClamps()
{
    Fixture f;
    const CharacterEntity character = f.makeCharacter(QString(), {});
    QVERIFY(f.characterDao.insert(character));
    const QString sessionId = f.chatService.createSession(character.id);
    f.chatService.switchSession(sessionId);

    QVERIFY(f.chatService.renameSession(sessionId, QStringLiteral("  新标题  ")));
    QCOMPARE(f.chatService.currentSession().title, QStringLiteral("新标题"));

    const QString longTitle(60, QLatin1Char('长'));
    QVERIFY(f.chatService.renameSession(sessionId, longTitle));
    QCOMPARE(f.chatService.currentSession().title.size(), 40);

    // 空白标题直接拒绝，标题保持不变
    QVERIFY(!f.chatService.renameSession(sessionId, QStringLiteral("   ")));
    QCOMPARE(f.chatService.currentSession().title.size(), 40);
}

// 发送：用户消息必须排在助手占位消息之前（同一事务、同一秒），并且请求交给了网关
void ChatServiceTest::sendMessageAppendsUserBeforeAssistantAndStartsGeneration()
{
    Fixture f;
    const CharacterEntity character = f.makeCharacter(QString(), {});
    QVERIFY(f.characterDao.insert(character));
    const QString sessionId = f.chatService.createSession(character.id);
    f.chatService.switchSession(sessionId);

    f.chatService.sendMessage(QStringLiteral("你好"));

    const QList<ChatMessageEntity> messages = f.chatService.loadLatestMessages(sessionId, 10);
    QCOMPARE(messages.size(), 2);
    QCOMPARE(static_cast<int>(messages.first().type),
             static_cast<int>(GlobalConstants::MessageType::User));
    QCOMPARE(messages.first().content, QStringLiteral("你好"));
    QCOMPARE(static_cast<int>(messages.last().type),
             static_cast<int>(GlobalConstants::MessageType::Assistant));
    QCOMPARE(static_cast<int>(messages.last().status),
             static_cast<int>(GlobalConstants::MessageGenerationStatus::Generating));

    // 事务成对出现，生成请求只发一次
    QCOMPARE(f.transaction.begins, 1);
    QCOMPARE(f.transaction.commits, 1);
    QCOMPARE(f.completion.starts, 1);
    QVERIFY(f.chatService.isGenerating());
}

// 完成：内容落库、状态转 Normal、会话标题按前 10 个字符自动生成
void ChatServiceTest::finishingGenerationWritesContentAndAutoTitles()
{
    Fixture f;
    const CharacterEntity character = f.makeCharacter(QString(), {});
    QVERIFY(f.characterDao.insert(character));
    const QString sessionId = f.chatService.createSession(character.id);
    f.chatService.switchSession(sessionId);
    QCOMPARE(f.chatService.currentSession().title, QStringLiteral("新对话"));

    f.chatService.sendMessage(QStringLiteral("你好"));
    f.completion.completeWith(QStringLiteral("这是AI的回复内容"));

    const QList<ChatMessageEntity> messages = f.chatService.loadLatestMessages(sessionId, 10);
    QCOMPARE(messages.size(), 2);
    QCOMPARE(messages.last().content, QStringLiteral("这是AI的回复内容"));
    QCOMPARE(static_cast<int>(messages.last().status),
             static_cast<int>(GlobalConstants::MessageGenerationStatus::Normal));

    QCOMPARE(f.chatService.currentSession().title, QStringLiteral("这是AI的回复内容..."));
    QVERIFY(!f.chatService.isGenerating());
}

// 失败：状态转 Failed，标题保持默认，生成中标志复位
void ChatServiceTest::failingGenerationMarksMessageFailed()
{
    Fixture f;
    const CharacterEntity character = f.makeCharacter(QString(), {});
    QVERIFY(f.characterDao.insert(character));
    const QString sessionId = f.chatService.createSession(character.id);
    f.chatService.switchSession(sessionId);

    f.chatService.sendMessage(QStringLiteral("你好"));
    f.completion.failWith(QStringLiteral("网络错误"));

    const QList<ChatMessageEntity> messages = f.chatService.loadLatestMessages(sessionId, 10);
    QCOMPARE(messages.size(), 2);
    QCOMPARE(static_cast<int>(messages.last().status),
             static_cast<int>(GlobalConstants::MessageGenerationStatus::Failed));
    QCOMPARE(f.chatService.currentSession().title, QStringLiteral("新对话"));
    QVERIFY(!f.chatService.isGenerating());
}

// 切换会话要写回两处：角色卡的 last_session_id + 设置里的最后角色
void ChatServiceTest::sessionSwitchPersistsLastActiveCharacterAndSession()
{
    Fixture f;
    const CharacterEntity character = f.makeCharacter(QString(), {});
    QVERIFY(f.characterDao.insert(character));

    const QString sessionId = f.chatService.createSession(character.id);
    // create 只插会话行，不改变当前会话
    QVERIFY(f.chatService.currentSessionId().isEmpty());

    f.chatService.switchSession(sessionId);
    QCOMPARE(f.chatService.currentSessionId(), sessionId);
    QCOMPARE(f.chatService.currentCharacterId(), character.id);

    QCOMPARE(f.characterDao.selectById(character.id).lastSessionId, sessionId);
    QCOMPARE(f.settingsService.getString(GlobalConstants::LAST_ACTIVE_CHARACTER_ID),
             character.id);
}

// 用户改过名的会话，生成结束时不能被自动标题覆盖
void ChatServiceTest::autoTitleSkipsRenamedSession()
{
    Fixture f;
    const CharacterEntity character = f.makeCharacter(QString(), {});
    QVERIFY(f.characterDao.insert(character));
    const QString sessionId = f.chatService.createSession(character.id);
    f.chatService.switchSession(sessionId);

    QVERIFY(f.chatService.renameSession(sessionId, QStringLiteral("我的标题")));
    f.chatService.sendMessage(QStringLiteral("你好"));
    f.completion.completeWith(QStringLiteral("这是AI的回复内容"));

    QCOMPARE(f.chatService.currentSession().title, QStringLiteral("我的标题"));
}

QTEST_GUILESS_MAIN(ChatServiceTest)
#include "ChatServiceTest.moc"
