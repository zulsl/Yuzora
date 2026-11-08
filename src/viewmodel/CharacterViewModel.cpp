#include "CharacterViewModel.h"
#include "CharacterService.h"
#include "CharacterItemViewModel.h"
#include "CharacterListModel.h"
#include "ChatService.h"
#include "WorldInfoService.h"

CharacterViewModel::CharacterViewModel(QObject* parent)
    : QObject(parent)
    , m_characterListModel(new CharacterListModel(this))
    , m_characterService(&CharacterService::instance())
    , m_chatService(&ChatService::instance())
    , m_worldInfoService(&WorldInfoService::instance())
{
    // 绑定业务层信号，自动同步视图状态
    connect(m_characterService, &CharacterService::characterListChanged,
            this, &CharacterViewModel::onCharacterListChanged);
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

void CharacterViewModel::setCurrentIndex(int index)
{
    if (m_currentIndex == index) return;
    if (index < 0 || index >= m_characterListModel->rowCount()) return;

    m_currentIndex = index;
    emit currentIndexChanged();
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

void CharacterViewModel::searchCharacters()
{
    QList<CharacterEntity> entities = m_characterService->searchByName(m_searchKeyword);
    updateListModel(entities);
}

void CharacterViewModel::createNewCharacter()
{
    CharacterEntity newChar;
    newChar.name = "新角色";
    newChar.cardFormatVersion = 2;
    newChar.characterVersion = "1.0.0";

    m_characterService->createCharacter(newChar);

    refreshList();

    m_currentCharacter = newChar;
    refreshForm();
}

void CharacterViewModel::saveCurrentCharacter(const CharacterEntity& entity)
{
    CharacterEntity updated = entity;
    m_characterService->updateCharacter(updated);

    m_currentCharacter = updated;
    refreshForm();
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
    qDebug()<<"文件url:"<<fileUrl;
    QString filePath = fileUrl.toLocalFile();
    QString newId = m_characterService->importCharacterFromFile(filePath);

    //切换表单
    // if (!newId.isEmpty()) {
    //     selectCharacter(newId);
    // }
}

void CharacterViewModel::exportCurrentCharacter(const QString& savePath)
{
    if (!hasSelection()) return;
    m_characterService->exportCharacterToFile(m_currentCharacter.id, savePath);
}

void CharacterViewModel::toggleFavorite(const QString& charId)
{
    m_characterService->toggleFavorite(charId);
}

void CharacterViewModel::selectCharacter(const QString& charId)
{
    // 重复点击同一角色：不重复读库、不重复触发整份文本重排版
    // （与 CompletionPresetViewModel::selectPreset 的行为保持一致）
    if (m_currentCharacter.id == charId) return;

    CharacterEntity entity = m_characterService->getCharacterById(charId);
    if (entity.id.isEmpty()) return;

    m_currentCharacter = entity;
    refreshForm();
}

void CharacterViewModel::clearSelection()
{
    m_currentCharacter = CharacterEntity();
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
    emit characterBookNameChanged();
}


QString CharacterViewModel::getCharacterBookName() const
{

    if (m_currentCharacter.worldBookIds.isEmpty()) return "无";

    QString bookId=m_currentCharacter.worldBookIds.first();
    WorldInfoBookEntity entity = m_worldInfoService->getBookById(bookId);

    return entity.name.isEmpty() ? "无" : entity.name;
}

