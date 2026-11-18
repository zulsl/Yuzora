#include <QtTest>

#include "MessageTextRenderer.h"

class MessageTextRendererTest : public QObject
{
    Q_OBJECT

private slots:
    void replacesIdentityMacros();
    void replacesLegacyTags();
    void keepsUnknownMacros();
    void doesNotExpandInsertedNames();
    void fallsBackWhenNamesMissing();
};

void MessageTextRendererTest::replacesIdentityMacros()
{
    QCOMPARE(MessageTextRenderer::render(
                 QStringLiteral("{{user}} 看向 {{char}}，{{charIfNotGroup}} 笑了。"),
                 QStringLiteral("小林"), QStringLiteral("雪乃")),
             QStringLiteral("小林 看向 雪乃，雪乃 笑了。"));
}

void MessageTextRendererTest::replacesLegacyTags()
{
    QCOMPARE(MessageTextRenderer::render(
                 QStringLiteral("<USER> 和 <BOT> / <CHAR> / <GROUP>"),
                 QStringLiteral("小林"), QStringLiteral("雪乃")),
             QStringLiteral("小林 和 雪乃 / 雪乃 / 雪乃"));
}

void MessageTextRendererTest::keepsUnknownMacros()
{
    // 显示层不执行有状态宏，避免看一条消息就改变随机数/变量。
    const QString raw = QStringLiteral(
        "{{getvar::mood}} {{random::a::b}} {{description}} {{user}}");
    QCOMPARE(MessageTextRenderer::render(raw, QStringLiteral("小林"), QStringLiteral("雪乃")),
             QStringLiteral("{{getvar::mood}} {{random::a::b}} {{description}} 小林"));
}

void MessageTextRendererTest::doesNotExpandInsertedNames()
{
    // 用户名本身带宏字样时，不能被二次展开。
    QCOMPARE(MessageTextRenderer::render(
                 QStringLiteral("{{user}}"), QStringLiteral("{{char}}"), QStringLiteral("雪乃")),
             QStringLiteral("{{char}}"));
}

void MessageTextRendererTest::fallsBackWhenNamesMissing()
{
    QCOMPARE(MessageTextRenderer::render(
                 QStringLiteral("{{user}}/{{char}}"), QString(), QString()),
             QStringLiteral("User/{{char}}"));
}

QTEST_APPLESS_MAIN(MessageTextRendererTest)
#include "MessageTextRendererTest.moc"
