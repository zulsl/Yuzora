#include "DatabaseManager.h"
#include <QDebug>
#include <QThread>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QMutexLocker>

#include "Utils.h"
#include "GlobalConstants.h"


DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::~DatabaseManager()
{
    //数据库单例生命周期和程序进程一致,不需要析构,以下废弃
    // // 仅关闭主线程连接，子线程连接由线程生命周期自动管理
    // QString mainConnName = connectionNameForCurrentThread();
    // if (QSqlDatabase::contains(mainConnName)) {
    //     QSqlDatabase db = QSqlDatabase::database(mainConnName);
    //     if (db.isOpen()) {
    //         db.close();
    //     }
    //     QSqlDatabase::removeDatabase(mainConnName);
    // }
}

bool DatabaseManager::initialize()
{
    if (m_isReady) {
        return true;
    }

// 构建数据库文件路径，确保目录存在
    m_dbFilePath = Utils::getAppDataPath() + QDir::separator() + GlobalConstants::DB_FILE_NAME;
    Utils::ensureDirExists(Utils::getAppDataPath());

// 创建主线程数据库连接
    QSqlDatabase db = getThreadConnection();
    if (!db.isOpen()) {
        qCritical() << "[数据库] 打开失败：" << db.lastError().text();
        return false;
    }

    // 全局开启外键约束
    QSqlQuery pragmaQuery(db);
// pragmaQuery.exec("PRAGMA foreign_keys = ON;");

    // 读取当前数据库版本
    int currentVersion = 0;
    if (pragmaQuery.exec("PRAGMA user_version;") && pragmaQuery.next()) {
        currentVersion = pragmaQuery.value(0).toInt();
    }

// 版本增量升级(预留)
    if (currentVersion < DATABASE_VERSION) {
        if (!upgradeDatabase(db, currentVersion, DATABASE_VERSION)) {
            qCritical() << "[数据库]版本升级失败" << currentVersion;
            return false;
        }
        // 更新数据库版本号
        pragmaQuery.exec(QString("PRAGMA user_version = %1;").arg(DATABASE_VERSION));
    }

// 初始化所有数据表（幂等操作，已存在则跳过）
    if (!createTables(db)) {
        qCritical() << "[数据库] 创建表格失败：" << db.lastError().text();
        return false;
    }

    m_isReady = true;
    qInfo() << "[数据库] 初始化成功，版本：" << DATABASE_VERSION;
    return true;
}

bool DatabaseManager::isReady() const
{
    return m_isReady;
}

QSqlDatabase DatabaseManager::getThreadConnection()
{
    const QString connName = connectionNameForCurrentThread();

    // 判断连接池中是否存在指定名称的数据库连接,连接已存在且已打开，直接返回
    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase db = QSqlDatabase::database(connName);
        if (db.isOpen()) {
            return db;
        }
    }

    // 加锁创建新连接（QSqlDatabase::addDatabase 非线程安全）
    QMutexLocker locker(&m_mutex);

    // 双重检查，避免加锁期间已被其他线程创建
    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase db = QSqlDatabase::database(connName);
        if (db.isOpen()) {
            return db;
        }
    }

    // 创建线程专属连接
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);//添加驱动
    db.setDatabaseName(m_dbFilePath);

    if (!db.open()) {
        qCritical() << "[数据库] 线程创建连接失败：" << connName
                    << "Error:" << db.lastError().text();
        return QSqlDatabase();
    }

    // 每个连接单独开启外键（SQLite默认关闭，每个连接都需要单独开启）
    QSqlQuery query(db);
    query.exec("PRAGMA foreign_keys = ON;");

    qDebug()<< "[数据库] 新连接已创建:" << connName;
    return db;
}

void DatabaseManager::closeAllResources()
{
    QSqlDatabase db = getThreadConnection();
    if (db.isOpen())
        db.close();
}

bool DatabaseManager::beginTransaction()
{
    QSqlDatabase db = getThreadConnection();
    if (!db.isOpen()) {
        return false;
    }
    return db.transaction();
}

bool DatabaseManager::commitTransaction()
{
    QSqlDatabase db = getThreadConnection();
    if (!db.isOpen()) {
        return false;
    }
    return db.commit();
}

bool DatabaseManager::rollbackTransaction()
{
    QSqlDatabase db = getThreadConnection();
    if (!db.isOpen()) {
        return false;
    }
    return db.rollback();
}

QSqlError DatabaseManager::executeQuery(const QString& sql, const QVariantMap& bindValues)
{
    QSqlDatabase db = getThreadConnection();
    if (!db.isOpen()) {
        return db.lastError();
    }

    QSqlQuery query(db);
    if (!query.prepare(sql)) {
        return query.lastError();
    }

    // 绑定命名参数
    for (auto it = bindValues.constBegin(); it != bindValues.constEnd(); ++it) {
        query.bindValue(":" + it.key(), it.value());
    }

    if (!query.exec()) {
        return query.lastError();
    }

    return QSqlError();
}

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
{
}

bool DatabaseManager::createTables(QSqlDatabase& db)
{
    QSqlQuery query(db);

    // 1.角色卡表
    const QString createCharacters = R"(
        CREATE TABLE IF NOT EXISTS characters (
            id TEXT NOT NULL PRIMARY KEY,
            name TEXT NOT NULL,
            description TEXT DEFAULT '',
            first_message TEXT DEFAULT '',
            alternate_greetings TEXT DEFAULT '[]',  -- JSON 数组格式
            creator_notes TEXT DEFAULT '',
            creator TEXT DEFAULT '',
            personality TEXT DEFAULT '',
            scenario TEXT DEFAULT '',
            example_dialogue TEXT DEFAULT '',
            system_prompt TEXT DEFAULT '',
            character_version TEXT DEFAULT '1.0.0',
            tags TEXT DEFAULT '[]',                 -- JSON 数组格式
            card_format_version INTEGER DEFAULT 2,
            avatar_path TEXT DEFAULT '',
            world_book_ids TEXT DEFAULT '[]',       -- JSON 数组格式
            last_session_id TEXT,
            is_favorite INTEGER DEFAULT 0,          -- SQLite 用 0/1 表示布尔值
            extensions TEXT DEFAULT '{}',           -- JSON 对象格式
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL
        );
    )";
    if (!query.exec(createCharacters)) return false;
    query.exec("CREATE INDEX IF NOT EXISTS idx_characters_name ON characters(name);");
    query.exec("CREATE INDEX IF NOT EXISTS idx_characters_is_favorite ON characters(is_favorite);");


    // 2.聊天会话表
    const QString createSessions = R"(
        CREATE TABLE IF NOT EXISTS chat_sessions (
            id TEXT NOT NULL PRIMARY KEY,
            character_id TEXT NOT NULL,
            title TEXT,
            created_at TEXT,
            last_modified TEXT,
            FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE
        );
    )";
    if (!query.exec(createSessions)) return false;
    query.exec("CREATE INDEX IF NOT EXISTS idx_sessions_character_id ON chat_sessions(character_id);");

    // 3.聊天消息表
    const QString createMessages = R"(
        CREATE TABLE IF NOT EXISTS chat_messages (
            id TEXT NOT NULL PRIMARY KEY,
            session_id TEXT NOT NULL,
            message_type INTEGER NOT NULL,
            status INTEGER NOT NULL,
            content TEXT,
            sender_name TEXT,
            timestamp TEXT,
            metadata TEXT,
            is_edited INTEGER DEFAULT 0,
            is_hidden INTEGER DEFAULT 0,
            FOREIGN KEY (session_id) REFERENCES chat_sessions(id) ON DELETE CASCADE
        );
    )";
    if (!query.exec(createMessages)) return false;
    query.exec("CREATE INDEX IF NOT EXISTS idx_messages_session_id ON chat_messages(session_id);");
    query.exec("CREATE INDEX IF NOT EXISTS idx_messages_timestamp ON chat_messages(timestamp);");

    // 4.世界书文集表
    const QString createWorldBooks = R"(
        CREATE TABLE IF NOT EXISTS world_info_books (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            is_enabled INTEGER,
            created_at TEXT,
            updated_at TEXT
        );
    )";
    if (!query.exec(createWorldBooks)) return false;

    // 5.世界书条目表
    const QString createWorldEntries = R"(
        CREATE TABLE IF NOT EXISTS world_info_entries (
            id TEXT PRIMARY KEY,
            book_id TEXT NOT NULL,
            name TEXT,                       -- 条目标识名称
            keywords TEXT,                   -- 主关键词 JSON数组
            secondary_keywords TEXT,         -- 二级关键词 JSON数组
            content TEXT,                    -- 注入的正文内容
            constant INTEGER NOT NULL,       -- 触发策略
            vectorized INTEGER NOT NULL,     -- 向量化
            position INTEGER NOT NULL,       -- 插入位置
            role INTEGER,                    -- 在什么角色插入
            sort_order INTEGER NOT NULL,     -- 顺序
            extensions TEXT,                 -- 扩展字段 JSON
            FOREIGN KEY (book_id) REFERENCES world_info_books(id) ON DELETE CASCADE
        );
    )";
    if (!query.exec(createWorldEntries)) return false;
    query.exec("CREATE INDEX IF NOT EXISTS idx_entries_book_id ON world_info_entries(book_id);");
    query.exec("CREATE INDEX IF NOT EXISTS idx_entries_enabled ON world_info_entries(is_enabled);");


    // 对话补全预设表
    const QString createPresets = R"(
        CREATE TABLE IF NOT EXISTS completion_presets (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            is_default INTEGER DEFAULT 0,
            max_context_token INTEGER DEFAULT 4096,
            max_response_token INTEGER DEFAULT 1024,
            stream_enabled INTEGER DEFAULT 1,
            temperature REAL DEFAULT 0.7,
            top_p REAL DEFAULT 0.9,
            top_k REAL DEFAULT 40,
            frequency_penalty REAL DEFAULT 0.0,
            presence_penalty REAL DEFAULT 0.0,
            repetition_penalty REAL DEFAULT 1.1,
            top_a REAL DEFAULT 0.0,
            min_p REAL DEFAULT 0.0,

            extra_settings TEXT, -- JSON对象，预留扩展
            created_at TEXT,
            updated_at TEXT,

            max_context_unlocked INTEGER,
            names_behavior INTEGER,
            continue_postfix TEXT,
            continue_prefill INTEGER,
            squash_system_messages INTEGER,
            function_calling INTEGER,
            show_thoughts INTEGER,
            prompt_order TEXT,
            impersonation_Prompt TEXT,
            wi_Format TEXT,
            scenario_Format TEXT,
            personality_Format TEXT,
            group_nudge_prompt TEXT,
            new_chat_prompt TEXT,
            new_group_chat_prompt TEXT,
            new_example_chat_prompt TEXT,
            continue_nudge_prompt TEXT
        );
    )";
    if (!query.exec(createPresets)) return false;
    query.exec("CREATE INDEX IF NOT EXISTS idx_presets_default ON completion_presets(is_default);");

    //提示词表
    const QString createPrompts = R"(
        CREATE TABLE IF NOT EXISTS completion_presets_prompts (
            id TEXT PRIMARY KEY,
            identifier TEXT NOT NULL,
            preset_id TEXT NOT NULL,
            name TEXT,
            is_active INTEGER NOT NULL,
            role TEXT,
            content TEXT,
            system_prompt INTEGER,
            marker INTEGER,
            injection_position INTEGER,
            injection_depth INTEGER,
            injection_order INTEGER,
            forbid_overrides INTEGER
        );
    )";
    if (!query.exec(createPrompts)) return false;
    query.exec("CREATE INDEX IF NOT EXISTS idx_prompts_identifier ON completion_presets_prompts(identifier);");
    query.exec("CREATE INDEX IF NOT EXISTS idx_prompts_preset_id ON completion_presets_prompts(preset_id);");


    // 配置表
    const QString createSettings = R"(
        CREATE TABLE IF NOT EXISTS settings (
            setting_key TEXT NOT NULL PRIMARY KEY,
            setting_value TEXT,
            value_type INTEGER,
            description TEXT,
            updated_at TEXT
        );
    )";
    if (!query.exec(createSettings)) return false;

    // 8. 向量记忆表
    const QString createVectorMemory = R"(
        CREATE TABLE IF NOT EXISTS vector_memories (
            id TEXT NOT NULL PRIMARY KEY,
            character_id TEXT NOT NULL,
            content TEXT,
            timestamp TEXT,
            importance REAL DEFAULT 0.5,
            embedding_file_path TEXT,
            source_session_id TEXT,
            FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE
        );
    )";
    if (!query.exec(createVectorMemory)) return false;
    query.exec("CREATE INDEX IF NOT EXISTS idx_vector_character_id ON vector_memories(character_id);");

    //api配置表
    const QString apiConfigs = R"(
        CREATE TABLE IF NOT EXISTS api_configs (
            id TEXT NOT NULL PRIMARY KEY,
            name TEXT NOT NULL,
            model_type INTEGER NOT NULL DEFAULT 0,
            is_builtin INTEGER NOT NULL DEFAULT 0,
            is_openai_compatible INTEGER NOT NULL DEFAULT 1,
            base_domain TEXT,
            api_path TEXT,
            api_key TEXT,
            available_models TEXT, -- JSON数组存储
            current_model TEXT,
            extra_headers TEXT,    -- JSON对象存储
            extra_params TEXT,    -- JSON对象存储
            timeout INTEGER NOT NULL DEFAULT 60000,
            is_active INTEGER NOT NULL DEFAULT 0,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL
        );
    )";
    if (!query.exec(apiConfigs)) return false;
    query.exec("CREATE INDEX IF NOT EXISTS idx_api_configs_active ON api_configs(is_active);");

    return true;
}

bool DatabaseManager::upgradeDatabase(QSqlDatabase& db, int oldVersion, int newVersion)
{
    Q_UNUSED(db);
    Q_UNUSED(oldVersion);
    Q_UNUSED(newVersion);

    // 当前为初始版本1，无需增量升级
    // 后续版本迭代时，在此处按版本号依次执行 ALTER TABLE 等迁移语句
    // 示例：if (oldVersion < 2) { 执行v1→v2升级SQL }
    return true;
}

QString DatabaseManager::connectionNameForCurrentThread() const
{
    // 以线程ID哈希作为连接名，保证每个线程唯一
    return QString("db_conn_%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
}