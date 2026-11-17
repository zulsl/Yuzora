#include "MacroEngine.h"

#include <QRandomGenerator>
#include <QSet>
#include <QStringList>

namespace {

bool isNameStart(QChar c)
{
    return (c >= QLatin1Char('a') && c <= QLatin1Char('z'))
        || (c >= QLatin1Char('A') && c <= QLatin1Char('Z'));
}

bool isNameChar(QChar c)
{
    return isNameStart(c) || (c >= QLatin1Char('0') && c <= QLatin1Char('9'))
        || c == QLatin1Char('_') || c == QLatin1Char('-');
}

QString rtrimLine(QString s)
{
    while (!s.isEmpty() && s.back().isSpace()) s.chop(1);
    return s;
}

int leadingSpaces(const QString &s)
{
    int n = 0;
    while (n < s.size() && (s[n] == QLatin1Char(' ') || s[n] == QLatin1Char('\t'))) ++n;
    return n;
}

bool isTruthy(const QString &value)
{
    const QString v = value.trimmed();
    if (v.isEmpty()) return false;
    const QString lower = v.toLower();
    if (lower == QLatin1String("false") || lower == QLatin1String("off")) return false;
    bool ok = false;
    const double number = v.toDouble(&ok);
    if (ok) return number != 0.0;
    return true;
}

/// 单值宏：宏名 -> 是否认识
bool isValueMacro(const QString &name)
{
    static const QSet<QString> names = {
        QStringLiteral("char"), QStringLiteral("user"), QStringLiteral("group"),
        QStringLiteral("charifnotgroup"), QStringLiteral("groupnotmuted"),
        QStringLiteral("charifnotgroup"), QStringLiteral("notchar"),
        QStringLiteral("description"), QStringLiteral("personality"),
        QStringLiteral("scenario"), QStringLiteral("persona"),
        QStringLiteral("charprompt"), QStringLiteral("charinstruction"),
        QStringLiteral("charjailbreak"), QStringLiteral("charversion"),
        QStringLiteral("char_version"), QStringLiteral("creatornotes"),
        QStringLiteral("creator_notes"), QStringLiteral("mesexamples"),
        QStringLiteral("mesexamplesraw"), QStringLiteral("chardepthprompt"),
        QStringLiteral("model"), QStringLiteral("original"),
        QStringLiteral("maxprompt"), QStringLiteral("maxprompttokens"),
        QStringLiteral("maxcontext"), QStringLiteral("maxcontexttokens"),
        QStringLiteral("maxresponse"), QStringLiteral("maxresponsetokens"),
        QStringLiteral("input"), QStringLiteral("lastmessage"),
        QStringLiteral("lastusermessage"), QStringLiteral("lastcharmessage"),
    };
    return names.contains(name);
}

struct Macro {
    QString name;                 // 小写
    QStringList rawArgs;
    bool keepWhitespace = false;  // # 旗标：不做 auto-trim
    bool scoped = false;
    QString scopedContent;
    int openEnd = -1;             // 开头标签结束位置（"}}"" 之后）
    int end = -1;                 // 指向 "}}" 之后
};

/// 跳过 {{ 之后、宏名之前的执行旗标（! ? ~ #）
int skipFlags(const QString &text, int pos, bool *keepWhitespace = nullptr)
{
    const int n = text.size();
    while (pos < n && (text[pos] == QLatin1Char('!') || text[pos] == QLatin1Char('?')
                       || text[pos] == QLatin1Char('~') || text[pos] == QLatin1Char('#'))) {
        if (keepWhitespace && text[pos] == QLatin1Char('#')) *keepWhitespace = true;
        ++pos;
    }
    return pos;
}

/// 找作用域宏的收尾标签 {{/name}}；把同名开标签算作一层嵌套
int findScopedClose(const QString &text, int from, const QString &name)
{
    const QString closeTag = QStringLiteral("{{/") + name + QStringLiteral("}}");
    int depth = 0;
    int i = from;
    const int n = text.size();
    while (i < n) {
        const int open = text.indexOf(QStringLiteral("{{"), i);
        if (open < 0) return -1;

        if (text.mid(open, closeTag.size()) == closeTag) {
            if (depth == 0) return open;
            --depth;
            i = open + closeTag.size();
            continue;
        }

        int p = skipFlags(text, open + 2);
        const int nameStart = p;
        while (p < n && isNameChar(text[p])) ++p;
        if (text.mid(nameStart, p - nameStart).toLower() == name && p < n
            && (text[p] == QLatin1Char(':') || text[p] == QLatin1Char('}')
                || text[p].isSpace())) {
            ++depth;
        }
        i = open + 2;
    }
    return -1;
}

/// 在深度 0 上按 {{else}} 切开 if 的两个分支
QStringList splitElse(const QString &content)
{
    const QString tag = QStringLiteral("{{else}}");
    int i = 0;
    const int n = content.size();
    while (i < n) {
        const int open = content.indexOf(QStringLiteral("{{"), i);
        if (open < 0) break;
        if (content.mid(open, tag.size()) == tag) {
            return { content.left(open), content.mid(open + tag.size()) };
        }

        // 跳过一层宏（作用域宏整体跳过）
        int p = skipFlags(content, open + 2);
        const int nameStart = p;
        while (p < n && isNameChar(content[p])) ++p;
        const QString name = content.mid(nameStart, p - nameStart).toLower();
        const int scopedClose = name.isEmpty() ? -1 : findScopedClose(content, open + 2, name);
        if (scopedClose >= 0) {
            i = scopedClose + 5 + name.size();
        } else {
            const int macroClose = content.indexOf(QStringLiteral("}}"), open + 2);
            i = (macroClose < 0) ? n : macroClose + 2;
        }
    }
    return { content };
}

/// ST 的 auto-trim：去首尾空行 + 按首个非空行的缩进整体左移 + 去行尾空白
QString trimScopedContent(const QString &raw, bool keepWhitespace)
{
    if (keepWhitespace) return raw;

    QStringList lines = raw.split(QLatin1Char('\n'));
    while (!lines.isEmpty() && lines.first().trimmed().isEmpty()) lines.removeFirst();
    while (!lines.isEmpty() && lines.last().trimmed().isEmpty()) lines.removeLast();
    if (lines.isEmpty()) return QString();

    int indent = 0;
    for (const QString &line : lines) {
        if (line.trimmed().isEmpty()) continue;
        indent = leadingSpaces(line);
        break;
    }

    QStringList out;
    out.reserve(lines.size());
    for (QString line : lines) {
        line = rtrimLine(line);
        if (indent > 0 && line.size() >= indent && line.left(indent).trimmed().isEmpty())
            line = line.mid(indent);
        out.append(line);
    }
    return out.join(QLatin1Char('\n'));
}

class Engine
{
public:
    Engine(MacroEnv &env, bool readOnly) : m_env(env), m_readOnly(readOnly) {}

    QString evalText(const QString &text)
    {
        QString out;
        int i = 0;
        const int n = text.size();
        while (i < n) {
            const int open = text.indexOf(QStringLiteral("{{"), i);
            if (open < 0) { out += text.mid(i); break; }
            out += text.mid(i, open - i);

            // 没有配对的 {{/name}} 要原样输出（ST 保留未知收尾标签）
            if (open + 2 < n && text[open + 1] == QLatin1Char('/')
                && text[open + 2] != QLatin1Char('/') && isNameStart(text[open + 2])) {
                const int close = text.indexOf(QStringLiteral("}}"), open + 2);
                if (close > 0) {
                    out += text.mid(open, close + 2 - open);
                    i = close + 2;
                    continue;
                }
            }


            Macro m;
            if (scanMacro(text, open, m) < 0) {
                // {{ 后面跟的不是宏名：ST 对 "{{{x}}" 这种只吞一个 '{'，对 "{{ x" 保留两个
                const QChar next = (open + 2 < n) ? text[open + 2] : QChar();
                if (next == QLatin1Char('{')) {
                    out += QLatin1Char('{');
                    i = open + 1;
                } else {
                    out += QStringLiteral("{{");
                    i = open + 2;
                }
                continue;
            }

            // 零参宏被判成作用域形式（{{user}}…{{/user}}）时，ST 会整体保留成原文
            if (!m.scoped && (isValueMacro(m.name) || m.name == QLatin1String("noop"))) {
                const int close = findScopedClose(text, m.end, m.name);
                if (close >= 0) {
                    m.scoped = true;
                    m.scopedContent = text.mid(m.end, close - m.end);
                    m.end = close + 5 + m.name.size();
                }
            }

            // {{trim}}（非作用域）：删掉两侧换行，和 ST 的老行为一致
            if (m.name == QLatin1String("trim") && !m.scoped) {
                while (!out.isEmpty()
                       && (out.back() == QLatin1Char('\n') || out.back() == QLatin1Char('\r')))
                    out.chop(1);
                int k = m.end;
                while (k < n && (text[k] == QLatin1Char('\n') || text[k] == QLatin1Char('\r'))) ++k;
                i = k;
                continue;
            }

            out += dispatch(m, text, open);
            i = m.end;
        }
        return out;
    }

private:
    int scanMacro(const QString &text, int pos, Macro &m);

    QString rawPreserve(const Macro &m, const QString &text, int pos) const
    {
        if (m.scoped) {
            const int tagLen = (m.openEnd > pos) ? (m.openEnd - pos) : 0;
            const QString openTag = (tagLen > 0) ? text.mid(pos, tagLen)
                                                 : (QStringLiteral("{{") + m.name + QStringLiteral("}}"));
            return openTag
                + const_cast<Engine *>(this)->evalText(m.scopedContent)
                + QStringLiteral("{{/") + m.name + QStringLiteral("}}");
        }
        const QString inner = text.mid(pos + 2, m.end - pos - 4);
        return QStringLiteral("{{") + const_cast<Engine *>(this)->evalText(inner)
            + QStringLiteral("}}");
    }

    QStringList resolvedArgs(const Macro &m)
    {
        QStringList args;
        for (const QString &raw : m.rawArgs)
            args.append(evalText(raw).trimmed());
        if (m.scoped)
            args.append(trimScopedContent(evalText(m.scopedContent), m.keepWhitespace));
        return args;
    }

    QString dispatch(const Macro &m, const QString &text, int pos);

    QString evalIf(const Macro &m, const QString &text, int pos);

    QString valueOf(const QString &name) const;

    bool truthiness(const QString &rawCond);

    QString setLocal(const QString &name, const QString &value)
    {
        if (!m_readOnly && m_env.localVars) m_env.localVars->insert(name, value);
        return QString();
    }

    MacroEnv &m_env;
    bool m_readOnly;
};

QString Engine::valueOf(const QString &name) const
{
    const auto it = m_env.values.constFind(name);
    if (it != m_env.values.constEnd()) return it.value();
    const auto lazy = m_env.lazyValues.constFind(name);
    if (lazy != m_env.lazyValues.constEnd()) return lazy.value();
    if (name == QLatin1String("newline")) return QStringLiteral("\n");
    if (name == QLatin1String("noop")) return QString();
    return QString();
}

int Engine::scanMacro(const QString &text, int pos, Macro &m)
{
    const int n = text.size();
    int j = pos + 2;
    if (j >= n) return -1;

    // {{// 注释}} / {{//}}…{{///}} 块注释
    if (text[j] == QLatin1Char('/') && j + 1 < n && text[j + 1] == QLatin1Char('/')) {
        const int close = text.indexOf(QStringLiteral("}}"), j + 2);
        if (close < 0) return -1;
        m.name = QStringLiteral("//");
        if (text.mid(j + 2, close - (j + 2)).trimmed().isEmpty()) {
            int scan = close + 2;
            while (scan < n) {
                const int open = text.indexOf(QStringLiteral("{{"), scan);
                if (open < 0) break;
                int p = open + 2;
                int slashes = 0;
                while (p < n && text[p] == QLatin1Char('/')) { ++slashes; ++p; }
                if (slashes >= 3 && text.mid(p, 2) == QLatin1String("}}")) {
                    m.scoped = true;
                    m.scopedContent = text.mid(close + 2, open - (close + 2));
                    m.end = p + 2;
                    return m.end;
                }
                scan = open + 2;
            }
        }
        m.end = close + 2;
        return m.end;
    }

    if (!isNameStart(text[j])
        && text[j] != QLatin1Char('!') && text[j] != QLatin1Char('?')
        && text[j] != QLatin1Char('~') && text[j] != QLatin1Char('#')) return -1;

    j = skipFlags(text, j, &m.keepWhitespace);
    if (j >= n || !isNameStart(text[j])) return -1;

    const int nameStart = j;
    while (j < n && isNameChar(text[j])) ++j;
    m.name = text.mid(nameStart, j - nameStart).toLower();

    int end = -1;
    if (j < n && text[j] == QLatin1Char(':')) {
        if (j + 1 < n && text[j + 1] == QLatin1Char(':')) {
            // "::" 分隔的多个参数
            int argStart = j + 2;
            int k = argStart;
            int depth = 0;
            while (k < n) {
                if (text.mid(k, 2) == QLatin1String("{{")) { ++depth; k += 2; continue; }
                if (text.mid(k, 2) == QLatin1String("}}")) {
                    if (depth == 0) {
                        m.rawArgs.append(text.mid(argStart, k - argStart));
                        end = k + 2;
                        break;
                    }
                    --depth;
                    k += 2;
                    continue;
                }
                if (depth == 0 && text.mid(k, 2) == QLatin1String("::")) {
                    m.rawArgs.append(text.mid(argStart, k - argStart));
                    k += 2;
                    argStart = k;
                    continue;
                }
                ++k;
            }
        } else {
            // 老写法：单冒号后面的整段算一个参数
            int k = j + 1;
            int depth = 0;
            while (k < n) {
                if (text.mid(k, 2) == QLatin1String("{{")) { ++depth; k += 2; continue; }
                if (text.mid(k, 2) == QLatin1String("}}")) {
                    if (depth == 0) {
                        m.rawArgs.append(text.mid(j + 1, k - (j + 1)));
                        end = k + 2;
                        break;
                    }
                    --depth;
                    k += 2;
                    continue;
                }
                ++k;
            }
        }
        if (end < 0) return -1;
    } else if (j < n && text[j] != QLatin1Char('}') && !text[j].isSpace()) {
        return -1;
    } else {
        int k = j;
        int depth = 0;
        while (k < n) {
            if (text.mid(k, 2) == QLatin1String("{{")) { ++depth; k += 2; continue; }
            if (text.mid(k, 2) == QLatin1String("}}")) {
                if (depth == 0) {
                    const QString rest = text.mid(j, k - j).trimmed();
                    if (!rest.isEmpty()) m.rawArgs.append(rest);
                    end = k + 2;
                    break;
                }
                --depth;
                k += 2;
                continue;
            }
            ++k;
        }
        if (end < 0) return -1;
    }

    m.end = end;
    m.openEnd = end;

    // 作用域形式 {{name}}…{{/name}}
    static const QSet<QString> scopable = {
        QStringLiteral("setvar"), QStringLiteral("setglobalvar"),
        QStringLiteral("reverse"), QStringLiteral("trim"), QStringLiteral("if"),
    };
    if (scopable.contains(m.name)) {
        const int close = findScopedClose(text, m.end, m.name);
        if (close >= 0) {
            m.scoped = true;
            m.scopedContent = text.mid(m.end, close - m.end);
            m.end = close + 5 + m.name.size();
        }
    }
    return m.end;
}

QString Engine::evalIf(const Macro &m, const QString &text, int pos)
{
    // 条件要用"没被求值过"的原文，否则 {{if {{getvar::x}}}} 里 x 的 "!" 会被误当成取反
    const QString condRaw = m.rawArgs.value(0);

    if (m.scoped) {
        const QStringList branches = splitElse(m.scopedContent);
        const bool condition = truthiness(condRaw);
        const QString chosen = condition ? branches.value(0) : branches.value(1);
        return trimScopedContent(evalText(chosen), m.keepWhitespace);
    }

    const QStringList args = resolvedArgs(m);
    if (args.isEmpty()) return rawPreserve(m, text, pos);
    const bool condition = truthiness(condRaw);
    if (args.size() >= 2 && condition) return args.value(1);
    return QString();
}

bool Engine::truthiness(const QString &rawCond)
{
    QString cond = rawCond.trimmed();
    bool negate = false;
    if (cond.startsWith(QLatin1Char('!'))) {
        negate = true;
        cond = cond.mid(1).trimmed();
    }

    bool truthy = false;
    if (cond.startsWith(QLatin1String("{{"))) {
        truthy = isTruthy(evalText(cond));
    } else {
        bool allNameChars = !cond.isEmpty();
        for (const QChar c : cond) {
            if (!isNameChar(c)) { allNameChars = false; break; }
        }
        if (allNameChars && (isValueMacro(cond) || cond == QLatin1String("newline")
                             || cond == QLatin1String("noop"))) {
            truthy = isTruthy(valueOf(cond));
        } else {
            truthy = isTruthy(evalText(cond));
        }
    }
    return negate ? !truthy : truthy;
}

QString Engine::dispatch(const Macro &m, const QString &text, int pos)
{
    const QString &name = m.name;

    if (name == QLatin1String("//")) return QString();
    if (name == QLatin1String("if")) return evalIf(m, text, pos);

    if (name == QLatin1String("trim") && m.scoped)
        return trimScopedContent(evalText(m.scopedContent), m.keepWhitespace);

    QStringList args = resolvedArgs(m);

    // 取值宏不接受参数：env 里给了什么名字就支持什么名字（{{lastChatMessage}} 之类
    // 是调用方临时塞进变量表的），isValueMacro 只是兜底（没给值时替换成空串）
    if (m_env.values.contains(name) || m_env.lazyValues.contains(name)) {
        if (!args.isEmpty()) return rawPreserve(m, text, pos);
        return valueOf(name);
    }
    if (isValueMacro(name)) {
        if (!args.isEmpty()) return rawPreserve(m, text, pos);
        return valueOf(name);
    }
    if (name == QLatin1String("noop")) return QString();
    if (name == QLatin1String("newline")) return QStringLiteral("\n");

    if (name == QLatin1String("reverse")) {
        if (args.size() != 1) return rawPreserve(m, text, pos);
        const QString value = args.value(0);
        QString reversed;
        reversed.reserve(value.size());
        for (int i = value.size() - 1; i >= 0; --i) reversed.append(value.at(i));
        return reversed;
    }

    if (name == QLatin1String("random") || name == QLatin1String("pick")) {
        if (args.isEmpty()) return rawPreserve(m, text, pos);
        const int idx = int(QRandomGenerator::global()->bounded(int(args.size())));
        return args.value(idx);
    }

    if (name == QLatin1String("roll")) {
        if (args.size() != 1) return rawPreserve(m, text, pos);
        QString formula = args.value(0).trimmed().toLower();
        formula.remove(QLatin1Char(' '));
        int bonus = 0;
        int bonusPos = formula.indexOf(QLatin1Char('+'));
        if (bonusPos < 0) bonusPos = formula.indexOf(QLatin1Char('-'), 1);
        if (bonusPos > 0) {
            bonus = formula.mid(bonusPos).toInt();
            formula = formula.left(bonusPos);
        }
        int count = 1;
        int faces = 0;
        const int dPos = formula.indexOf(QLatin1Char('d'));
        if (dPos < 0) {
            faces = formula.toInt();
            count = 0;
        } else {
            count = formula.left(dPos).toInt();
            faces = formula.mid(dPos + 1).toInt();
        }
        if (faces <= 0) return rawPreserve(m, text, pos);
        int total = bonus;
        for (int i = 0; i < count; ++i)
            total += 1 + int(QRandomGenerator::global()->bounded(faces));
        return QString::number(total);
    }

    // ---- 变量 ----
    const bool global = name.contains(QLatin1String("globalvar"));
    QHash<QString, QString> *store = global ? m_env.globalVars : m_env.localVars;

    auto readVar = [&](const QString &key) {
        if (!store) return QString();
        return store->value(key);
    };
    auto writeVar = [&](const QString &key, const QString &value) {
        if (!m_readOnly && store) store->insert(key, value);
    };

    if (name == QLatin1String("setvar") || name == QLatin1String("setglobalvar")) {
        if (args.size() != 2) return rawPreserve(m, text, pos);
        writeVar(args.value(0), args.value(1));
        return QString();
    }
    if (name == QLatin1String("getvar") || name == QLatin1String("getglobalvar")) {
        if (args.size() != 1) return rawPreserve(m, text, pos);
        return readVar(args.value(0));
    }
    if (name == QLatin1String("hasvar") || name == QLatin1String("varexists")
        || name == QLatin1String("hasglobalvar")) {
        if (args.size() != 1) return rawPreserve(m, text, pos);
        const bool exists = store && store->contains(args.value(0));
        return exists ? QStringLiteral("true") : QStringLiteral("false");
    }
    if (name == QLatin1String("addvar") || name == QLatin1String("addglobalvar")) {
        if (args.size() != 2) return rawPreserve(m, text, pos);
        const QString key = args.value(0);
        const QString add = args.value(1);
        const QString current = readVar(key);
        bool okA = false;
        bool okB = false;
        const double a = current.toDouble(&okA);
        const double b = add.toDouble(&okB);
        QString next;
        if (okA && okB) next = QString::number(a + b);
        else if (!okA && okB && current.trimmed().isEmpty()) next = add;
        else next = current + add;
        writeVar(key, next);
        return QString();
    }
    if (name == QLatin1String("incvar") || name == QLatin1String("decvar")
        || name == QLatin1String("incglobalvar") || name == QLatin1String("decglobalvar")) {
        if (args.size() != 1) return rawPreserve(m, text, pos);
        const QString key = args.value(0);
        const int delta = name.startsWith(QLatin1String("inc")) ? 1 : -1;
        const QString next = QString::number(readVar(key).toInt() + delta);
        writeVar(key, next);
        return m_readOnly ? QString() : next;
    }

    return rawPreserve(m, text, pos);
}

} // namespace

QString MacroEnv::lookup(const QString &name) const
{
    const auto it = values.constFind(name);
    if (it != values.constEnd()) return it.value();
    const auto lazy = lazyValues.constFind(name);
    if (lazy != lazyValues.constEnd()) return lazy.value();
    return QString();
}

static QString preprocessLegacyTags(const QString &content, const MacroEnv &env)
{
    QString text = content;
    if (!env.userName.isEmpty())
        text.replace(QStringLiteral("<USER>"), env.userName, Qt::CaseInsensitive);
    if (!env.charName.isEmpty()) {
        text.replace(QStringLiteral("<BOT>"), env.charName, Qt::CaseInsensitive);
        text.replace(QStringLiteral("<CHAR>"), env.charName, Qt::CaseInsensitive);
        text.replace(QStringLiteral("<CHARIFNOTGROUP>"), env.charName, Qt::CaseInsensitive);
        text.replace(QStringLiteral("<GROUP>"), env.charName, Qt::CaseInsensitive);
    }
    return text;
}

QString MacroEngine::evaluate(const QString &content, MacroEnv &env)
{
    if (content.isEmpty()) return content;
    Engine engine(env, false);
    return engine.evalText(preprocessLegacyTags(content, env));
}

QString MacroEngine::evaluateReadOnly(const QString &content, const MacroEnv &env)
{
    if (content.isEmpty()) return content;

    QHash<QString, QString> local = env.localVars ? *env.localVars : QHash<QString, QString>();
    QHash<QString, QString> global = env.globalVars ? *env.globalVars : QHash<QString, QString>();
    MacroEnv copy = env;
    copy.localVars = &local;
    copy.globalVars = &global;

    Engine engine(copy, true);
    return engine.evalText(preprocessLegacyTags(content, copy));
}
