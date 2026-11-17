#ifndef MACROENGINE_H
#define MACROENGINE_H

#include <QHash>
#include <QString>

/**
 * @brief SillyTavern 宏（{{...}}）替换引擎
 *
 * ST 1.19 的宏系统在 public/scripts/macros/：
 *   MacroLexer → MacroParser → MacroCstWalker → MacroRegistry 里注册的 handler。
 * 这里用等价语义实现提示词构建真正用到的那部分（详见 docs/MACRO_ENGINE.md）：
 *
 *   - 普通取值宏：{{char}} / {{user}} / {{description}} / {{mesExamples}} …
 *   - 带参宏：{{reverse::abc}}，以及 ST 兼容的老写法 {{reverse:abc}}、{{reverse abc}}
 *   - 注释：{{// 任意文本}}（单行）、{{//}}…{{///}}（块注释）
 *   - 变量：{{setvar::n::v}} / {{getvar::n}} / {{addvar}} / {{incvar}} / {{decvar}}
 *           / {{setglobalvar}} / {{getglobalvar}} / {{hasvar}}
 *   - 作用域宏：{{setvar::n}}内容{{/setvar}}、{{trim}}…{{/trim}}、{{if 条件}}…{{else}}…{{/if}}
 *   - 其它：{{trim}} / {{noop}} / {{newline}} / {{reverse}} / {{random}} / {{pick}} / {{roll}}
 *   - 未知宏原样保留，但里面的嵌套宏照样会被解析（和 ST 一致）
 *
 * 变量作用域：localVars 对应 ST 的 chat_metadata.variables（每个聊天一份），
 * globalVars 对应 ST 的 extension_settings.variables.global（全局一份）。
 */
struct MacroEnv {
    QHash<QString, QString> values;        // 直接取值的宏（char/user/description/...）
    QHash<QString, QString> lazyValues;    // 想取时才算的宏（如 mesExamples）
    QHash<QString, QString> *localVars = nullptr;   // 聊天级变量，非空才支持 setvar 等
    QHash<QString, QString> *globalVars = nullptr;  // 全局变量
    QString userName = QStringLiteral("User");
    QString charName = QStringLiteral("Character");

    QString lookup(const QString &name) const;
};

class MacroEngine
{
public:
    /// 替换 content 里所有宏；env 里的变量表会被 {{setvar}} 之类改写
    static QString evaluate(const QString &content, MacroEnv &env);

    /// 只做替换、不执行有副作用的宏（给"预览"用的保守模式）
    static QString evaluateReadOnly(const QString &content, const MacroEnv &env);
};

#endif // MACROENGINE_H
