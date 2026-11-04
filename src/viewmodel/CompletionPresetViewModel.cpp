#include "CompletionPresetViewModel.h"
#include "PresetItemModel.h"
#include "GlobalConstants.h"
#include "ScopeTimer.h"

#include <QFileInfo>

CompletionPresetViewModel::CompletionPresetViewModel(QObject* parent)
    : QObject(parent)
    , m_service(&CompletionPresetService::instance())
    , m_presetModel(new PresetListModel(this))
    , m_presetPromptListModel(new PresetPromptListModel(this))
{

    // 绑定 Service 层信号
    // connect(m_service, &CompletionPresetService::presetListChanged,
    //         this, &CompletionPresetViewModel::refreshList);
    connect(m_service, &CompletionPresetService::operationError,
            this, &CompletionPresetViewModel::operationError);
    connect(m_service, &CompletionPresetService::exportSuccess,
            this, &CompletionPresetViewModel::exportSuccess);

    // Model 的业务字段变化 → 通知 Service 落盘
    connect(m_presetPromptListModel, &PresetPromptListModel::entryWriting,
            this, [](const PromptEntity & entry) {
                CompletionPresetService::instance().updatePrompt(entry);
            });

    // 初始化加载列表
    refreshList();
}

CompletionPresetViewModel::~CompletionPresetViewModel() = default;

// ======================================
// 属性读取
// ======================================
PresetListModel* CompletionPresetViewModel::presetModel() const
{
    return m_presetModel;
}

PresetPromptListModel *CompletionPresetViewModel::presetPromptModel() const
{
    return m_presetPromptListModel;
}

CompletionPresetEntity CompletionPresetViewModel::currentEditingPreset() const
{
    return m_currentEditingPreset;
}

bool CompletionPresetViewModel::hasSelection() const
{
    return !m_currentEditingPreset.id.isEmpty();
}

// ======================================
// 列表与选中管理
// ======================================
void CompletionPresetViewModel::refreshList()
{
    //刷新左侧整体列表
    QList<CompletionPresetEntity> entities = CompletionPresetService::instance().getAllPresets();

    m_presetModel->loadList(entities);
}

void CompletionPresetViewModel::selectPreset(const QString& presetId)
{
    if (m_currentEditingPreset.id == presetId) return;

    m_currentEditingPreset = CompletionPresetService::instance().getPresetById(presetId);
    emit currentEditingPresetChanged();

    refreshPromptList();

}

// ======================================
// 增删改基础操作
// ======================================
void CompletionPresetViewModel::createPreset()
{
    CompletionPresetEntity newEntity;
    newEntity.name="新预设";
    CompletionPresetService::instance().createPreset(newEntity);
    refreshList();
}

void CompletionPresetViewModel::deletePreset(const QString& presetId)
{
    bool success = CompletionPresetService::instance().deletePreset(presetId);
    if (success && m_currentEditingPreset.id == presetId) {
        m_currentEditingPreset = {};
        emit currentEditingPresetChanged();
    }
    refreshList();
}

void CompletionPresetViewModel::duplicatePreset(const QString& presetId)
{
    QString newId = CompletionPresetService::instance().duplicatePreset(presetId);
    if (!newId.isEmpty()) {
        selectPreset(newId);
    }
}

void CompletionPresetViewModel::setActivePreset(const QString& presetId)
{
    bool success = CompletionPresetService::instance().setActivePreset(presetId);
    if (success && m_currentEditingPreset.id == presetId) {
        m_currentEditingPreset.isDefault = true;
        emit currentEditingPresetChanged();
    }
    refreshList();
}

void CompletionPresetViewModel::renamePreset(const QString& presetId, const QString& newName)
{
    bool success = CompletionPresetService::instance().renamePreset(presetId, newName);
    if (success && m_currentEditingPreset.id == presetId) {
        m_currentEditingPreset.name = newName.trimmed();
        emit currentEditingPresetChanged();
    }
    refreshList();
}

void CompletionPresetViewModel::applyEditingPreset(const CompletionPresetEntity& newPreset)
{
    if (!hasSelection()) return;


    CompletionPresetService::instance().updatePreset(newPreset);
    m_currentEditingPreset = newPreset;
    emit currentEditingPresetChanged();


    // 3. 持久化保存（写入文件/数据库，原有 updatePreset 的持久化逻辑移到此处）
    // ... 原有持久化代码 ...

    refreshList();
    emit operationTip("预设已保存");
}

void CompletionPresetViewModel::refreshPromptList()
{
    if (m_currentEditingPreset.id.isEmpty()) return;

    QList<PromptEntity> prompts = CompletionPresetService::instance().getPromptsByPresetId(m_currentEditingPreset.id);
    QStringList promptOrder=m_currentEditingPreset.promptOrder;


    //建立 id -> PromptEntity 的哈希索引
    QHash<QString, PromptEntity> promptLookup;
    promptLookup.reserve(prompts.size());
    for (const PromptEntity & p : prompts) {
        promptLookup.insert(p.identifier, p);
    }
    QList<PromptEntity> items;
    items.reserve(promptOrder.size());
    //按 promptOrder 顺序遍历，直接查哈希
    for (const auto& identifier : promptOrder) {
        auto it = promptLookup.find(identifier);//返回迭代器
        if (it != promptLookup.end()) {//找到
            items.append(it.value());
        } else {
            qWarning() <<QString("%1%2")
                .arg("[CompletionPresetViewModel::refreshPromptList]未找到 identifier:").arg(identifier);
        }
    }


    // //按 promptOrder 顺序遍历，直接查哈希
    // QList<PresetPromptItemModel*> items;
    // items.reserve(promptOrder.size());
    // for (const auto& orderEntry : promptOrder) {
    //     // auto it = promptLookup.find(orderEntry.identifier);//返回迭代器
    //     auto it = promptLookup.find(orderEntry);//返回迭代器
    //     if (it != promptLookup.end()) {//找到
    //         items.append(new PresetPromptItemModel(it.value()));
    //     }
    // }
    m_presetPromptListModel->loadList(items);
}


// ======================================
// 导入导出
// ======================================
void CompletionPresetViewModel::importFromFile(const QUrl& fileUrl)
{
    qDebug()<<"导入文件url:["<<fileUrl<<"]";
    QString filePath = fileUrl.toLocalFile();
    QFileInfo fi(fileUrl.toLocalFile());
    QString fileName = fi.completeBaseName();
    qDebug()<<"文件名:["<<fileName<<"]";
    CompletionPresetService::instance().importFromSillyTavern(filePath,fileName);
}

void CompletionPresetViewModel::exportCurrent(const QString& presetId, const QString& savePath)
{
    CompletionPresetService::instance().exportToSillyTavern(presetId, savePath);
}

void CompletionPresetViewModel::exportAll(const QString& savePath)
{
    CompletionPresetService::instance().exportAllToSillyTavern(savePath);
}
