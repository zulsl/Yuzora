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
        const QString& generationType = QStringLiteral("normal")//normal/continue/regenerate/impersonate/quiet
        );


    //以下未启用
    //估算消息列表的Token消耗量
    int estimateMessagesToken(
        const QList<ChatMessageEntity>& messages,
        GlobalConstants::PromptTemplateType templateType);
    //获取指定类型的默认模板文本
    QString getPromptTemplate(GlobalConstants::PromptTemplateType type);
    //获取角色相关的提示词变量映射
    QVariantMap getPromptVariables(const CharacterEntity& character);
    // 构建角色基础系统提示词（仅用于兼容旧接口）
    QString buildSystemPrompt(const CharacterEntity& character);



    /**
     * @brief 构建完整的聊天消息数组（核心方法）
     * @param templateType 提示词模板类型（暂未使用，保留接口）
     * @param maxContextToken 最大上下文Token数，用于自动截断历史
     * @param customTemplate 自定义模板字符串，仅CustomPrompt类型生效（暂未使用）
     * @param worldInfoSection 世界书注入文本
     * @param memorySection 记忆注入文本
     * @param generationType     生成类型："normal", "continue", "impersonate", "quiet"
     * @param systemPromptOverride 角色卡覆盖的主提示（可为空）
     * @param jailbreakPromptOverride 角色卡覆盖的越狱提示（可为空）
     * @param extensionPrompts   扩展提示列表（来自插件）
     * @param cyclePrompt        用于继续生成时的最后一条消息（可选）
     * @param messageExamples    示例对话（可选）
     * @return 格式化后的消息数组（统一ChatML结构）
     */
    QList<ChatMessage> buildChatMessages(
        const CharacterEntity& character,
        const QList<ChatMessageEntity>& history,
        const QString& userInput,
        GlobalConstants::PromptTemplateType templateType,
        int maxContextToken,
        const QString& customTemplate,
        const QString& worldInfoSection,
        const QString& memorySection,
        const QString& generationType,//生成类型："normal", "continue", "impersonate", "quiet"
        const QString& systemPromptOverride,
        const QString& jailbreakPromptOverride,
        const QList<ExtensionPrompt>& extensionPrompts,//扩展提示列表（来自插件）
        const QString& cyclePrompt,
        const QList<ChatMessageEntity>& messageExamples
        );

private:
    explicit PromptBuilder(QObject* parent = nullptr);

    void loadDefaultTemplates();
    void loadDefaultPromptsAndOrder(); // 初始化酒馆风格的默认提示词与顺序


    // 变量替换（使用 Utils::replacePromptVariables）
    QString substituteParams(const QString& input, const QVariantMap& vars) const;
    // 准备提示集合（合并系统提示与用户配置，应用覆盖）
    QList<PromptEntity> preparePromptCollection(
        const CompletionPresetEntity& preset,
        const WorldInfoResult & wi,
        const CharacterEntity & character,
        const QList<PromptEntity>& userPrompts);

    //============开始填充===================
    // 返回值就是最终消息序列（内部按提示词顺序扁平化）
    QList<ChatMessage> populateChatCompletion(
        const QList<PromptEntity>& prompts,ChatCompletion& chatCompletion,
        const CompletionPresetEntity & preset,
        const QList<ChatMessageEntity> & history,const QString & userInput,
        const CharacterEntity & character,
        const WorldInfoResult& wi,
        const PopulateContext& pCtx);



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
        const CompletionPresetEntity& preset,const CharacterEntity& character);
    //填充聊天历史记录（组名 chatHistory，位置由 prompts 顺序决定）
    void populateChatHistory(const QList<ChatMessage>& messages,
        ChatCompletion& chatCompletion,
        const CompletionPresetEntity& preset);
    //把角色卡的 example_dialogue 解析成 <START> 分隔的示例块（user/assistant 交替）
    QList<QList<ChatMessage>> parseExampleDialogue(const CharacterEntity& character);



    const PromptEntity* getPromptById(const QString& identifier) const;
    int estimateTokens(const QString& text) const;

    std::function<int(const QString&)> m_estimator;//"可调用对象"的容器
    /*策略模式,函数体可改,初始为构造的Lambda函数,赋值可更改函数体*/

    // 每条消息额外开销（role + name + 边界符号，OpenAI 约 +3~+4）
    static inline constexpr int PER_MESSAGE_OVERHEAD = 4;



    QMap<GlobalConstants::PromptTemplateType, QString> m_templates;
    // QList<PromptEntity> m_prompts;
    // QList<Prompt> m_prompt;                 // 所有提示词库
    // QList<PromptOrderEntry> m_promptOrder;   // 当前角色的顺序列表
    // QHash<QString, PromptEntity> m_promptLookup; // 哈希表快速查找（identifier -> Prompt）
};

#endif