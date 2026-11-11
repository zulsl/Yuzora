#include "CharacterService.h"
#include "CharacterDao.h"
#include "Utils.h"
#include "AppException.h"
#include "DatabaseManager.h"
#include "WorldInfoService.h"
#include <QDir>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QFile>

CharacterService& CharacterService::instance()
{
    static CharacterService instance;
    return instance;
}

CharacterService::CharacterService(QObject* parent)
    : QObject(parent)
    , m_dao(new CharacterDao(this))
    , m_worldInfoService(&WorldInfoService::instance())
{
}

CharacterService::CharacterService(CharacterDao* dao, QObject* parent)
    : CharacterService(dao, &WorldInfoService::instance(), parent)
{
}

CharacterService::CharacterService(CharacterDao* dao,
                                   WorldInfoService* worldInfoService,
                                   QObject* parent)
    : QObject(parent)
    , m_dao(dao ? dao : new CharacterDao(this))
    , m_worldInfoService(worldInfoService)
{
}

CharacterService::~CharacterService() = default;

void CharacterService::ensureSeedData()
{
    initBuiltinCharacters();
}

void CharacterService::initBuiltinCharacters()
{
    // 已有配置则跳过初始化，避免重复插入
    if (!m_dao->selectAll().isEmpty()) return;

    CharacterEntity entity;
    entity.name="ai助手";
    entity.description="You are a helpful assistant.";
    entity.createdAt=QDateTime::currentDateTime();
    entity.updatedAt=QDateTime::currentDateTime();
    createCharacter(entity);
}

// ==================== 查询类接口 ====================
QList<CharacterEntity> CharacterService::getAllCharacters()
{
    return m_dao->selectAll();
}

QString CharacterService::getDefaultCharacterId()
{
    const QList<CharacterEntity> list = m_dao->selectAll();
    if (list.isEmpty()) {
        return {};
    }
    return list.first().id;
}

CharacterEntity CharacterService::getCharacterById(const QString& id)
{
    return m_dao->selectById(id);
}

QList<CharacterEntity> CharacterService::searchByName(const QString& keyword)
{
    if (keyword.trimmed().isEmpty()) {
        return getAllCharacters();
    }
    return m_dao->searchByName(keyword);
}

// ==================== 基础增删改 ====================
QString CharacterService::createCharacter(const CharacterEntity& character)
{
    QString errorMsg;
    if (!validateCharacterData(character, errorMsg)) {
        emit operationError(errorMsg);
        return {};
    }

    CharacterEntity newChar = character;
    if (newChar.id.isEmpty()) newChar.id = Utils::generateUuid();
    // if (!newChar.avatarPath.isEmpty()) newChar.avatarPath = newChar.id+newChar.avatarPath;
    newChar.createdAt = QDateTime::currentDateTime();
    newChar.updatedAt = QDateTime::currentDateTime();

    bool success = m_dao->insert(newChar);
    if (!success) {
        emit operationError("创建角色失败：数据库写入错误");
        return QString{};
    }

    emit characterListChanged();
    return newChar.id;
}

bool CharacterService::updateCharacter(const CharacterEntity& character)
{
    QString errorMsg;
    if (!validateCharacterData(character, errorMsg)) {
        emit operationError(errorMsg);
        return false;
    }

    if (character.id.isEmpty()) {
        emit operationError("更新失败：角色ID不能为空");
        return false;
    }

    CharacterEntity updated = character;
    updated.updatedAt = QDateTime::currentDateTime();

    bool success = m_dao->update(updated);
    if (!success) {
        emit operationError("更新角色失败：数据库写入错误");
        return false;
    }

    emit characterUpdated(character.id);
    return true;
}

bool CharacterService::updateAvatar(const QString& charId, const QString& sourceFilePath)
{
    if (charId.isEmpty()) {
        emit operationError("更新头像失败：角色ID不能为空");
        return false;
    }
    if (sourceFilePath.isEmpty()) {
        emit operationError("更新头像失败：文件路径为空");
        return false;
    }

    CharacterEntity existing = m_dao->selectById(charId);
    if (existing.id.isEmpty()) {
        emit operationError("更新头像失败：角色不存在");
        return false;
    }

    QImage image(sourceFilePath);
    if (image.isNull()) {
        emit operationError("更新头像失败：无法读取图片");
        return false;
    }

    // 头像文件名带唯一后缀，保证 QML Image 的 source 变化后能重新加载，
    // 而不是继续命中旧文件缓存。
    const QString newAvatarPath = charId + "_" + Utils::generateUuid() + ".png";
    const QString avatarFullPath = QDir(Utils::getAvatarDirPath()).filePath(newAvatarPath);
    if (!image.save(avatarFullPath, "PNG")) {
        emit operationError("更新头像失败：无法写入头像文件");
        return false;
    }

    if (!m_dao->updateAvatar(charId, newAvatarPath)) {
        cleanupAvatarFile(newAvatarPath);
        emit operationError("更新头像失败：数据库写入错误");
        return false;
    }

    if (!existing.avatarPath.isEmpty() && existing.avatarPath != newAvatarPath)
        cleanupAvatarFile(existing.avatarPath);

    emit characterUpdated(charId);
    emit characterListChanged();
    return true;
}

bool CharacterService::deleteCharacter(const QString& id)
{
    if (id.isEmpty()) {
        emit operationError("删除失败：角色ID不能为空");
        return false;
    }

    CharacterEntity character = m_dao->selectById(id);
    if (character.id.isEmpty()) {
        emit operationError("删除失败：角色不存在");
        return false;
    }

    // 同步清理本地头像文件
    if (!character.avatarPath.isEmpty()) {
        cleanupAvatarFile(character.avatarPath);
    }

    bool success = m_dao->remove(id);
    if (!success) {
        emit operationError("删除角色失败：数据库操作错误");
        return false;
    }

    emit characterListChanged();
    emit characterDeleted(id);
    return true;
}

QString CharacterService::duplicateCharacter(const QString& sourceId)
{
    CharacterEntity source = m_dao->selectById(sourceId);
    if (source.id.isEmpty()) {
        emit operationError("复制失败：源角色不存在");
        return {};
    }

    CharacterEntity copy = source;
    copy.id = Utils::generateUuid();
    copy.name = source.name + " (副本)";
    QDateTime now = QDateTime::currentDateTime();
    copy.createdAt = now;
    copy.updatedAt = now;

    // 复制头像文件
    if (!source.avatarPath.isEmpty()) {
        QString srcPath = QDir(Utils::getAvatarDirPath()).filePath(source.avatarPath);
        QString newFileName = copy.id + ".png";
        QString destPath = QDir(Utils::getAvatarDirPath()).filePath(newFileName);
        if (QFile::exists(srcPath)) {
            QFile::copy(srcPath, destPath);
            copy.avatarPath = newFileName;
        } else {
            copy.avatarPath.clear();
        }
    }

    bool success = m_dao->insert(copy);
    if (!success) {
        emit operationError("复制角色失败：数据库写入错误");
        return {};
    }

    emit characterListChanged();
    return copy.id;
}

// ==================== 导入导出 ====================
QString CharacterService::importCharacterFromFile(const QString& filePath)
{
    if (!QFile::exists(filePath)) {
        qDebug()<<"导入失败："<<filePath<<"文件不存在";
        emit operationError("导入失败：文件不存在");
        return {};
    }

    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    QJsonObject cardData;

    try {
        if (suffix == "png") {
            cardData = Utils::extractPngMetadata(filePath);
            if (cardData.isEmpty()) {
                emit operationError("导入失败：PNG文件不含角色卡元数据");
                return {};
            }
        } else if (suffix == "json") {
            QJsonDocument doc = Utils::readJsonFile(filePath);
            if (doc.isNull() || !doc.isObject()) {
                emit operationError("导入失败：JSON文件格式错误");
                return {};
            }
            cardData = doc.object();
        } else {
            emit operationError("导入失败：不支持的文件格式");
            return {};
        }

        // 版本兼容转换：V1 自动转 V2(未完成)
        // int version = cardData.value("spec").toString("v2").remove("v").toInt();
        // if (version == 1) {
        //     cardData = Utils::convertCharacterV1ToV2(cardData).object();
        // }

        CharacterEntity entity = parseCardDataToEntity(cardData);

        if (suffix == "png") {
            //头像路径
            entity.avatarPath = entity.id+".png";
            // PNG 角色卡直接用原图作为头像
            QImage image(filePath);
            if (!image.isNull()) {
                saveAvatarFromImage(image,entity.id);
            }
        }

        const QString newId = createCharacter(entity);
        if (newId.isEmpty()) {
            return {};
        }
        return newId;

    } catch (const AppException& e) {
        emit operationError(e.getMessage());
        return {};
    }
}

bool CharacterService::exportCharacterToFile(const QString& charId, const QString& savePath)
{
    CharacterEntity entity = getCharacterById(charId);
    if (entity.id.isEmpty()) {
        emit operationError("导出失败：角色不存在");
        return false;
    }

    QFileInfo fileInfo(savePath);
    QString suffix = fileInfo.suffix().toLower();

    try {
        QJsonObject cardData = buildCardDataFromEntity(entity);

        if (suffix == "json") {
            QJsonDocument doc(cardData);
            if (!Utils::writeJsonFile(savePath, doc)) {
                emit operationError("导出失败：文件写入错误");
                return false;
            }
        } else if (suffix == "png") {
            QString avatarFullPath = QDir(Utils::getAvatarDirPath()).filePath(entity.avatarPath);
            if (!QFile::exists(avatarFullPath)) {
                emit operationError("导出失败：角色头像文件不存在");
                return false;
            }
            // 先复制到目标路径，再注入元数据
            QFile::copy(avatarFullPath, savePath);
            if (!Utils::injectPngMetadata(savePath, cardData)) {
                QFile::remove(savePath);
                emit operationError("导出失败：元数据注入失败");
                return false;
            }
        } else {
            emit operationError("导出失败：不支持的文件格式");
            return false;
        }

        emit exportSuccess();
        return true;

    } catch (const AppException& e) {
        emit operationError(e.getMessage());
        return false;
    }
}

int CharacterService::importBatchFromDirectory(const QString& dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        emit operationError("批量导入失败：目录不存在");
        return 0;
    }

    QStringList filters = {"*.png", "*.json"};
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

    int successCount = 0;
    DatabaseManager::instance().beginTransaction();

    for (const QFileInfo& file : files) {
        QString id = importCharacterFromFile(file.absoluteFilePath());
        if (!id.isEmpty()) {
            successCount++;
        }
    }

    if (successCount > 0) {
        DatabaseManager::instance().commitTransaction();
        emit characterListChanged();
        emit importSuccess(successCount);
    } else {
        DatabaseManager::instance().rollbackTransaction();
    }

    return successCount;
}

// ==================== 业务状态操作 ====================
bool CharacterService::toggleFavorite(const QString& charId)
{
    CharacterEntity entity = m_dao->selectById(charId);
    if (entity.id.isEmpty()) {
        emit operationError("操作失败：角色不存在");
        return false;
    }

    entity.isFavorite = !entity.isFavorite;
    entity.updatedAt = QDateTime::currentDateTime();

    bool success = m_dao->update(entity);
    if (success) {
        emit characterListChanged();
        emit characterUpdated(charId);
    } else {
        emit operationError("收藏状态更新失败");
    }
    return success;
}

bool CharacterService::incrementUseCount(const QString& charId)
{
    CharacterEntity entity = m_dao->selectById(charId);
    if (entity.id.isEmpty()) {
        return false;
    }

    // entity.useCount += 1;
    // entity.lastUsedAt = QDateTime::currentDateTime();
    return m_dao->update(entity);
}

bool CharacterService::bindDefaultPreset(const QString& charId, const QString& presetId)
{
    CharacterEntity entity = m_dao->selectById(charId);
    if (entity.id.isEmpty()) {
        emit operationError("绑定失败：角色不存在");
        return false;
    }

    // entity.defaultPresetId = presetId;
    entity.updatedAt = QDateTime::currentDateTime();

    bool success = m_dao->update(entity);
    if (success) {
        emit characterUpdated(charId);
    }
    return success;
}

bool CharacterService::bindWorldBooks(const QString& charId, const QStringList& bookIds)
{
    CharacterEntity entity = m_dao->selectById(charId);
    if (entity.id.isEmpty()) {
        emit operationError("绑定失败：角色不存在");
        return false;
    }

    entity.worldBookIds = bookIds;
    entity.updatedAt = QDateTime::currentDateTime();

    bool success = m_dao->update(entity);
    if (success) {
        emit characterUpdated(charId);
    }
    return success;
}

// ==================== 私有辅助方法 ====================
bool CharacterService::validateCharacterData(const CharacterEntity& character, QString& outError)
{
    if (character.name.trimmed().isEmpty()) {
        outError = "角色名称不能为空";
        return false;
    }
    if (character.name.length() > 100) {
        outError = "角色名称不能超过100个字符";
        return false;
    }
    return true;
}

QString CharacterService::saveAvatarFromImage(const QImage& image, const QString& charId)
{
    QString avatarDir = Utils::getAvatarDirPath();
    QString fileName = charId + ".png";
    QString fullPath = QDir(avatarDir).filePath(fileName);

    if (!image.save(fullPath, "PNG")) {
        THROW(FileIOError, "头像保存失败：无法写入文件");
    }

    qDebug()<<"头像相对路径为"<<fileName;

    return fileName; // 返回相对路径
}

void CharacterService::cleanupAvatarFile(const QString& relativePath)
{
    if (relativePath.isEmpty()) return;

    QString fullPath = QDir(Utils::getAvatarDirPath()).filePath(relativePath);
    if (QFile::exists(fullPath)) {
        QFile::remove(fullPath);
    }
}

CharacterEntity CharacterService::parseCardDataToEntity(const QJsonObject& cardData)
{
    CharacterEntity entity;

    QJsonObject data = cardData.value("data").toObject();
    entity.id = Utils::generateUuid();
    entity.name = data.value("name").toString();
    entity.description = data.value("description").toString();
    entity.firstMessage = data.value("first_mes").toString();

    // 备选开场白
    QJsonArray altGreetings = data.value("alternate_greetings").toArray();
    for (const auto& val : altGreetings) {
        entity.alternateGreetings.append(val.toString());
    }

    entity.creatorNotes = data.value("creator_notes").toString();
    entity.creator = data.value("creator").toString();
    entity.personality = data.value("personality").toString();
    entity.scenario = data.value("scenario").toString();
    entity.exampleDialogue = data.value("mes_example").toString();
    entity.systemPrompt = data.value("system_prompt").toString();
    entity.characterVersion = data.value("character_version").toString("1.0.0");

    // 标签
    QJsonArray tags = data.value("tags").toArray();
    for (const auto& val : tags) {
        entity.tags.append(val.toString());
    }

    entity.cardFormatVersion = cardData.value("spec").toString("v2").remove("v").toInt();

    //是否收藏
    entity.isFavorite = false;

    //导入并绑定世界书id
    QJsonObject characterBook = data.value("character_book").toObject();
    if (m_worldInfoService && !characterBook.isEmpty()) {
        const QString bookId = m_worldInfoService->importWorldBook(characterBook);
        if (!bookId.isEmpty()) {
            entity.worldBookIds.append(bookId);
        }
    }


    // 扩展字段
    if (data.contains("extensions")) {
        entity.extensions = data.value("extensions").toObject().toVariantMap();
    }

    entity.createdAt=QDateTime::fromString(cardData.value("create_date").toString(),Qt::ISODate);

    return entity;
}

QJsonObject CharacterService::buildCardDataFromEntity(const CharacterEntity& entity)
{
    QJsonObject data;
    data["name"] = entity.name;
    data["description"] = entity.description;
    data["personality"] = entity.personality;
    data["scenario"] = entity.scenario;
    data["first_mes"] = entity.firstMessage;
    data["mes_example"] = entity.exampleDialogue;
    data["system_prompt"] = entity.systemPrompt;
    data["creator_notes"] = entity.creatorNotes;
    data["creator"] = entity.creator;
    data["character_version"] = entity.characterVersion;

    // 备选开场白
    QJsonArray altGreetings;
    for (const QString& s : entity.alternateGreetings) {
        altGreetings.append(s);
    }
    data["alternate_greetings"] = altGreetings;

    // 标签
    QJsonArray tags;
    for (const QString& s : entity.tags) {
        tags.append(s);
    }
    data["tags"] = tags;

    // 扩展字段
    if (!entity.extensions.isEmpty()) {
        data["extensions"] = QJsonObject::fromVariantMap(entity.extensions);
    }

    QJsonObject card;
    card["spec"] = QString("v%1").arg(entity.cardFormatVersion);
    card["data"] = data;
    return card;
}