#ifndef WORLDINFOMATCHER_H
#define WORLDINFOMATCHER_H

#include <QObject>
#include <QStringList>
#include "WorldInfoEntryEntity.h"
#include "ChatMessageEntity.h"
#include "ChatMessage.h"
#include "CharacterEntity.h"

struct MatchedWorldInfo {
    QString content;
    InsertPosition position;//插入位置
    int priority;//优先级(顺序)
};

struct WorldInfoDepthGroup {//世界书深度组
    int depth;//深度
    GlobalConstants::MessageType insertRole;    //--null/0/1/2   null:无 0:系统在深度 1:用户在深度  2:ai在深度
    QStringList entries;//条目
};

struct WorldInfoExample {//世界书对话示例
    QString content;
    int position;  // EMTop 或 EMBottom
    int sortOrder = 0;  // 同位置内的排序键（EMTop/EMBottom 各排各的）
};

struct WorldInfoResult {
    QString worldInfoString;                     // 完整字符串（UI 用）
    QString worldInfoBefore;            // 拼在 charDescription 前
    QString worldInfoAfter;             // 拼在 charDescription 后

    QList<WorldInfoExample> worldInfoExamples;// 示例消息位置的条目 5=EMTop, 6=EMBottom
    QList<WorldInfoDepthGroup> worldInfoDepth;// in-chat 深度注入
    QMap<QString, QStringList> outletEntries;// outlet 条目
};

class WorldInfoMatcher : public QObject {
    Q_OBJECT
public:
    static WorldInfoMatcher& instance();
    ~WorldInfoMatcher() override = default;

    //核心匹配入口:输入扫描文本、全部条目,返回世界书排序完成的文本
    WorldInfoResult matchAndResolve(
        const QList<ChatMessageEntity>& history,
        const QList<WorldInfoEntryEntity>& entries,   // 全部条目
        const QString& userInput,
        const CharacterEntity& character,         // ← 角色卡字段
        const QString& personaDesc = {},
        const QSet<QString>& stickyCache = {}   // 粘性状态（可选）
        );

    //构建扫描文本
    QString buildScanText(
        const QList<ChatMessageEntity>& history,
        const QString& userInput);

    // 按注入位置拼接成最终注入文本
    QString buildInjectedText(const QList<MatchedWorldInfo>& matched, InsertPosition position);

private:
    WorldInfoMatcher() = default;


    bool matchKeyword(
        const QString& text,
        const QString& keyword,
        bool useRegex,
        bool caseSensitive,
        bool matchWholeWords);

    bool rollProbability(int probability);


    //辅助函数
    // 单个条目"是否命中"的唯一判定入口：常驻 / 关键词 / 次关键词 / 向量化都在这里，
    // 首轮匹配和递归扫描都必须走它，否则两轮的判定条件会不一致。
    bool evaluateEntry(const WorldInfoEntryEntity& entry, const QString& scanText) const;
    bool isMatch(//是否匹配
        const WorldInfoEntryEntity& entry,const QString& scanText,
        bool globalCaseSensitive,bool globalWholeWords) const;
    bool checkSelective(//检查选择性
        const WorldInfoEntryEntity& entry,const QString& scanText,
        bool globalCaseSensitive,bool globalWholeWords) const;
    QList<WorldInfoEntryEntity> applyRecursion(//应用递归扫描
        QList<WorldInfoEntryEntity> firstPass,
        const QList<WorldInfoEntryEntity>& allEntries);
    void applyGroupScoring(QList<WorldInfoEntryEntity>& entries);//应用分组评分

    int world_info_depth= 2;                 // (重要)扫描聊天历史的深度（消息条数）
    int world_info_budget= 25;               // WI 的 token 预算百分比
    int world_info_budget_cap= 0;            // 预算上限（0=不限制）
    bool world_info_include_names= true;      // (重要)扫描时是否包含发言者名字
    bool world_info_recursive= false;         // 是否递归扫描
    int world_info_min_activations= 0;       // 至少触发多少条目
    int world_info_min_activations_depth_max= 0;
    int world_info_max_recursion_steps= 0;   // 最大递归层数
    bool world_info_case_sensitive= false;    // 全局大小写敏感()
    bool world_info_match_whole_words= false; // 全局全字匹配()
    bool world_info_use_group_scoring= false; // 全局组评分
    int world_info_character_strategy= 0;    // 多角色策略
};

#endif // WORLDINFOMATCHER_H