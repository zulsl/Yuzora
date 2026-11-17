#ifndef PROMPTBUILDER_H
#define PROMPTBUILDER_H

#include <QObject>
#include <QJsonArray>
#include <QVariantMap>
#include "GlobalConstants.h"
#include "CharacterEntity.h"
#include "ChatMessageEntity.h"
#include "ChatMessage.h"
#include "PromptEntity.h"
#include "CompletionPresetEntity.h"
#include "WorldInfoMatcher.h"
#include "ChatCompletion.h"
#include "MacroEngine.h"

//合集
struct PopulateContext {
    const WorldInfoResult* wi;
    QString userInput;
    QString bias;             // prompt bias（可能为空）
    QString quietPrompt;      // 静默提示（可能为空）
    QString quietImage;       // 静默图片（可能为空）
    QString type;             // "normal" / "continue" / "impersonate" / "quiet"
    //正常,继续,模仿,静默隐藏(用于生成摘要等)
    QString cyclePrompt;      // continue 场景的最后一条消息
    QString userName;         // 用户名字（{{user}} 与示例对话里的 name 用），空则退回 "User"
    QList<QList<ChatMessage>> messageExamples;   // 示例对话
};

//扩展提示（来自插件）
struct ExtensionPrompt {
    QString identifier;     // 唯一标识符
    QString role;           // "system", "user", "assistant"
    QString value;          // 内容
    int position;           // 注入位置: 0=前, 1=后（相对于主提示）
    int roleType;           // 扩展角色类型（可选）
};

//提示词构建器
class PromptBuilder : public QObject {
    Q_OBJECT
public:
    static PromptBuilder& instance();
    ~PromptBuilder() override = default;
    PromptBuilder(const PromptBuilder&) = delete;
    PromptBuilder& operator=(const PromptBuilder&) = delete;
    PromptBuilder(PromptBuilder&&) = delete;
    PromptBuilder& operator=(PromptBuilder&&) = delete;


    //构建完整聊天消息
    //
    // 提示词由调用方取好再传进来（不再在这里调 CompletionPresetService::instance()）：
    // 这样 buildChatMessages 就是一串纯函数，单测里给一份 fixture 就能断言最终消息序列。
    QList<ChatMessage> buildChatMessages(
        const CharacterEntity& character,   //当前角色
        const QList<ChatMessageEntity>& history,    //历史消息(按时间升序，不包含 status != Normal 的)
        const QString& userInput,   //最新用户输入（当前由历史承载，保留给"无历史时补一条"的未来用法）
        const CompletionPresetEntity& preset,       //当前预设
        const WorldInfoResult& wi,                  //世界书匹配结果
        const QList<PromptEntity>& userPrompts,     //预设里的提示词表（DB 里那条 preset 的）
        const QString& personaDesc = QString(),//用户 Persona 描述（可选）
        const QString& generationType = QStringLiteral("normal"),//normal/continue/regenerate/impersonate/quiet
        const QString& userName = QString()          // 用户名字；空则由调用方保证 {{user}} 退回 "User"
        );


    //估算消息列表的Token消耗量
    int estimateMessagesToken(
        const QList<ChatMessageEntity>& messages,
        GlobalConstants::PromptTemplateType templateType);
    //获取角色相关的提示词变量映射
    QVariantMap getPromptVariables(const CharacterEntity& character,
                                   const QString& userName = QString());

    // 当前构建属于哪个会话：{{setvar}} 这类宏的变量是"每个聊天一份"的变量。
    // 由 ChatPromptContext 在 buildChatMessages 之前设置。
    void setVariableSession(const QString& sessionId) { m_variableSession = sessionId; }
    // 某个会话的本地变量（聊天级），主要给测试和调试用
    QHash<QString, QString> localVariables(const QString& sessionId) const
    {
        return m_localVariables.value(sessionId);
    }
    QHash<QString, QString>& globalVariables() { return m_globalVariables; }

    //以下未启用
    //获取指定类型的默认模板文本
    QString getPromptTemplate(GlobalConstants::PromptTemplateType type);
    // 构建角色基础系统提示词（仅用于兼容旧接口）
    QString buildSystemPrompt(const CharacterEntity& character);


private:
    explicit PromptBuilder(QObject* parent = nullptr);

    void loadDefaultTemplates();
    void loadDefaultPromptsAndOrder(); // 初始化酒馆风格的默认提示词与顺序
    void insertByCanonical(QList<PromptEntity> & result,QSet<QString> &seenIds,const PromptEntity& p);//插入保底提示词顺序,解决提示词顺序列表缺失项

    // 宏替换（SillyTavern 语义，见 MacroEngine）
    QString substituteParams(const QString& input, const QVariantMap& vars);

    // 聊天级宏：{{lastMessage}} / {{lastUserMessage}} / {{lastCharMessage}}
    // ST 的定义见 scripts/macros/definitions/chat-macros.js:103-116
    QVariantMap getChatMacros(const QList<ChatMessageEntity>& history,
                              const QString& userInput) const;

    // 准备提示集合（合并系统提示与用户配置，应用覆盖）
    // vars 由 buildChatMessages 统一算好传进来（角色变量 + 聊天宏），
    // 避免各处重复 getPromptVariables 得到不一致的变量表。
    QList<PromptEntity> preparePromptCollection(
        const CompletionPresetEntity& preset,
        const WorldInfoResult & wi,
        const CharacterEntity & character,
        const QList<PromptEntity>& userPrompts,
        const QString& generationType,
        const QVariantMap& vars);

    //============开始填充===================
    // 返回值就是最终消息序列（内部按提示词顺序扁平化）
    // outIdentifiers 非空时，按输出顺序记录每条消息来源的提示词 identifier（调试/对比 ST 用）
    QList<ChatMessage> populateChatCompletion(
        const QList<PromptEntity>& prompts,ChatCompletion& chatCompletion,
        const CompletionPresetEntity & preset,
        const QList<ChatMessageEntity> & history,const QString & userInput,
        const CharacterEntity & character,
        const WorldInfoResult& wi,
        const PopulateContext& pCtx,
        const QVariantMap& vars,
        QStringList* outIdentifiers = nullptr);

    // 把本次装配过程写成 prompt_trace_last.json（YUZORA_TRACE_PROMPT 控制）
    void writePromptTrace(const CompletionPresetEntity& preset,
                          const QList<PromptEntity>& allPrompts,
                          const QString& generationType,
                          const QList<ChatMessage>& messages,
                          const QStringList& messageIdentifiers) const;



    // 历史消息滑动窗口截断,根据Token截断上下文
    QList<ChatMessageEntity> truncateContext(
        const QList<ChatMessageEntity>& context,//消息列表
        int systemPromptTokens,//当前系统Token
        int userInputTokens,//用户输入最大Token
        int maxTotalToken,//最大Token
        GlobalConstants::PromptTemplateType templateType//提示词模板类型
        );


    // ===== 内部实现 =====
    void appendToCollection(//添加到系统提示词集合
        QList<PromptEntity>& collection,//集合
        const QString& content,//内容
        const QString& identifier,
        GlobalConstants::MessageType role=GlobalConstants::MessageType::System,//角色
        const QString& name = QString(),//发送者名称
        bool skipIfEmpty = true);


    //将历史记录转换为消息
    QList<ChatMessage> convertHistoryToMessages(const QList<ChatMessageEntity>& history);
    //注入到聊天历史
    QList<ChatMessage> populationInjectionPrompts(
        const QList<PromptEntity>& absolutePrompts,
        const QList<ChatMessage>& messages,
        const WorldInfoResult& wi,
        const QVariantMap& vars);
    //填充对话示例（组名 dialogueExamples，位置由 prompts 顺序决定）
    void populateDialogueExamples(const QList<QList<ChatMessage>>& messageExamples,
        ChatCompletion& chatCompletion,
        const CompletionPresetEntity& preset,const CharacterEntity& character,
        const QVariantMap& vars);
    //填充聊天历史记录（组名 chatHistory，位置由 prompts 顺序决定）
    void populateChatHistory(const QList<ChatMessage>& messages,
        ChatCompletion& chatCompletion,
        const CompletionPresetEntity& preset,
        const QVariantMap& vars);
    // 把角色卡 example_dialogue + 世界书 EMTop/EMBottom 解析成 <START> 分隔的示例块。
    // 解析结果保留 user/assistant 角色用于定位说话人，发送时统一按 ST 的 system + name 发。
    QList<QList<ChatMessage>> parseExampleDialogue(
            const CharacterEntity& character,
            const WorldInfoResult& wi,
            const QVariantMap& vars);
    // 解析一段示例文本（ST：<START> 分块，行首 "{{user}}:" / "{{char}}:" 切分说话人）
    QList<QList<ChatMessage>> parseExampleText(const QString& raw,
                                               const CharacterEntity& character,
                                               const QVariantMap& vars);



    const PromptEntity* getPromptById(const QString& identifier) const;
    int estimateTokens(const QString& text) const;

    std::function<int(const QString&)> m_estimator;//"可调用对象"的容器
    /*策略模式,函数体可改,初始为构造的Lambda函数,赋值可更改函数体*/

    // ST 的局部变量是"每个聊天一份"，全局变量全局一份。
    // 局部变量按 sessionId 分桶，避免多会话互相污染。
    QString m_variableSession;
    QHash<QString, QHash<QString, QString>> m_localVariables;
    QHash<QString, QString> m_globalVariables;

    // 每条消息额外开销（role + name + 边界符号，OpenAI 约 +3~+4）
    static inline constexpr int PER_MESSAGE_OVERHEAD = 4;



    QMap<GlobalConstants::PromptTemplateType, QString> m_templates;
    QStringList m_guaranteedPrompts;
    // QList<PromptEntity> m_prompts;
    // QList<Prompt> m_prompt;                 // 所有提示词库
    // QList<PromptOrderEntry> m_promptOrder;   // 当前角色的顺序列表
    // QHash<QString, PromptEntity> m_promptLookup; // 哈希表快速查找（identifier -> Prompt）
};

#endif