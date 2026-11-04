// #ifndef CHARACTERMANAGER_H
// #define CHARACTERMANAGER_H

// #include <QObject>
// #include <QList>
// #include <QMap>
// #include <QJsonObject>
// #include <QImage>

// //角色管理类

// //角色卡信息
// struct CharacterInfo {
//     QString id;//生成的id
//     QString name;
//     QString description;//描述
//     QString personality;//个性
//     QString firstMessage;//第一条消息
//     QString exampleDialogue;//对话示例
//     QString systemPrompt;//系统提示词
//     QString creatorNotes;//作者注释
//     QVariantMap extensions;//扩展
//     QString avatarPath;//头像路径
//     QImage avatarCache;//头像缓存
//     QDateTime createdAt;//创建时间
//     QDateTime updatedAt;//更新时间
//     int version;//版本
// };

// class CharacterManager : public QObject {
//     Q_OBJECT
// public:
//     static CharacterManager& instance();
//     ~CharacterManager() override = default;

// // ======================================
// // 角色基础CRUD
// // ======================================
//     //获取角色列表
//     Q_INVOKABLE QList<CharacterInfo> getAllCharacters();
//     //按ID获取角色
//     Q_INVOKABLE CharacterInfo getCharacterById(const QString& id);
//     //创建角色
//     Q_INVOKABLE QString createCharacter(const CharacterInfo& character);
//     //更新角色
//     Q_INVOKABLE bool updateCharacter(const CharacterInfo& character);
//     //删除角色
//     Q_INVOKABLE bool deleteCharacter(const QString& id);
//     //复制角色
//     Q_INVOKABLE bool duplicateCharacter(const QString& sourceId, QString& newId);

// // ======================================
// // 角色卡导入导出
// // ======================================
//     //从png导入角色
//     CharacterInfo importCharacterFromPng(const QString& filePath);
//     //从Json导入角色
//     CharacterInfo importCharacterFromJson(const QString& filePath);
//     //导出角色到Png
//     Q_INVOKABLE bool exportCharacterToPng(const QString& charId, const QString& savePath);
//     //将角色导出到Json
//     Q_INVOKABLE QJsonObject exportCharacterToJson(const QString& charId);

// // ======================================
// // 头像缓存优化
// // ======================================
//     //预加载角色头像
//     Q_INVOKABLE void preloadCharacterAvatar(const QString& charId);
//     //清除头像缓存
//     Q_INVOKABLE void clearAvatarCache();

// signals:
//     //角色列表已更改
//     void characterListChanged();
//     //角色已更新
//     void characterUpdated(const QString& charId);
//     //角色已删除
//     void characterDeleted(const QString& charId);

// private:
//     CharacterManager(QObject* parent = nullptr);

//     //加载所有角色
//     void loadAllCharacters();
//     //将角色保存到文件
//     void saveCharacterToFile(const CharacterInfo& character);
//     //解析角色卡json数据
//     CharacterInfo parseCharacterData(const QJsonObject& data);
//     //角色转Json
//     QJsonObject characterToJson(const CharacterInfo& character);

//     QMap<QString, CharacterInfo> m_characters;//角色列表
//     QMap<QString, QImage> m_avatarCache;//头像缓存
//     QString m_characterDir;//角色目录
// };

// #endif // CHARACTERMANAGER_H
