#include <QtTest>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

#include "MacroEngine.h"

// 语料来自 SillyTavern 1.19.0 自己的 e2e 用例：
//   tests/frontend/MacroEngine.e2e.js
// 已一次性抽取并固化在 tests/fixtures/st_macro_corpus.json（148 条），不再依赖本机 ST 安装。
// 这里拿它做差分测试：同一个输入，我们的引擎必须和 ST 输出一致。

namespace {

MacroEnv makeEnv()
{
    MacroEnv env;
    env.userName = QStringLiteral("User");
    env.charName = QStringLiteral("Character");
    env.values.insert(QStringLiteral("char"), QStringLiteral("Character"));
    env.values.insert(QStringLiteral("user"), QStringLiteral("User"));
    env.values.insert(QStringLiteral("group"), QStringLiteral("Character"));
    env.values.insert(QStringLiteral("charifnotgroup"), QStringLiteral("Character"));
    env.values.insert(QStringLiteral("description"), QString());
    env.values.insert(QStringLiteral("personality"), QString());
    env.values.insert(QStringLiteral("scenario"), QString());
    return env;
}

QString show(const QString &s)
{
    QString out;
    for (const QChar c : s) {
        if (c == QLatin1Char('\n')) out += QStringLiteral("\\n");
        else if (c == QLatin1Char('\r')) out += QStringLiteral("\\r");
        else if (c == QLatin1Char('\t')) out += QStringLiteral("\\t");
        else out += c;
    }
    return out;
}

} // namespace

class MacroEngineTest : public QObject
{
    Q_OBJECT

private slots:
    void stCorpus()
    {
        QFile file(QStringLiteral(TEST_FIXTURE_DIR "/st_macro_corpus.json"));
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.fileName()));
        const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
        const QJsonArray cases = root.value(QStringLiteral("cases")).toArray();
        QVERIFY(!cases.isEmpty());

        int failed = 0;
        QStringList failures;
        for (const QJsonValue &value : cases) {
            const QJsonObject item = value.toObject();
            const QString name = item.value(QStringLiteral("name")).toString();
            const QString input = item.value(QStringLiteral("input")).toString();
            const QString expected = item.value(QStringLiteral("expected")).toString();

            QHash<QString, QString> local;
            QHash<QString, QString> global;
            MacroEnv env = makeEnv();
            env.localVars = &local;
            env.globalVars = &global;

            const QString actual = MacroEngine::evaluate(input, env);
            if (actual != expected) {
                ++failed;
                if (failures.size() < 25) {
                    failures.append(QStringLiteral("  ✗ %1\n      in  : %2\n      want: %3\n      got : %4")
                                        .arg(name, show(input), show(expected), show(actual)));
                }
            }
        }

        const int passed = cases.size() - failed;
        qInfo("ST 宏语料：%d/%d 通过（%d 失败）", passed, cases.size(), failed);
        for (const QString &line : failures) qInfo().noquote() << line;
        QVERIFY2(failed == 0, qPrintable(QStringLiteral("%1 / %2 条用例与 ST 不一致")
                                            .arg(failed).arg(cases.size())));
    }

    // 变量在同一份 env 里跨多次调用是连续的（对应 ST 的聊天级变量）
    void variablesPersistAcrossCalls()
    {
        QHash<QString, QString> local;
        MacroEnv env = makeEnv();
        env.localVars = &local;

        QCOMPARE(MacroEngine::evaluate(QStringLiteral("{{setvar::hp::10}}"), env), QString());
        QCOMPARE(MacroEngine::evaluate(QStringLiteral("{{addvar::hp::5}}{{getvar::hp}}"), env),
                 QStringLiteral("15"));
        QCOMPARE(MacroEngine::evaluate(QStringLiteral("{{incvar::hp}}"), env), QStringLiteral("16"));
        QCOMPARE(MacroEngine::evaluate(QStringLiteral("{{decvar::hp}}"), env), QStringLiteral("15"));
        QCOMPARE(local.value(QStringLiteral("hp")), QStringLiteral("15"));
    }

    // 这条是用户实际遇到的写法：注释里写中文说明，宏只做副作用
    void userReportedCase()
    {
        QHash<QString, QString> local;
        MacroEnv env = makeEnv();
        env.localVars = &local;

        const QString content = QStringLiteral(
            "{{setvar::AI_role::A.U.T.O.}} {{//助手名称，可更改}}{{getvar::AI_role}}");

        QCOMPARE(MacroEngine::evaluate(content, env), QStringLiteral(" A.U.T.O."));
        QCOMPARE(local.value(QStringLiteral("AI_role")), QStringLiteral("A.U.T.O."));
    }

    // 只读模式不改变量（给"预览"用）
    void readOnlyDoesNotMutateVariables()
    {
        QHash<QString, QString> local;
        MacroEnv env = makeEnv();
        env.localVars = &local;

        QCOMPARE(MacroEngine::evaluateReadOnly(QStringLiteral("{{setvar::x::1}}"), env), QString());
        QVERIFY(local.isEmpty());
    }
};

QTEST_MAIN(MacroEngineTest)
#include "MacroEngineTest.moc"
