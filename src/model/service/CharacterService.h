#ifndef CHARACTERSERVICE_H
#define CHARACTERSERVICE_H

#include <QObject>
#include <QList>
#include <QString>
#include "dao/CharacterDao.h"
#include "entity/CharacterEntity.h"

class WorldInfoService;

/**
 * @brief 角色管理业务服务层
 * 职责：业务逻辑封装、数据校验、导入导出流程、事务控制、关联资源管理
 */
class CharacterService : public QObject
{
    Q_OBJECT
public:
    static CharacterService& instance();
    ~CharacterService() override;

    // 测试/组合根可注入 DAO；生产仍使用 instance()。
    explicit CharacterService(CharacterDao* dao, QObject* parent = nullptr);
    // 测试可把世界书导入依赖设为 nullptr，角色卡导入时跳过世界书。
    explicit CharacterService(CharacterDao* dao, WorldInfoService* worldInfoService,
                              QObject* parent = nullptr);

    CharacterService(const CharacterService&) = delete;
    CharacterService& operator=(const CharacterService&) = delete;
    CharacterService(CharacterService&&) = delete;
    CharacterService& operator=(CharacterService&&) = delete;

    // 由 ApplicationBootstrap 在数据库初始化后调用；构造函数不再写库。
    void ensureSeedData();

    // ========== 查询类接口 ==========
    QList<CharacterEntity> getAllCharacters();
    QString getDefaultCharacterId();
    CharacterEntity getCharacterById(const QString& id);
    QList<CharacterEntity> searchByName(const QString& keyword);

    // ========== 角色基础增删改 ==========
    QString createCharacter(const CharacterEntity& character);//创建角色
    bool updateCharacter(const CharacterEntity& character);
    bool updateAvatar(const QString& charId, const QString& sourceFilePath);//更新角色头像
    bool deleteCharacter(const QString& id);
    QString duplicateCharacter(const QString& sourceId);

    // ========== 从文件导入导出角色卡 ==========
    QString importCharacterFromFile(const QString& filePath);//导入
    bool exportCharacterToFile(const QString& charId, const QString& savePath);
    int importBatchFromDirectory(const QString& dirPath);//批量导入

    // ========== 业务状态操作 ==========
    bool toggleFavorite(const QString& charId);
    bool incrementUseCount(const QString& charId);
    bool bindDefaultPreset(const QString& charId, const QString& presetId);
    bool bindWorldBooks(const QString& charId, const QStringList& bookIds);

signals:
    void characterListChanged();//列表变更
    void characterUpdated(const QString& charId);//角色变更
    void characterDeleted(const QString& charId);
    void importSuccess(int importCount);
    void exportSuccess();
    void operationError(const QString& errorMsg);

private:
    explicit CharacterService(QObject* parent = nullptr);

    void initBuiltinCharacters();

    // 内部业务辅助方法
    bool validateCharacterData(const CharacterEntity& character, QString& outError);//验证角色()
    QString saveAvatarFromImage(const QImage& image, const QString& charId);//保存头像
    void cleanupAvatarFile(const QString& relativePath);//删除本地头像
    CharacterEntity parseCardDataToEntity(const QJsonObject& cardData);//解析角色卡数据
    QJsonObject buildCardDataFromEntity(const CharacterEntity& entity);

    CharacterDao* m_dao = nullptr;
    WorldInfoService* m_worldInfoService = nullptr;

};

#endif // CHARACTERSERVICE_H