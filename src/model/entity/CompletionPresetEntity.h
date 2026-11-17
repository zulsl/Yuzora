#ifndef COMPLETIONPRESETENTITY_H
#define COMPLETIONPRESETENTITY_H

#include <QString>
#include <QVariantMap>
#include <QDateTime>
#include <QtQml>
#include "GlobalConstants.h"
#include "PromptEntity.h"

//对话补全预设数据实体
// struct PromptOrderEntry {
//     QString identifier;
//     bool enabled = true;
// };

struct CompletionPresetEntity {
    Q_GADGET//元对象开关
    QML_VALUE_TYPE(completionPresetEntity)//注册为 QML 原生值类型

public:
    QString id;                                 //唯一id
    QString name;
    bool isActive = false;                     //是否启用

    // ========== 基础生成 ==========
    int maxContextToken = 4096;                         //最大上下文长度（以词符数计）上下文窗口
    int maxResponseToken = 1024;                        //最大回复长度（以词符数计）
    bool streamEnabled = true;                          //是否流式
    QJsonObject stream_options;//流式响应选项,仅在stream:true时生效。只控制用量信息的返回方式,不改变计费规则。

    // ========== 常用参数 ==========
    QJsonObject thinking;//控制大模型是否开启思维链。
    QString reasoning_effort;//控制模型的推理程度
    QJsonObject response_format;//指定模型的响应输出格式
    QStringList stop;   //停止词列表,当模型生成的文本中遇到这些指定的字符串时会立即停止生成。

    // ========== 核心采样参数 ==========
    double temperature = 0.7;                   //温度(创造力,越低越死板,越高越放飞自我.写作推荐1.0,问答0.3~0.7)
    double topP = 0.9;                          //词汇多样性(0.1:只选最肯定的词,0.9:选概率前 90% 的词,写作推荐0.95)

    //以下弃用
    double topK = 40;                           //候选词数量(topK=1:永远选概率最高的(最死板),topK=40:从前40个词里随机选(自然),写作推荐60)
    double frequencyPenalty = 0.0;              //频率惩罚(弃用)
    double presencePenalty = 0.0;               //存在惩罚(弃用)
    double repetitionPenalty = 1.1;             //重复惩罚
    //一种更广义的惩罚，直接对已出现过的 token 的概率进行削减。(1.0:不惩罚,1.1:轻微惩罚(默认)1.5~2.0:强惩罚.写作推荐1.05)
    double topA = 0.0;                          //局部典型采样
    //另一种动态采样方法，其具体机制因实现而异，通常根据概率分布的形状动态调整候选集大小。
    double minP = 0.0;                          //最小概率采样
    //一种较新的采样策略。它设定一个相对于最高概率词的动态阈值。

    // ========== 扩展与时间戳 ==========
    QVariantMap extraSettings;
    QDateTime createdAt;
    QDateTime updatedAt;

    // ========== 杂项 ==========
    bool maxContextUnlocked=true;           //最大上下文解锁
    int namesBehavior=-1;                   //角色名称行为
    QString continuePostfix;               //继续后缀
    bool continuePrefill=false;            //继续预填充
    bool squashSystemMessages=false;       //压缩系统消息
    bool functionCalling=false;            //启用函数调用
    bool showThoughts=false;               //请求思维链

    // ========== 顺序列表(顺序条目) ==========
    // QList<PromptOrderEntry> promptOrder;
    QStringList promptOrder;

    // ========== 实用提示词 ==========
    // 默认值必须和 ST 的 default_* 一致（openai.js:107-114,428）：
    // 这些值只代表"没设置"，显式空串（比如狐神抚的 new_chat_prompt=""）导入后必须保持空，
    // 所以 PromptBuilder 不再做"空 → 默认"的回退，回退放在这里和 DAO 里。
    QString impersonationPrompt;                //ai帮答提示词
    QString wiFormat = QStringLiteral("{0}");   //世界信息格式模板
    QString scenarioFormat = QStringLiteral("{{scenario}}");    //场景格式模板
    QString personalityFormat = QStringLiteral("{{personality}}"); //角色设定格式模板
    QString groupNudgePrompt;                   //群聊推进提示词模板
    QString newChatPrompt = QStringLiteral("[Start a new Chat]");      //新聊天
    QString newGroupChatPrompt = QStringLiteral("[Start a new group Chat]"); //新群聊
    QString newExampleChatPrompt = QStringLiteral("[Example Chat]");   //新示例聊天
    QString continueNudgePrompt;                //继续推进
    // QString//替换空消息



    //以下未写入外存(废弃)
    // ========== 动态温度 ==========
    bool dynamicTempEnabled = false;            //动态温度已启用
    double dynamicTempMin = 0.5;                //最低动态温度
    double dynamicTempMax = 1.5;                //动态最高温度
    double dynamicTempExponent = 1.0;           //动态温度指数
    // ========== 提示词模板 ==========
    GlobalConstants::PromptTemplateType promptTemplate
        = GlobalConstants::PromptTemplateType::ChatML;  //提示词模板类型
    QString customTemplate;                             //自定义模板()
    QString systemPromptPrefix;                         //系统提示前缀
    QString systemPromptSuffix;                         //系统提示后缀
    QString userMessagePrefix;                          //用户消息前缀
    QString userMessageSuffix;                          //用户消息后缀
    QString assistantMessagePrefix;                     //助手消息前缀
    QString assistantMessageSuffix;                     //助手消息后缀
    QString conversationSeparator;                      //对话分隔符
    // ========== 停止序列 ==========
    QStringList stoppingStrings;                 //停止字符串
    // ========== 响应处理 ==========
    bool autoContinueEnabled = false;           //自动继续已启用
    int autoContinueMax = 3;                    //自动继续最大值
    int maxRetryCount = 1;                      //最大重试次数

    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(bool isActive MEMBER isActive)
    Q_PROPERTY(int maxContextToken MEMBER maxContextToken)
    Q_PROPERTY(int maxResponseToken MEMBER maxResponseToken)
    Q_PROPERTY(bool streamEnabled MEMBER streamEnabled)

    Q_PROPERTY(double temperature MEMBER temperature)
    Q_PROPERTY(double topP MEMBER topP)
    Q_PROPERTY(double topK MEMBER topK)
    Q_PROPERTY(double frequencyPenalty MEMBER frequencyPenalty)
    Q_PROPERTY(double presencePenalty MEMBER presencePenalty)
    Q_PROPERTY(double repetitionPenalty MEMBER repetitionPenalty)
    Q_PROPERTY(double topA MEMBER topA)
    Q_PROPERTY(double minP MEMBER minP)

    Q_PROPERTY(QVariantMap extraSettings MEMBER extraSettings)
    Q_PROPERTY(QDateTime createdAt MEMBER createdAt)
    Q_PROPERTY(QDateTime updatedAt MEMBER updatedAt)

    Q_PROPERTY(bool maxContextUnlocked MEMBER maxContextUnlocked)
    Q_PROPERTY(int namesBehavior MEMBER namesBehavior)
    Q_PROPERTY(QString continuePostfix MEMBER continuePostfix)
    Q_PROPERTY(bool continuePrefill MEMBER continuePrefill)
    Q_PROPERTY(bool squashSystemMessages MEMBER squashSystemMessages)
    Q_PROPERTY(bool functionCalling MEMBER functionCalling)
    Q_PROPERTY(bool showThoughts MEMBER showThoughts)

    // ========== 实用提示词 ==========
    Q_PROPERTY(QString impersonationPrompt MEMBER impersonationPrompt)
    Q_PROPERTY(QString wiFormat MEMBER wiFormat)
    Q_PROPERTY(QString scenarioFormat MEMBER scenarioFormat)
    Q_PROPERTY(QString personalityFormat MEMBER personalityFormat)
    Q_PROPERTY(QString groupNudgePrompt MEMBER groupNudgePrompt)
    Q_PROPERTY(QString newChatPrompt MEMBER newChatPrompt)
    Q_PROPERTY(QString newGroupChatPrompt MEMBER newGroupChatPrompt)
    Q_PROPERTY(QString newExampleChatPrompt MEMBER newExampleChatPrompt)
    Q_PROPERTY(QString continueNudgePrompt MEMBER continueNudgePrompt)

};
// 声明类型可存入 QVariant（Qt6 大部分场景可省略，建议保留兼容）
Q_DECLARE_METATYPE(CompletionPresetEntity)
#endif