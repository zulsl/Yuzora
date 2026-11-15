#ifndef GLOBALCONSTANTS_H
#define GLOBALCONSTANTS_H

#include <QObject>
#include <QString>
#include <QVersionNumber>
#include <QtQml/qqmlregistration.h>

namespace GlobalConstants
{
Q_NAMESPACE
// 供 QML 使用：把本命名空间的枚举注册进 Yuzora 模块，
// QML 侧可直接写 GlobalConstants.User / GlobalConstants.Assistant，
// 不再用裸数字 0/1/2 判断消息角色。
QML_ELEMENT

// ======================================
// 通用业务常量
// ======================================
    // 应用基础信息
    //Q_GLOBAL_STATIC替换const QString APP_NAME = "Yuzora";更好的全局静态变量
    const QString APP_NAME = "Yuzora";
    const QVersionNumber APP_VERSION = QVersionNumber(2, 0, 0);

    //数据库常量
    const QString DB_FILE_NAME = "Yuzora_data.db";

    //头像目录
    const QString AVATARS_DIR = "avatars";

    // 网络Token常量
    const int API_TIMEOUT = 60000;
    const int DEFAULT_MAX_TOKEN = 8192;
    const int DEFAULT_CONTEXT_TOKEN = 4096;
    const int DEFAULT_RESPONSE_TOKEN = 1024;

    // 角色卡版本规范
    const QString CHARACTER_CARD_EXT = "png";
    const QString CHARACTER_CARD_META_KEY = "chara";
    // Q_GLOBAL_STATIC(QString,CHARACTER_CARD_EXT,"png")
    const int CHARACTER_CARD_V1 = 1;
    const int CHARACTER_CARD_V2 = 2;
    const int CHARACTER_CARD_V3 = 3;



    //用户设置
    inline const QString USER_AVATAR = "user.user_acatar";
    inline const QString USER_NAME = "user.user_name";

    // 外观设置
    inline const QString THEME_MODE = "appearance.theme_mode";          // light/dark/system
    inline const QString FONT_SIZE = "appearance.base_font_size";            // 全局字体大小
    inline const QString MESSAGE_BUBBLE_STYLE = "appearance.bubble_style"; // 气泡样式
    inline const QString THEME_COLOR = "appearance.theme_color";  // 主题颜色
    // 通用设置
    inline const QString AUTO_SAVE_INTERVAL = "general.auto_save_interval"; // 自动保存间隔(秒)
    inline const QString SEND_WITH_ENTER = "general.send_with_enter";      // 回车发送开关
    inline const QString SHOW_TIME_STAMP = "general.show_time_stamp";      // 显示消息时间
    inline const QString AUTO_SCROLL_TO_BOTTOM = "general.auto_scroll";    // 自动滚动到底部
    // 网络设置
    inline const QString ENABLE_PROXY = "network.enable_proxy";            // 代理开关
    inline const QString PROXY_TYPE = "network.proxy_type";                // http/socks5
    inline const QString PROXY_HOST = "network.proxy_host";                // 代理地址
    inline const QString PROXY_PORT = "network.proxy_port";                // 代理端口
    inline const QString REQUEST_TIMEOUT = "network.request_timeout";      // 请求超时(秒)
    // 存储设置(废弃)
    inline const QString DATA_STORAGE_PATH = "storage.data_path";          // 数据存储目录
    // 聊天设置
    inline const QString DEFAULT_PRESET_ID = "chat.default_preset_id";     // 默认生成预设
    inline const QString AUTO_CONTINUE_GENERATE = "chat.auto_continue";    // 自动续写
    inline const QString MAX_HISTORY_LOAD = "chat.max_history_load";       // 默认加载历史条数
    // 聊天状态持久化
    inline const QString LAST_ACTIVE_CHARACTER_ID = "chat.last_character_id";




// ======================================
// 枚举定义
// ======================================
    // AI接口格式枚举
    enum class ModelType {
        OpenAI,//openai兼容类型
        ChatCompletions,
        Responses,
        AnthropicMessages,
        Gemini,
        DeepSeek,
        Claude,
        Unknown
    };
    Q_ENUM_NS(ModelType)

    // 聊天消息类型枚举
    enum MessageType {//聊天角色
        System = 0,         //系统提示词
        User,           //用户
        Assistant,      //ai回复
        ToolCall,       //工具调用
        ToolResult,     //工具结果
        Notice,          //通知
        Other
    };
    Q_ENUM_NS(MessageType)

    enum class InjectionPosition {
        Relative,   // 相对位置（顺序插入）
        Absolute    // 绝对位置（在聊天历史中按深度插入）
    };

    // 消息生成状态枚举
    enum class MessageGenerationStatus {
        Normal,         // 正常已完成
        Generating,     // 生成中
        Failed,         // 生成失败
        Canceled        // 已取消
    };
    Q_ENUM_NS(MessageGenerationStatus)

    // 主题类型枚举
    enum class ThemeType {
        Default,
        Light,
        Dark,
        Custom
    };
    Q_ENUM_NS(ThemeType)

    // 提示词模板类型枚举
    enum class PromptTemplateType {
        Alpaca,     //Alpaca 模型专用提示词格式(老旧 7B 老模型)
        ChatML,     //OpenAI 通用聊天模板格式(包括:通义千问、Qwen、文心一言、智谱、DeepSeek、百川)
        Llama2,     //Meta LLaMA 2 模型专用格式(老式)
        Mistral,    //Mistral 模型专用格式
        GeminiPrompt,     //Gemini
        CustomPrompt      //其他格式
    };

    // 记忆类型枚举
    enum class MemoryType {
        ShortTerm,      //短期
        LongTermSummary,//长期总结
        VectorDB        //向量数据库
    };
    Q_ENUM_NS(MemoryType)

    // 世界书触发模式枚举
    enum class WorldInfoTriggerType {
        ExactMatch,//精确匹配
        FuzzyMatch,//模糊匹配
        RegexMatch//正则表达式匹配
    };


    // 枚举与字符串互转工具方法
    QString messageTypeToString(MessageType type);
    MessageType stringToMessageType(const QString& str);

}

#endif // GLOBALCONSTANTS_H
