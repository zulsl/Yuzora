#include "CompletionPresetViewModel.h"
#include "PresetItemModel.h"
#include "GlobalConstants.h"
#include "ScopeTimer.h"
#include "Utils.h"

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
            this, [this](const PromptEntity & entry) {
                m_service->updatePrompt(entry);
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

int CompletionPresetViewModel::presetDetailTab() const
{
    return m_presetDetailTab;
}

void CompletionPresetViewModel::setPresetDetailTab(int tab)
{
    if (tab != 0 && tab != 1) return;
    if (m_presetDetailTab == tab) return;

    m_presetDetailTab = tab;
    emit presetDetailTabChanged();
}

// ======================================
// 列表与选中管理
// ======================================
void CompletionPresetViewModel::refreshList()
{
    //刷新左侧整体列表
    QList<CompletionPresetEntity> entities = m_service->getAllPresets();

    m_presetModel->loadList(entities);
}

void CompletionPresetViewModel::selectPreset(const QString& presetId)
{
    if (m_currentEditingPreset.id == presetId) return;

    m_currentEditingPreset = m_service->getPresetById(presetId);

    // 切换预设时回到“基础参数”Tab
    if (m_presetDetailTab != 0) {
        m_presetDetailTab = 0;
        emit presetDetailTabChanged();
    }

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
    m_service->createPreset(newEntity);
    refreshList();
}

void CompletionPresetViewModel::deletePreset(const QString& presetId)
{
    bool success = m_service->deletePreset(presetId);
    if (success && m_currentEditingPreset.id == presetId) {
        m_currentEditingPreset = {};
        if (m_presetDetailTab != 0) {
            m_presetDetailTab = 0;
            emit presetDetailTabChanged();
        }
        emit currentEditingPresetChanged();
    }
    refreshList();
}

void CompletionPresetViewModel::duplicatePreset(const QString& presetId)
{
    QString newId = m_service->duplicatePreset(presetId);
    if (!newId.isEmpty()) {
        selectPreset(newId);
    }
}

void CompletionPresetViewModel::setActivePreset(const QString& presetId)
{
    bool success = m_service->setActivePreset(presetId);
    if (success && m_currentEditingPreset.id == presetId) {
        m_currentEditingPreset.isDefault = true;
        emit currentEditingPresetChanged();
    }
    refreshList();
}

void CompletionPresetViewModel::renamePreset(const QString& presetId, const QString& newName)
{
    bool success = m_service->renamePreset(presetId, newName);
    if (success && m_currentEditingPreset.id == presetId) {
        m_currentEditingPreset.name = newName.trimmed();
        emit currentEditingPresetChanged();
    }
    refreshList();
}

void CompletionPresetViewModel::applyEditingPreset(const CompletionPresetEntity& newPreset)
{
    if (!hasSelection()) return;


    m_service->updatePreset(newPreset);
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

    QList<PromptEntity> prompts = m_service->getPromptsByPresetId(m_currentEditingPreset.id);
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

    m_presetPromptListModel->loadList(items);
}

PromptEntity CompletionPresetViewModel::makeNewPrompt() const
{
    PromptEntity prompt;
    prompt.id = Utils::generateUuid();
    prompt.identifier = prompt.id;
    prompt.presetId = m_currentEditingPreset.id;
    prompt.name = QStringLiteral("新提示词");
    prompt.role = GlobalConstants::MessageType::System;
    prompt.content.clear();
    prompt.isActive = true;
    prompt.system_prompt = false;
    prompt.marker = false;
    prompt.extension = false;
    prompt.injection_position = 0;
    prompt.injection_depth = 4;
    prompt.injection_order = 100;
    return prompt;
}

void CompletionPresetViewModel::reloadCurrentPresetAndPrompts()
{
    if (m_currentEditingPreset.id.isEmpty()) return;

    // 提示词增删只改变 promptOrder；不要用数据库实体覆盖基础参数草稿。
    const CompletionPresetEntity fresh = m_service->getPresetById(m_currentEditingPreset.id);
    if (fresh.id.isEmpty()) return;

    m_currentEditingPreset.promptOrder = fresh.promptOrder;
    emit currentEditingPresetChanged();
    refreshPromptList();
}

void CompletionPresetViewModel::addPrompt()
{
    if (m_currentEditingPreset.id.isEmpty()) {
        emit operationError(QStringLiteral("请先选择预设"));
        return;
    }

    if (m_service->addPrompt(m_currentEditingPreset.id, makeNewPrompt()))
        reloadCurrentPresetAndPrompts();
}

void CompletionPresetViewModel::insertPromptBefore(const QString& beforeIdentifier)
{
    if (m_currentEditingPreset.id.isEmpty()) {
        emit operationError(QStringLiteral("请先选择预设"));
        return;
    }
    if (beforeIdentifier.isEmpty()) {
        addPrompt();
        return;
    }

    if (m_service->insertPromptBefore(m_currentEditingPreset.id, beforeIdentifier, makeNewPrompt()))
        reloadCurrentPresetAndPrompts();
}

void CompletionPresetViewModel::deletePrompt(const QString& promptId)
{
    if (m_currentEditingPreset.id.isEmpty() || promptId.isEmpty()) return;

    if (m_service->deletePrompt(m_currentEditingPreset.id, promptId))
        reloadCurrentPresetAndPrompts();
}


// ======================================
// 导入导出
// ======================================
void CompletionPresetViewModel::importFromFile(const QUrl& fileUrl)
{
    QString filePath = fileUrl.toLocalFile();
    QFileInfo fi(filePath);
    QString fileName = fi.completeBaseName();

    const int count = m_service->importFromSillyTavern(filePath, fileName);

    // 导入后必须刷新列表并通知界面：原先只调用了 Service，模型没有重载，
    // 新导入的预设不会出现在列表里，用户看到的就是"导入没反应"。
    if (count > 0) {
        refreshList();
        emit importSuccess(count);
    }
}

void CompletionPresetViewModel::exportCurrent(const QString& presetId, const QUrl& savePath)
{
    QString filePath = savePath.toLocalFile();
    m_service->exportToSillyTavern(presetId, filePath);
}

void CompletionPresetViewModel::exportAll(const QString& savePath)
{
    m_service->exportAllToSillyTavern(savePath);
}
