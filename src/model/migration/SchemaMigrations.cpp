#include "SchemaMigrations.h"

#include <QSqlError>
#include <QSqlQuery>

namespace {

bool execSql(QSqlDatabase& db, const QString& sql, QString& error)
{
    QSqlQuery query(db);
    if (query.exec(sql)) {
        return true;
    }

    error = QStringLiteral("SQL 执行失败：\n%1\n%2")
                .arg(sql, query.lastError().text());
    return false;
}

} // namespace

QVector<SchemaMigration> SchemaMigrations::all()
{
    return {
        {1, QStringLiteral("baseline_v1"), &SchemaMigrations::applyBaseline}
    };
}

int SchemaMigrations::latestVersion()
{
    const QVector<SchemaMigration> migrations = all();
    return migrations.isEmpty() ? 0 : migrations.last().version;
}

bool SchemaMigrations::applyBaseline(QSqlDatabase& db, QString& error)
{
    const auto run = [&db, &error](const QString& sql) {
        return execSql(db, sql, error);
    };

    // 1. 角色卡
    if (!run(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS characters (
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
        )
    )"))) {
        return false;
    }
    if (!run(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_characters_name ON characters(name)"))) {
        return false;
    }
    if (!run(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_characters_is_favorite ON characters(is_favorite)"))) {
        return false;
    }

    // 2. 聊天会话
    if (!run(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS chat_sessions (
            id TEXT NOT NULL PRIMARY KEY,
            character_id TEXT NOT NULL,
            title TEXT,
            created_at TEXT,
            last_modified TEXT,
            FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE
        )
    )"))) {
        return false;
    }
    if (!run(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_sessions_character_id ON chat_sessions(character_id)"))) {
        return false;
    }

    // 3. 聊天消息
    if (!run(QStringLiteral(R"(
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
        )
    )"))) {
        return false;
    }
    if (!run(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_messages_session_id ON chat_messages(session_id)"))) {
        return false;
    }
    if (!run(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_messages_timestamp ON chat_messages(timestamp)"))) {
        return false;
    }

    // 4. 世界书文集
    if (!run(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS world_info_books (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            is_enabled INTEGER,
            created_at TEXT,
            updated_at TEXT
        )
    )"))) {
        return false;
    }

    // 5. 世界书条目
    if (!run(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS world_info_entries (
            id TEXT PRIMARY KEY,
            book_id TEXT NOT NULL,
            name TEXT,
            keywords TEXT,
            secondary_keywords TEXT,
            content TEXT,
            is_enabled INTEGER NOT NULL DEFAULT 1,
            constant INTEGER NOT NULL,
            vectorized INTEGER NOT NULL,
            position INTEGER NOT NULL,
            role INTEGER,
            insert_depth INTEGER NOT NULL DEFAULT 4,
            sort_order INTEGER NOT NULL,
            use_probability INTEGER NOT NULL DEFAULT 0,
            probability INTEGER NOT NULL DEFAULT 100,
            selective INTEGER NOT NULL DEFAULT 1,
            selective_logic INTEGER NOT NULL DEFAULT 0,
            scan_depth INTEGER NOT NULL DEFAULT -1,
            extensions TEXT,
            FOREIGN KEY (book_id) REFERENCES world_info_books(id) ON DELETE CASCADE
        )
    )"))) {
        return false;
    }
    if (!run(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_entries_book_id ON world_info_entries(book_id)"))) {
        return false;
    }
    if (!run(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_entries_enabled ON world_info_entries(is_enabled)"))) {
        return false;
    }

    // 6. 对话补全预设
    if (!run(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS completion_presets (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            is_default INTEGER DEFAULT 0,
            max_context_token INTEGER DEFAULT 4096,
            max_response_token INTEGER DEFAULT 1024,
            stream_enabled INTEGER DEFAULT 1,
            temperature REAL DEFAULT 0.7,
            top_p REAL DEFAULT 0.9,
            top_k INTEGER DEFAULT 40,
            frequency_penalty REAL DEFAULT 0.0,
            presence_penalty REAL DEFAULT 0.0,
            repetition_penalty REAL DEFAULT 1.1,
            top_a REAL DEFAULT 0.0,
            min_p REAL DEFAULT 0.0,

            extra_settings TEXT,
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
        )
    )"))) {
        return false;
    }
    if (!run(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_presets_default ON completion_presets(is_default)"))) {
        return false;
    }

    // 7. 预设提示词
    if (!run(QStringLiteral(R"(
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
        )
    )"))) {
        return false;
    }
    if (!run(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_prompts_identifier ON completion_presets_prompts(identifier)"))) {
        return false;
    }
    if (!run(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_prompts_preset_id ON completion_presets_prompts(preset_id)"))) {
        return false;
    }

    // 8. 配置
    if (!run(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS settings (
            setting_key TEXT NOT NULL PRIMARY KEY,
            setting_value TEXT,
            value_type INTEGER,
            description TEXT,
            updated_at TEXT
        )
    )"))) {
        return false;
    }

    // 9. 向量记忆
    if (!run(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS vector_memories (
            id TEXT NOT NULL PRIMARY KEY,
            character_id TEXT NOT NULL,
            content TEXT,
            timestamp TEXT,
            importance REAL DEFAULT 0.5,
            embedding_file_path TEXT,
            source_session_id TEXT,
            FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE
        )
    )"))) {
        return false;
    }
    if (!run(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_vector_character_id ON vector_memories(character_id)"))) {
        return false;
    }

    // 10. API 配置
    if (!run(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS api_configs (
            id TEXT NOT NULL PRIMARY KEY,
            name TEXT NOT NULL,
            model_type INTEGER NOT NULL DEFAULT 0,
            is_builtin INTEGER NOT NULL DEFAULT 0,
            is_openai_compatible INTEGER NOT NULL DEFAULT 1,
            base_domain TEXT,
            api_path TEXT,
            api_key TEXT,
            available_models TEXT,
            current_model TEXT,
            extra_headers TEXT,
            extra_params TEXT,
            timeout INTEGER NOT NULL DEFAULT 60000,
            is_active INTEGER NOT NULL DEFAULT 0,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL
        )
    )"))) {
        return false;
    }
    if (!run(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_api_configs_active ON api_configs(is_active)"))) {
        return false;
    }

    return true;
}
