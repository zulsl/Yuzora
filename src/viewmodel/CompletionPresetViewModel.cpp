#include "CompletionPresetViewModel.h"
#include "CompletionPresetDraft.h"
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
    , m_presetDraft(new CompletionPresetDraft(this))
{
    connect(m_presetDraft, &CompletionPresetDraft::changed, this, [this]() {
        const bool dirty = !m_presetDraft->equals(m_presetBaseline);
        if (m_presetDirty != dirty) {
            m_presetDirty = dirty;
            emit presetDirtyChanged();
        }
        emit presetDraftChanged();
    });

    // 绑定 Service 层信号
    connect(m_service, &CompletionPresetService::operationError,
            this, &CompletionPresetViewModel::operationError);
    connect(m_service, &CompletionPresetService::exportSuccess,
            this, &CompletionPresetViewModel::exportSuccess);

    // Model 的业务字段变化不再直接落盘；由 setPromptField + 定时器统一提交。
    m_promptSaveTimer = new QTimer(this);
    m_promptSaveTimer->setSingleShot(true);
    m_promptSaveTimer->setInterval(300);
    connect(m_promptSaveTimer, &QTimer::timeout,
            this, &CompletionPresetViewModel::flushPendingPromptEdits);

    // 初始化加载列表
    refreshList();

    // 首启时把"正在编辑的预设"也指向当前生效的那条。
    // 否则 currentPreset 一直是默认构造的空实体，要等用户点开预设页
    // （PresetListPanel 里点某一行才调 selectPreset）才会被填上，
    // 期间任何绑 currentPreset 的界面（比如聊天页的副标题）读到的都是空。
    const CompletionPresetEntity active = m_service->getActivePreset();
    if (!active.id.isEmpty())
        selectPreset(active.id);
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

CompletionPresetEntity CompletionPresetViewModel::currentPreset() const
{
    return m_currentPreset;
}

CompletionPresetEntity CompletionPresetViewModel::activePreset() const
{
    return m_activePreset;
}

void CompletionPresetViewModel::refreshActivePreset()
{
    const CompletionPresetEntity active = m_service->getActivePreset();

    // 只比较展示会用到的字段，避免每次刷新列表都白发一次信号
    if (active.id == m_activePreset.id && active.name == m_activePreset.name)
        return;

    m_activePreset = active;
    emit activePresetChanged();
}

CompletionPresetDraft* CompletionPresetViewModel::presetDraft() const
{
    return m_presetDraft;
}

bool CompletionPresetViewModel::presetDirty() const
{
    return m_presetDirty;
}

bool CompletionPresetViewModel::hasSelection() const
{
    return !m_currentPreset.id.isEmpty();
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

    // 生效预设跟着一起刷新：聊天页读的是它，不能等用户开预设页才更新
    refreshActivePreset();
}

void CompletionPresetViewModel::selectPreset(const QString& presetId)
{
    if (m_currentPreset.id == presetId) return;

    flushPendingPromptEdits();
    m_currentPreset = m_service->getPresetById(presetId);

    // 切换预设时回到“基础参数”Tab
    if (m_presetDetailTab != 0) {
        m_presetDetailTab = 0;
        emit presetDetailTabChanged();
    }

    emit currentPresetChanged();

    refreshPromptList();

    // 选中预设后必须把草稿载入：否则采样参数显示的是草稿默认值，
    // 而且草稿里没有 id，"保存参数"会直接校验失败（表现为点了没反应）。
    resetPresetDraft(m_currentPreset);
}

// ======================================
// 增删改基础操作
// ======================================
void CompletionPresetViewModel::createPreset()
{
    CompletionPresetEntity newEntity;
    newEntity.id = Utils::generateUuid();
    newEntity.name = "新预设";
    m_service->CreateDefaultPromptEntries(newEntity.id,newEntity.promptOrder);

    const QString newId = m_service->createPreset(newEntity);
    if (newId.isEmpty()) {
        return;
    }

    refreshList();
    selectPreset(newId);
}

void CompletionPresetViewModel::deletePreset(const QString& presetId)
{
    bool success = m_service->deletePreset(presetId);
    if (success && m_currentPreset.id == presetId) {
        m_currentPreset = {};
        if (m_presetDetailTab != 0) {
            m_presetDetailTab = 0;
            emit presetDetailTabChanged();
        }
        emit currentPresetChanged();
        resetPresetDraft(CompletionPresetEntity{});
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
    if (success && m_currentPreset.id == presetId) {
        m_currentPreset.isActive = true;
        emit currentPresetChanged();
    }
    refreshList();
}

void CompletionPresetViewModel::renamePreset(const QString& presetId, const QString& newName)
{
    bool success = m_service->renamePreset(presetId, newName);
    if (success && m_currentPreset.id == presetId) {
        m_currentPreset.name = newName.trimmed();
        syncDraftWithSavedField(QStringLiteral("name"), m_currentPreset.name);
        emit currentPresetChanged();
    }
    refreshList();
}

void CompletionPresetViewModel::setMaxContextToken(const QVariant &value)
{
    if(m_currentPreset.maxContextToken == value.toInt()) return;
    CompletionPresetEntity newPreset = m_currentPreset;
    newPreset.maxContextToken = value.toInt();
    if (!m_service->updatePreset(newPreset)) return;

    m_currentPreset = newPreset;
    emit currentPresetChanged();
}

void CompletionPresetViewModel::setMaxResponseToken(const QVariant &value)
{
    if(m_currentPreset.maxResponseToken == value.toInt()) return;
    CompletionPresetEntity newPreset = m_currentPreset;
    newPreset.maxResponseToken = value.toInt();
    if (!m_service->updatePreset(newPreset)) return;

    m_currentPreset = newPreset;
    emit currentPresetChanged();
}

void CompletionPresetViewModel::setStreamEnabled(const QVariant &value)
{
    if(m_currentPreset.streamEnabled==value.toBool()) return;
    CompletionPresetEntity newPreset = m_currentPreset;
    newPreset.streamEnabled = value.toBool();
    if (!m_service->updatePreset(newPreset)) return;
    m_currentPreset = newPreset;
    emit currentPresetChanged();
}

void CompletionPresetViewModel::setTopP(const QVariant &value)
{
    if(m_currentPreset.topP==value.toDouble()) return;
    CompletionPresetEntity newPreset=m_currentPreset;
    newPreset.topP=value.toDouble();
    m_service->updatePreset(newPreset);
    m_currentPreset = newPreset;
    emit currentPresetChanged();
}

void CompletionPresetViewModel::setTopK(const QVariant &value)
{
    if(m_currentPreset.topK==value.toDouble()) return;
    CompletionPresetEntity newPreset=m_currentPreset;
    newPreset.topK=value.toDouble();
    m_service->updatePreset(newPreset);
    m_currentPreset = newPreset;
    emit currentPresetChanged();
}

void CompletionPresetViewModel::resetPresetDraft(const CompletionPresetEntity& entity)
{
    m_presetBaseline = entity;
    m_presetDraft->setEntity(entity);
    m_presetDirty = false;
    emit presetDraftChanged();
    emit presetDirtyChanged();
}

void CompletionPresetViewModel::syncDraftWithSavedField(const QString& field, const QVariant& value)
{
    if (!m_presetDraft) return;

    // 草稿里也写一份：否则之后点"保存参数"会用草稿里的旧值把刚保存的字段覆盖回去
    m_presetDraft->setField(field, value);

    // 刚保存的字段不该算"未保存改动"：基线前移到当前已保存实体
    m_presetBaseline = m_currentPreset;
    const bool dirty = !m_presetDraft->equals(m_presetBaseline);
    if (m_presetDirty != dirty) {
        m_presetDirty = dirty;
        emit presetDirtyChanged();
    }
    emit presetDraftChanged();
}

CompletionPresetEntity CompletionPresetViewModel::draftToPreset() const
{
    return m_presetDraft->toEntity(m_currentPreset);
}

void CompletionPresetViewModel::setPresetDraftField(const QString& field, const QVariant& value)
{
    // 与 QML 表单的字段一一对应：采样参数 + 基础参数都走草稿。
    // 之前白名单被收窄成只有两个惩罚项，导致温度 / Top-P / Top-K 的滑杆
    // 改了也不进草稿、点保存也不会写库（表现为"滑杆没反应"）。
    static const QStringList editableFields = {
        QStringLiteral("name"),
        QStringLiteral("maxContextToken"),
        QStringLiteral("maxResponseToken"),
        QStringLiteral("streamEnabled"),
        QStringLiteral("temperature"),
        QStringLiteral("topP"),
        QStringLiteral("topK"),
        QStringLiteral("frequencyPenalty"),
        QStringLiteral("presencePenalty")
    };

    if (!editableFields.contains(field)) {
        return;
    }

    m_presetDraft->setField(field, value);
}

bool CompletionPresetViewModel::savePresetDraft()
{
    if (!hasSelection()) {
        return false;
    }

    const CompletionPresetEntity entity = draftToPreset();
    if (!m_service->updatePreset(entity)) {
        return false;
    }

    m_currentPreset = entity;
    resetPresetDraft(entity);
    refreshList();
    emit operationTip(QStringLiteral("预设已保存"));
    return true;
}

void CompletionPresetViewModel::discardPresetDraft()
{
    resetPresetDraft(m_currentPreset);
}

void CompletionPresetViewModel::refreshPromptList()
{
    if (m_currentPreset.id.isEmpty()) return;

    QList<PromptEntity> prompts = m_service->getPromptsByPresetId(m_currentPreset.id);
    QStringList promptOrder=m_currentPreset.promptOrder;


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
    prompt.presetId = m_currentPreset.id;
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
    if (m_currentPreset.id.isEmpty()) return;

    // 提示词增删只改变 promptOrder；不要用数据库实体覆盖基础参数草稿。
    const CompletionPresetEntity fresh = m_service->getPresetById(m_currentPreset.id);
    if (fresh.id.isEmpty()) return;

    m_currentPreset.promptOrder = fresh.promptOrder;
    emit currentPresetChanged();
    refreshPromptList();
}

void CompletionPresetViewModel::addPrompt()
{
    if (m_currentPreset.id.isEmpty()) {
        emit operationError(QStringLiteral("请先选择预设"));
        return;
    }

    flushPendingPromptEdits();
    if (m_service->addPrompt(m_currentPreset.id, makeNewPrompt()))
        reloadCurrentPresetAndPrompts();
}

void CompletionPresetViewModel::insertPromptBefore(const QString& beforeIdentifier)
{
    if (m_currentPreset.id.isEmpty()) {
        emit operationError(QStringLiteral("请先选择预设"));
        return;
    }
    if (beforeIdentifier.isEmpty()) {
        addPrompt();
        return;
    }

    flushPendingPromptEdits();
    if (m_service->insertPromptBefore(m_currentPreset.id, beforeIdentifier, makeNewPrompt()))
        reloadCurrentPresetAndPrompts();
}

void CompletionPresetViewModel::deletePrompt(const QString& promptId)
{
    if (m_currentPreset.id.isEmpty() || promptId.isEmpty()) return;

    flushPendingPromptEdits();
    if (m_service->deletePrompt(m_currentPreset.id, promptId))
        reloadCurrentPresetAndPrompts();
}

int CompletionPresetViewModel::roleForPromptField(const QString& field) const
{
    if (field == QStringLiteral("name")) return PresetPromptListModel::NameRole;
    if (field == QStringLiteral("content")) return PresetPromptListModel::ContentRole;
    if (field == QStringLiteral("role")) return PresetPromptListModel::MessageTypeRole;
    if (field == QStringLiteral("position")) return PresetPromptListModel::PositionRole;
    if (field == QStringLiteral("isActive")) return PresetPromptListModel::IsActiveRole;
    return -1;
}

void CompletionPresetViewModel::setPromptField(const QString& promptId, const QString& field,
                                                const QVariant& value)
{
    if (promptId.isEmpty()) {
        return;
    }

    // 占位提示词（marker=true）的内容是运行时填的，界面上不能编辑。
    // 界面那边已经把输入框锁住了，这里再挡一层，防止别的入口把内容写进去。
    if (field == QStringLiteral("content")
        && m_presetPromptListModel->entryById(promptId).marker) {
        emit operationTip(QStringLiteral("系统占位提示词的内容不可编辑"));
        return;
    }

    const int role = roleForPromptField(field);
    if (role < 0 || !m_presetPromptListModel->updateField(promptId, role, value)) {
        return;
    }

    m_pendingPromptFields[promptId].insert(field, value);
    if (field == QStringLiteral("name") || field == QStringLiteral("content")) {
        m_promptSaveTimer->start();
    } else {
        flushPendingPromptEdits();
    }
}

void CompletionPresetViewModel::flushPendingPromptEdits()
{
    if (m_promptSaveTimer && m_promptSaveTimer->isActive()) {
        m_promptSaveTimer->stop();
    }
    if (m_pendingPromptFields.isEmpty()) {
        return;
    }

    const auto pending = m_pendingPromptFields;
    m_pendingPromptFields.clear();

    for (auto it = pending.constBegin(); it != pending.constEnd(); ++it) {
        const PromptEntity prompt = m_presetPromptListModel->entryById(it.key());
        if (prompt.id.isEmpty()) {
            continue;
        }
        if (!m_service->updatePrompt(prompt)) {
            refreshPromptList();
            return;
        }
    }
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
