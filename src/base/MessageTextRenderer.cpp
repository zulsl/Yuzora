#include "MessageTextRenderer.h"

#include <QRegularExpression>

QString MessageTextRenderer::render(const QString& raw,
                                    const QString& userName,
                                    const QString& charName)
{
    if (raw.isEmpty())
        return raw;

    // 与 PromptBuilder 的 {{user}} 兜底保持一致；角色名缺失时保留占位符，
    // 避免界面上突然出现一个空白名字。
    const QString trimmedUser = userName.trimmed();
    const QString resolvedUser = trimmedUser.isEmpty()
        ? QStringLiteral("User") : trimmedUser;
    const QString trimmedChar = charName.trimmed();

    // 一次扫描完成替换：
    //   - 不会把用户名里的 {{char}} 再当成宏展开；
    //   - {{User}} / <user> 这类大小写差异也能识别；
    //   - 未知宏原样保留（{{getvar}}、{{random}} 等不在这里执行）。
    static const QRegularExpression re(
        QStringLiteral(R"(\{\{(user|notchar|char|charifnotgroup|group|groupnotmuted)\}\}|<(user|bot|char|charifnotgroup|group)>)"),
        QRegularExpression::CaseInsensitiveOption);

    QString result;
    result.reserve(raw.size() + 32);

    int tail = 0;
    QRegularExpressionMatchIterator it = re.globalMatch(raw);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        result += raw.mid(tail, match.capturedStart() - tail);

        const QString name = (match.captured(1).isEmpty()
                                  ? match.captured(2)
                                  : match.captured(1)).toLower();
        const bool isUserMacro = (name == QLatin1String("user")
                                  || name == QLatin1String("notchar"));

        if (isUserMacro)
            result += resolvedUser;
        else if (!trimmedChar.isEmpty())
            result += trimmedChar;
        else
            result += match.captured(0);

        tail = match.capturedEnd();
    }
    result += raw.mid(tail);
    return result;
}
