#include "WorldInfoMatcher.h"
#include <QRegularExpression>
#include <QRandomGenerator>
#include <QSet>
#include <algorithm>

// ======================================
// 单例实例
// ======================================
WorldInfoMatcher& WorldInfoMatcher::instance()
{
    static WorldInfoMatcher instance;
    return instance;
}

// ======================================
// 核心匹配入口
// ======================================
WorldInfoResult WorldInfoMatcher::matchAndResolve(
    const QList<ChatMessageEntity>& history,
    const QList<WorldInfoEntryEntity>& entries,   // 全部条目
    const QString& userInput,
    const CharacterEntity& character,         // ← 角色卡字段
    const QString& personaDesc,
    const QSet<QString>& stickyCache   // 粘性状态（可选）
    )
{
    //===== 构建扫描文本 =====
    QString scanText = buildScanText(history,userInput,character);// 已拼好的扫描文本



    // ===== 第一步：首轮匹配 =====
    QList<WorldInfoEntryEntity> activated;//匹配成功的条目

    for (const auto& entry : entries) {
        // if (entry.disable) continue;

        // 1. 常驻直接命中()
        bool hit = entry.constant;

        // 2. 关键词匹配
        if (!hit && !entry.vectorized && !entry.keywords.isEmpty()) {
            hit = isMatch(entry,scanText,world_info_case_sensitive,world_info_match_whole_words);
        }

        // 3. 次关键词（selective）
        if (hit && entry.selective && !entry.secondaryKeywords.isEmpty()) {
            hit = checkSelective(entry, scanText,world_info_case_sensitive,world_info_match_whole_words);
        }

        // 4. 概率(未完成)
        // if (hit && entry.useProbability && entry.probability < 100) {
        //     hit = rollProbability(entry);
        // }

        // 5. 粘性/冷却/延迟（未完成）
        // if (hit && stickyCache.contains(entry.id)) hit = false;
        // if (hit && entry.cooldown > 0) hit = false;

        if (hit) activated.append(entry);
    }

    // ===== 第二步：递归扫描 =====
    if (world_info_recursive && !activated.isEmpty()) {
        activated = applyRecursion(activated, entries);
    }

    //组评分(未完成)
    // // ===== 第三步：组竞争 =====
    // applyGroupScoring(activated);
    // ===== 第四步：过滤被淘汰 =====
    // activated.erase(std::remove_if(activated.begin(), activated.end(),
    //     [](const auto& e) { return e._excluded; }),activated.end());


    // ===== 第五步：按 "插入位置" 分类 =====
    QList<WorldInfoEntryEntity> before, after;//两个列表
    QMap<int, QList<WorldInfoEntryEntity>> depthMap;//深度列表
    QList<WorldInfoExample> examples;//对话示例
    QMap<QString, QStringList> outlets;//outlet

    for (const auto& entry : activated) {
        switch (entry.position) {
        case WorldInfoEntryEntity::BeforeChar: before.append(entry); break;
        case WorldInfoEntryEntity::AfterChar: after.append(entry);  break;
        case WorldInfoEntryEntity::AtDepth: depthMap[entry.insertDepth].append(entry); break;
        case WorldInfoEntryEntity::EMTop:
            examples.append({entry.content, WorldInfoEntryEntity::EMTop}); break;
        case WorldInfoEntryEntity::EMBottom:
            examples.append({entry.content, WorldInfoEntryEntity::EMBottom}); break;
        case WorldInfoEntryEntity::Outlet:
            outlets[entry.group].append(entry.content); break;
        default:
            // ANTop/ANBottom 在 ST 里走 authors-note 路径，这里先跳过
            break;
        }
    }

    // ===== 第六步：排序 =====
    auto byOrder = [](const WorldInfoEntryEntity& a, const WorldInfoEntryEntity& b) {
        return a.sortOrder < b.sortOrder;
    };
    std::sort(before.begin(), before.end(), byOrder);
    std::sort(after.begin(), after.end(), byOrder);
    for (auto it = depthMap.begin(); it != depthMap.end(); ++it) {
        std::sort(it.value().begin(), it.value().end(), byOrder);
    }

    // ===== 第七步：组装输出 =====
    WorldInfoResult result;

    //将条目内容加入列表
    auto joinContents = [](const QList<WorldInfoEntryEntity>& list) {
        QStringList parts;
        for (const auto& e : list) parts << e.content.trimmed();
        return parts.join('\n');
    };
    result.worldInfoBefore = joinContents(before);
    result.worldInfoAfter  = joinContents(after);
    result.worldInfoExamples = examples;

    //按顺序加入in-chat深度表(待优化***)
    for (auto it = depthMap.begin(); it != depthMap.end(); ++it) {
        WorldInfoDepthGroup group;
        group.depth = it.key();
        group.insertRole = it.value().first().insertRole;   // 同 depth 默认同 role
        for (const auto& e : it.value()) {
            group.entries << e.content.trimmed();
        }
        result.worldInfoDepth.append(group);
    }

    //outlets
    result.outletEntries = outlets;

    // 完整字符串(保留列表,可供UI显示用)
    QStringList all;
    if (!result.worldInfoBefore.isEmpty()) all << result.worldInfoBefore;
    if (!result.worldInfoAfter.isEmpty())  all << result.worldInfoAfter;
    for (const auto& g : result.worldInfoDepth)
        all << g.entries.join('\n');
    result.worldInfoString = all.join('\n');

    return result;
}

QString WorldInfoMatcher::buildScanText(const QList<ChatMessageEntity> &history, const QString &userInput, const CharacterEntity &character)
{
    // 1. 构造扫描文本
    QString scanText;
    scanText += userInput + "\n";

    // 只扫描最近 N 条
    int scanDepth = world_info_depth;
    QList<ChatMessageEntity> recent;
    for (int i = history.size() - 1; i >= 0 && recent.size() < scanDepth; --i) {
        recent.prepend(history[i]);
    }
    for (const auto& msg : recent) {
        if (world_info_include_names)
            scanText += msg.senderName + ": " + msg.content + "\n";
        else
            scanText += msg.content + "\n";
    }

    scanText += character.description + "\n";
    scanText += character.personality + "\n";
    scanText += character.scenario + "\n";

    return scanText;
}


// ======================================
// 单关键词匹配：支持正则/全词/大小写敏感三种维度组合
// ======================================
bool WorldInfoMatcher::matchKeyword(
    const QString& text,
    const QString& keyword,
    bool useRegex,
    bool caseSensitive,
    bool matchWholeWords)
{
    if (keyword.isEmpty()) {
        return false;
    }

    Qt::CaseSensitivity caseMode = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    // 模式1：正则表达式匹配
    if (useRegex) {
        QRegularExpression::PatternOptions opts;
        if (!caseSensitive) {
            opts |= QRegularExpression::CaseInsensitiveOption;
        }
        QRegularExpression regex(keyword, opts);
        if (!regex.isValid()) {
            return false; // 无效正则直接判定不匹配，避免崩溃
        }
        return regex.match(text).hasMatch();
    }

    // 模式2：全词匹配（非正则场景下用 \b 边界实现）
    if (matchWholeWords) {
        QString escapedKey = QRegularExpression::escape(keyword);
        QString pattern = QStringLiteral("\\b%1\\b").arg(escapedKey);
        QRegularExpression::PatternOptions opts;
        if (!caseSensitive) {
            opts |= QRegularExpression::CaseInsensitiveOption;
        }
        QRegularExpression regex(pattern, opts);
        return regex.match(text).hasMatch();
    }

    // 模式3：普通子串包含匹配
    return text.contains(keyword, caseMode);
}

// ======================================
// 概率随机判定
// ======================================
bool WorldInfoMatcher::rollProbability(int probability)
{
    if (probability <= 0) {
        return false;
    }
    if (probability >= 100) {
        return true;
    }
    // 生成 [0, 99] 整数，小于概率值即命中
    return QRandomGenerator::global()->bounded(100) < probability;
}

bool WorldInfoMatcher::isMatch(const WorldInfoEntryEntity &entry, const QString &scanText, bool globalCaseSensitive, bool globalWholeWords)
{
    // bool caseSensitive = entry.caseSensitive.value_or(globalCaseSensitive);
    // bool wholeWords = entry.matchWholeWords.value_or(globalWholeWords);
    bool caseSensitive = entry.caseSensitive;
    bool wholeWords = entry.matchWholeWords;


    const QString haystack = caseSensitive ? scanText : scanText.toLower();

    for (const QString& key : entry.keywords) {
        if (key.isEmpty()) continue;
        const QString needle = caseSensitive ? key : key.toLower();

        if (wholeWords) {
            // 用 QRegularExpression 的 \b
            QRegularExpression re(
                QString("\\b%1\\b").arg(QRegularExpression::escape(needle)),
                caseSensitive
                    ? QRegularExpression::NoPatternOption
                    : QRegularExpression::CaseInsensitiveOption);
            if (re.match(scanText).hasMatch()) return true;
        } else {
            if (haystack.contains(needle)) return true;
        }
    }
    return false;
}

bool WorldInfoMatcher::checkSelective(const WorldInfoEntryEntity &entry, const QString &scanText, bool globalCaseSensitive, bool globalWholeWords)
{
    QList<bool> matchResults;
    for (const QString& key : entry.secondaryKeywords) {
        WorldInfoEntryEntity temp = entry;
        temp.keywords = {key};
        temp.constant = false;
        matchResults.append(isMatch(temp, scanText,globalCaseSensitive, globalWholeWords));
    }

    switch (entry.selectiveLogic) {
    case WorldInfoEntryEntity::AND_ANY:
        return std::any_of(matchResults.begin(), matchResults.end(),
                           [](bool b) { return b; });
    case WorldInfoEntryEntity::AND_ALL:
        return std::all_of(matchResults.begin(), matchResults.end(),
                           [](bool b) { return b; });
    case WorldInfoEntryEntity::NOT_ANY:
        return std::none_of(matchResults.begin(), matchResults.end(),
                            [](bool b) { return b; });
    case WorldInfoEntryEntity::NOT_ALL:
        return std::any_of(matchResults.begin(), matchResults.end(),
                           [](bool b) { return !b; });
    }
    return false;
}

QList<WorldInfoEntryEntity> WorldInfoMatcher::applyRecursion(QList<WorldInfoEntryEntity> firstPass, const QList<WorldInfoEntryEntity> &allEntries)
{
    const int maxSteps = world_info_max_recursion_steps>0 ?world_info_max_recursion_steps :3;

    QSet<QString> activatedIds;
    for (const auto& e : firstPass) activatedIds.insert(e.id);

    QString currentText;
    for (const auto& e : firstPass) {
        if (e.excludeRecursion) continue;
        currentText += e.content + "\n";
    }

    for (int step = 0; step < maxSteps; ++step) {
        QList<WorldInfoEntryEntity> newActivated;
        for (const auto& entry : allEntries) {
            // if (entry.disable) continue;
            if (activatedIds.contains(entry.id)) continue;
            if (entry.preventRecursion) continue;
            if (entry.delayUntilRecursion && step == 0) continue;

            WorldInfoEntryEntity copy = entry;
            copy.delayUntilRecursion = false;
            if (isMatch(copy, currentText,world_info_case_sensitive,world_info_match_whole_words)) {
                newActivated.append(entry);
                activatedIds.insert(entry.id);
            }
        }

        if (newActivated.isEmpty()) break;

        firstPass.append(newActivated);
        currentText.clear();
        for (const auto& e : newActivated) {
            if (e.excludeRecursion) continue;
            currentText += e.content + "\n";
        }
    }

    return firstPass;
}

void WorldInfoMatcher::applyGroupScoring(QList<WorldInfoEntryEntity> &entries)
{
    QMap<QString, QList<int>> groups;  // group name -> indices

     for (int i = 0; i < entries.size(); ++i) {
         if (entries[i].group.isEmpty()) continue;
         groups[entries[i].group].append(i);
     }

     for (auto it = groups.begin(); it != groups.end(); ++it) {
         if (it.value().size() <= 1) continue;

         // 找 groupWeight 最大的
         int winnerIdx = it.value().first();
         for (int idx : it.value()) {
             if (entries[idx].groupWeight > entries[winnerIdx].groupWeight) {
                 winnerIdx = idx;
             }
         }

         // // 其余标记淘汰
         // for (int idx : it.value()) {
         //     if (idx != winnerIdx) entries[idx]._excluded = true;
         // }
     }
}

// ======================================
// 按注入位置拼接最终文本
// ======================================
QString WorldInfoMatcher::buildInjectedText(const QList<MatchedWorldInfo>& matched, InsertPosition position)
{
    QStringList fragments;
    for (const auto& item : matched) {
        if (item.position == position) {
            fragments.append(item.content.trimmed());
        }
    }
    // 条目间用空行分隔，与 SillyTavern 输出格式完全一致
    return fragments.join("\n\n");
}