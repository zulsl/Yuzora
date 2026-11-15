#ifndef COMPLETIONPRESETVIEWMODEL_H
#define COMPLETIONPRESETVIEWMODEL_H

#include <QObject>
#include <QVariantMap>
#include <QHash>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>
#include <QTimer>
#include "PresetListModel.h"
#include "PresetPromptListModel.h"
#include "CompletionPresetEntity.h"
#include "PromptEntity.h"
#include "CompletionPresetService.h"

class QJSEngine;
class QTimer;
class CompletionPresetDraft;

/**
 * @brief 对话补全预设视图模型
 * 职责：预设列表 / 提示词条目列表两个数据源，当前编辑中的预设快照，以及导入导出。
 * 本类以 QML 单例形式暴露给 QML（类型名 CompletionPresetViewModel）。
 * 注意：对外暴露的是 currentPreset；历史 QML 里误写的 currentPreset 并不存在。
 */
class CompletionPresetViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // QML 单例工厂（供 QML 使用）：没有 create() 时单例会被标记为 isCreatable=false。
    static CompletionPresetViewModel* create(QQmlEngine*, QJSEngine*)
    {
        static CompletionPresetViewModel instance;
        // 生命周期归 C++：静态实例必须由我们销毁，否则退出时被销毁两次导致卡死
        QQmlEngine::setObjectOwnership(&instance, QQmlEngine::CppOwnership);
        return &instance;
    }
    //列表模型
    Q_PROPERTY(PresetListModel* presetModel READ presetModel CONSTANT)
    Q_PROPERTY(PresetPromptListModel* presetPromptModel READ presetPromptModel CONSTANT)

    //当前预设（只读，用于列表、删除、设为默认等操作）
    Q_PROPERTY(CompletionPresetEntity currentPreset READ currentPreset NOTIFY currentPresetChanged)

    // 编辑草稿：强类型 QObject，QML 直接读写；ViewModel 负责 dirty 和保存
    Q_PROPERTY(CompletionPresetDraft* presetDraft READ presetDraft NOTIFY presetDraftChanged)
    Q_PROPERTY(bool presetDirty READ presetDirty NOTIFY presetDirtyChanged)

    //有选择
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY currentPresetChanged)
    //预设详情的当前 Tab：0=基础参数，1=提示词条目
    Q_PROPERTY(int presetDetailTab READ presetDetailTab WRITE setPresetDetailTab
               NOTIFY presetDetailTabChanged)

public:
    ~CompletionPresetViewModel() override;

    // 属性读取
    PresetListModel* presetModel() const;
    PresetPromptListModel* presetPromptModel() const;
    CompletionPresetEntity currentPreset() const;
    CompletionPresetDraft* presetDraft() const;
    bool presetDirty() const;
    bool hasSelection() const;
    int presetDetailTab() const;
    void setPresetDetailTab(int tab);

    //预设操作
    Q_INVOKABLE void refreshList();//刷新列表
    Q_INVOKABLE void selectPreset(const QString& presetId);//选择
    Q_INVOKABLE void createPreset();//新建
    Q_INVOKABLE void deletePreset(const QString& presetId);//删除
    Q_INVOKABLE void setActivePreset(const QString& presetId);//设置激活
    Q_INVOKABLE void renamePreset(const QString& presetId, const QString& newName);//重命名
    Q_INVOKABLE void setMaxContextToken(const QVariant& value);//设置最大上下文
    Q_INVOKABLE void setMaxResponseToken(const QVariant& value);
    Q_INVOKABLE void setStreamEnabled(const QVariant& value);//设置流式
    Q_INVOKABLE void setTopP(const QVariant& value);
    Q_INVOKABLE void setTopK(const QVariant& value);
    //(弃用)
    Q_INVOKABLE void setPresetDraftField(const QString& field, const QVariant& value);
    Q_INVOKABLE bool savePresetDraft();
    Q_INVOKABLE void discardPresetDraft();
    Q_INVOKABLE void duplicatePreset(const QString& presetId);//复制(弃用,保留)


    //提示词条目操作
    Q_INVOKABLE void refreshPromptList();//刷新列表
    Q_INVOKABLE void addPrompt();//追加到末尾
    Q_INVOKABLE void insertPromptBefore(const QString& beforeIdentifier);//插入到选中项之前
    Q_INVOKABLE void deletePrompt(const QString& promptId);//删除非系统提示词
    Q_INVOKABLE void setPromptField(const QString& promptId, const QString& field,
                                    const QVariant& value);
    Q_INVOKABLE void flushPendingPromptEdits();


    // 导入导出
    Q_INVOKABLE void importFromFile(const QUrl& fileUrl);//导入预设
    Q_INVOKABLE void exportCurrent(const QString& presetId, const QUrl& savePath);//导出当前
    Q_INVOKABLE void exportAll(const QString& savePath);//弃用

signals:
    void importSuccess(int count);
    void exportSuccess();
    void operationError(const QString& errorMsg);
    void operationTip(const QString& tipMsg);
    void currentPresetChanged();
    void presetDraftChanged();
    void presetDirtyChanged();
    void presetDetailTabChanged();

private:
    CompletionPresetService* m_service = nullptr;

    PresetListModel* m_presetModel = nullptr;
    PresetPromptListModel* m_presetPromptListModel = nullptr;


    CompletionPresetEntity m_currentPreset;
    CompletionPresetDraft* m_presetDraft = nullptr;
    CompletionPresetEntity m_presetBaseline;
    bool m_presetDirty = false;
    int m_presetDetailTab = 0;

    PromptEntity makeNewPrompt() const;
    void reloadCurrentPresetAndPrompts();
    void resetPresetDraft(const CompletionPresetEntity& entity);
    // 即时保存成功后：草稿同步该字段 + 基线前移，避免"保存参数"把旧值写回去
    void syncDraftWithSavedField(const QString& field, const QVariant& value);
    CompletionPresetEntity draftToPreset() const;
    int roleForPromptField(const QString& field) const;

    QTimer* m_promptSaveTimer = nullptr;
    QHash<QString, QVariantMap> m_pendingPromptFields;

    // 构造函数必须 private：QML 单例只有在"无法默认构造"时才会调用 create()。
    // 若保持 public，引擎会自己 new 一个实例，C++ 侧的静态实例就与 QML 拿到的
    // 不是同一个对象（会出现"数据写进 A、界面读的是 B"这类极难定位的问题）。
    explicit CompletionPresetViewModel(QObject* parent = nullptr);

};

#endif