#include <QtTest>

#include "PresetPromptListModel.h"
#include "PromptEntity.h"

namespace {

PromptEntity makePrompt(const QString& id, bool marker, const QString& content)
{
    PromptEntity p;
    p.id = id;
    p.identifier = id;
    p.name = id;
    p.content = content;
    p.marker = marker;
    p.system_prompt = true;
    return p;
}

} // namespace

class PresetPromptListModelTest : public QObject {
    Q_OBJECT

private slots:
    // marker 必须以 role 的形式暴露给 QML，否则预设界面无法判断"系统占位"
    void markerRoleIsExposed()
    {
        PresetPromptListModel model;
        model.loadList({
            makePrompt(QStringLiteral("charDescription"), true, QString()),
            makePrompt(QStringLiteral("custom"), false, QStringLiteral("正文")),
        });

        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.roleNames().value(PresetPromptListModel::MarkerRole),
                 QByteArray("marker"));

        QCOMPARE(model.data(model.index(0), PresetPromptListModel::MarkerRole).toBool(), true);
        QCOMPARE(model.data(model.index(1), PresetPromptListModel::MarkerRole).toBool(), false);
        QCOMPARE(model.data(model.index(0), PresetPromptListModel::IdentifierRole).toString(),
                 QStringLiteral("charDescription"));
    }

    // 非占位条目仍然可以改内容（占位条目的拦截在 ViewModel 层）
    void nonMarkerContentIsUpdatable()
    {
        PresetPromptListModel model;
        model.loadList({makePrompt(QStringLiteral("custom"), false, QStringLiteral("旧"))});

        QVERIFY(model.updateField(QStringLiteral("custom"),
                                  PresetPromptListModel::ContentRole,
                                  QStringLiteral("新")));
        QCOMPARE(model.entryById(QStringLiteral("custom")).content, QStringLiteral("新"));
    }
};

QTEST_GUILESS_MAIN(PresetPromptListModelTest)
#include "PresetPromptListModelTest.moc"
