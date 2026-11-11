#include "CharacterViewModel.h"
#include "CharacterService.h"
#include "CharacterDraft.h"
#include "CharacterItemViewModel.h"
#include "CharacterListModel.h"
#include "ChatService.h"
#include "WorldInfoService.h"
#include "Utils.h"

CharacterViewModel::CharacterViewModel(QObject* parent)
    : QObject(parent)
    , m_characterListModel(new CharacterListModel(this))
    , m_characterService(&CharacterService::instance())
    , m_chatService(&ChatService::instance())
    , m_worldInfoService(&WorldInfoService::instance())
    , m_characterDraft(new CharacterDraft(this))
{
    connect(m_characterDraft, &CharacterDraft::changed, this, [this]() {
        const bool dirty = m_characterDraft->toEntity() != m_characterBaseline;
        if (m_characterDirty != dirty) {
            m_characterDirty = dirty;
            emit characterDirtyChanged();
        }
        emit characterDraftChanged();
    });
    // 绑定的唯一来源是草稿：草稿里 worldBookIds 一变，两个只读属性就跟着失效。
    // 走这条连接而不是在各调用点手动发信号，是为了覆盖 "切角色 / 丢弃草稿 / 保存"
    // 这三条同样会改写草稿的路径。
    connect(m_characterDraft, &CharacterDraft::worldBookIdsChanged, this, [this]() {
        emit characterBookIdChanged();
        emit characterBookNameChanged();
    });
    // 文集被删除 / 改名后，绑定名必须立刻失效或更新。
    // 少了这条连接，QML 的 characterBookName 会一直用删除前求值出来的缓存值，
    // 表现为"世界书已经删了，角色详情里还显示着旧名字"（不会出现"已不存在"）。
    connect(m_worldInfoService, &WorldInfoService::bookListChanged,
            this, [this]() { emit characterBookNameChanged(); });
    connect(m_worldInfoService, &WorldInfoService::bookUpdated,
            this, [this](const QString&) { emit characterBookNameChanged(); });
    // 绑定业务层信号，自动同步视图状态
    connect(m_characterService, &CharacterService::characterListChanged,
            this, &CharacterViewModel::onCharacterListChanged);
    connect(m_characterService, &CharacterService::characterUpdated,
            this, &CharacterViewModel::onCharacterUpdated);
    connect(m_characterService, &CharacterService::operationError,
            this, &CharacterViewModel::operationError);
    connect(m_characterService, &CharacterService::importSuccess,
            this, &CharacterViewModel::importSuccess);
    connect(m_characterService, &CharacterService::exportSuccess,
            this, &CharacterViewModel::exportSuccess);

    // 初始化加载角色列表
    refreshList();
}

CharacterViewModel::~CharacterViewModel() = default;

int CharacterViewModel::currentIndex() const
{
    return m_currentIndex;
}

// ==================== 属性读取 ====================
CharacterListModel* CharacterViewModel::characterListModel() const
{
    return m_characterListModel;
}

CharacterEntity CharacterViewModel::currentCharacter() const
{
    return m_currentCharacter;
}

QString CharacterViewModel::currentAvatarUrl() const
{
    return Utils::avatarPathToUrl(m_currentCharacter.avatarPath);
}

CharacterDraft* CharacterViewModel::characterDraft() const
{
    return m_characterDraft;
}

bool CharacterViewModel::characterDirty() const
{
    return m_characterDirty;
}

QString CharacterViewModel::searchKeyword() const
{
    return m_searchKeyword;
}

void CharacterViewModel::setSearchKeyword(const QString& keyword)
{
    if (m_searchKeyword != keyword) {
        m_searchKeyword = keyword;
        emit searchKeywordChanged();
    }
}

bool CharacterViewModel::hasSelection() const
{
    return !m_currentCharacter.id.isEmpty();
}

// ==================== UI 可调用方法 ====================
void CharacterViewModel::refreshList()
{
    QList<CharacterEntity> entities = m_characterService->getAllCharacters();
    updateListModel(entities);
}

void CharacterViewModel::createNewCharacter()
{
    CharacterEntity newChar;
    newChar.name = "新角色";
    newChar.cardFormatVersion = 2;
    newChar.characterVersion = "1.0.0";

    const QString newId = m_characterService->createCharacter(newChar);
    if (newId.isEmpty()) {
        return;
    }

    // createCharacter 会同步发出 characterListChanged，列表已刷新；
    // 这里只需要按返回的真实 id 选中新角色，不能用未写库的 newChar。
    selectCharacter(newId);
}

void CharacterViewModel::updateAvatar(const QUrl& fileUrl)
{
    if (!hasSelection()) {
        emit operationError("请先选择角色");
        return;
    }

    const QString filePath = fileUrl.toLocalFile();
    if (filePath.isEmpty()) {
        emit operationError("头像路径无效");
        return;
    }

    if (!m_characterService->updateAvatar(m_currentCharacter.id, filePath))
        return;

    m_currentCharacter.avatarPath =
        m_characterService->getCharacterById(m_currentCharacter.id).avatarPath;
    m_characterDraft->setAvatarPath(m_currentCharacter.avatarPath);
    m_characterBaseline.avatarPath = m_currentCharacter.avatarPath;
    m_characterDirty = false;
    emit characterDraftChanged();
    emit characterDirtyChanged();
    refreshForm();
    emit operationTip("头像已更新");
}

void CharacterViewModel::deleteCurrentCharacter()
{
    if (!hasSelection()) return;
    m_characterService->deleteCharacter(m_currentCharacter.id);
    clearSelection();
}

void CharacterViewModel::duplicateCurrentCharacter()
{
    if (!hasSelection()) return;

    QString newId = m_characterService->duplicateCharacter(m_currentCharacter.id);
    if (!newId.isEmpty()) {
        selectCharacter(newId);
    }
}

void CharacterViewModel::importCharacter(const QUrl& fileUrl)
{
    const QString filePath = fileUrl.toLocalFile();
    const QString newId = m_characterService->importCharacterFromFile(filePath);

    // 导入后直接选中新角色，避免用户还要手动找。
    if (!newId.isEmpty()) {
        selectCharacter(newId);
    }
}

void CharacterViewModel::exportCurrentCharacter(const QUrl& fileUrl)
{
    QString filePath = fileUrl.toLocalFile();
    if (!hasSelection()) return;
    m_characterService->exportCharacterToFile(m_currentCharacter.id, filePath);
}

bool CharacterViewModel::requestSelectCharacter(const QString& charId)
{
    if (charId.isEmpty() || charId == m_currentCharacter.id) {
        return true;
    }

    if (characterDirty()) {
        m_pendingCharacterId = charId;
        emit unsavedChanges();
        return false;
    }

    selectCharacter(charId);
    return true;
}

void CharacterViewModel::confirmPendingCharacterSelection()
{
    if (m_pendingCharacterId.isEmpty()) {
        return;
    }

    const QString targetId = m_pendingCharacterId;
    m_pendingCharacterId.clear();
    discardCharacterDraft();
    selectCharacter(targetId);
}

void CharacterViewModel::cancelPendingCharacterSelection()
{
    m_pendingCharacterId.clear();
}

void CharacterViewModel::selectCharacter(const QString& charId)
{
    // 重复点击同一角色：不重复读库、不重复触发整份文本重排版
    // （与 CompletionPresetViewModel::selectPreset 的行为保持一致）
    if (m_currentCharacter.id == charId) return;

    CharacterEntity entity = m_characterService->getCharacterById(charId);
    if (entity.id.isEmpty()) return;

    m_currentCharacter = entity;
    const int row = m_characterListModel->rowById(charId);
    if (row >= 0 && m_currentIndex != row) {
        m_currentIndex = row;
        emit currentIndexChanged();
    }
    resetCharacterDraft(entity);
    refreshForm();
}

void CharacterViewModel::clearSelection()
{
    m_currentCharacter = CharacterEntity();
    if (m_currentIndex != -1) {
        m_currentIndex = -1;
        emit currentIndexChanged();
    }
    resetCharacterDraft(m_currentCharacter);
    refreshForm();
}

void CharacterViewModel::openSelectedCharChat()
{
    if (!hasSelection())
    {
        emit operationTip("请先选择角色");
        return;
    }
    QString charId = m_currentCharacter.id;
    if (charId.isEmpty())
    {
        emit operationError("角色ID无效");
        return;
    }


    // 调用ChatService业务方法
    QString sessionId = m_chatService->openCharacterSession(charId);
    if (sessionId.isEmpty())
    {
        emit operationError("打开角色会话失败");
        return;
    }
    // 打开成功，通知QML切换页面到聊天页
    emit navigateToChatPage();
}

// ==================== 槽函数 ====================
void CharacterViewModel::onCharacterListChanged()
{
    refreshList();
}

void CharacterViewModel::onCharacterUpdated(const QString& charId)
{
    const CharacterEntity entity = m_characterService->getCharacterById(charId);
    if (entity.id.isEmpty()) {
        return;
    }

    m_characterListModel->updateItem(entity);

    if (m_currentCharacter.id == charId) {
        m_currentCharacter = entity;
        if (!characterDirty()) {
            resetCharacterDraft(entity);
        }
        refreshForm();
    }
}

void CharacterViewModel::onOperationError(const QString& errorMsg)
{
    emit operationError(errorMsg);
}

void CharacterViewModel::updateListModel(const QList<CharacterEntity>& entities)
{
    QList<CharacterItemViewModel*> items;
    items.reserve(entities.size());

    for (const CharacterEntity& entity : entities) {
        items.append(createItemFromEntity(entity));
    }

    m_characterListModel->loadList(items);
}

CharacterItemViewModel* CharacterViewModel::createItemFromEntity(const CharacterEntity& entity)
{
    auto* item = new CharacterItemViewModel();
    item->updateFromEntity(entity);
    return item;
}

void CharacterViewModel::refreshForm()
{
    emit currentCharacterChanged();
    emit characterBookIdChanged();
    emit characterBookNameChanged();
}

void CharacterViewModel::resetCharacterDraft(const CharacterEntity& entity)
{
    m_characterBaseline = entity;
    m_characterDraft->setEntity(entity);
    m_characterDirty = false;
    emit characterDraftChanged();
    emit characterDirtyChanged();
}

CharacterEntity CharacterViewModel::draftToEntity() const
{
    return m_characterDraft->toEntity();
}

void CharacterViewModel::setCharacterDraftField(const QString& field, const QVariant& value)
{
    static const QStringList editableFields = {
        QStringLiteral("name"),
        QStringLiteral("description"),
        QStringLiteral("firstMessage"),
        QStringLiteral("alternateGreetings"),
        QStringLiteral("creatorNotes"),
        QStringLiteral("creator"),
        QStringLiteral("personality"),
        QStringLiteral("scenario"),
        QStringLiteral("exampleDialogue"),
        QStringLiteral("characterVersion"),
        QStringLiteral("tags"),
        QStringLiteral("systemPrompt"),
        QStringLiteral("postHistoryInstructions")
    };

    if (!editableFields.contains(field)) {
        return;
    }

    m_characterDraft->setField(field, value);
}

bool CharacterViewModel::saveCharacterDraft()
{
    if (!hasSelection()) {
        return false;
    }

    const CharacterEntity entity = draftToEntity();
    if (!m_characterService->updateCharacter(entity)) {
        return false;
    }

    m_currentCharacter = entity;
    resetCharacterDraft(entity);
    emit operationTip(QStringLiteral("角色已保存"));
    return true;
}

void CharacterViewModel::discardCharacterDraft()
{
    resetCharacterDraft(m_currentCharacter);
}


// ==================== 世界书绑定（单本语义）====================

QString CharacterViewModel::characterBookId() const
{
    const QStringList ids = m_characterDraft ? m_characterDraft->worldBookIds() : QStringList();
    return ids.isEmpty() ? QString() : ids.first();
}

QString CharacterViewModel::getCharacterBookName() const
{
    const QString bookId = characterBookId();
    if (bookId.isEmpty()) {
        return {};
    }

    // 文集被删掉后这里返回空串：草稿里还留着旧 id，界面据此显示"已不存在"
    return m_worldInfoService->getBookById(bookId).name;
}

void CharacterViewModel::refreshCharacterBookBinding()
{
    emit characterBookIdChanged();
    emit characterBookNameChanged();
}

void CharacterViewModel::bindCharacterBook(const QString& bookId)
{
    if (!hasSelection() || !m_characterDraft) {
        emit operationError("请先选择角色");
        return;
    }
    if (bookId.isEmpty()) {
        unbindCharacterBook();
        return;
    }

    // 单本语义：整个绑定替换为这一本，绝不 append。
    // 追加会留下 "界面显示一本、列表里却有两本" 的脏状态。
    m_characterDraft->setWorldBookIds(QStringList{bookId});
}

void CharacterViewModel::unbindCharacterBook()
{
    if (!hasSelection() || !m_characterDraft) {
        emit operationError("请先选择角色");
        return;
    }
    if (m_characterDraft->worldBookIds().isEmpty()) {
        return;
    }

    // 清空整个列表，而不是只摘掉 first()，否则 "无绑定" 与列表内容会不一致
    m_characterDraft->setWorldBookIds(QStringList());
}

