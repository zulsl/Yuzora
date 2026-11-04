// #ifndef WORLDINFOMANAGER_H
// #define WORLDINFOMANAGER_H

// #include <QObject>
// #include <QList>
// #include <QMap>
// #include <QDateTime>
// #include "GlobalConstants.h"

// //世界书管理

// //世界书条目
// struct WorldInfoEntry {
//     QString id;
//     QString name;
//     QList<QString> keywords;//关键词
//     QString content;//内容
//     GlobalConstants::WorldInfoTriggerType triggerType =
//         GlobalConstants::WorldInfoTriggerType::ExactMatch;//触发类型
//     int priority = 100;//优先级
//     int depth = 4;//深度
//     bool isRecursive = false;//是否递归
//     bool isCaseSensitive = false;//是否区分大小写
//     bool isEnabled = true;//是否启用
//     QVariantMap extensions;//扩展
// };

// //世界书
// struct WorldInfoBook {
//     QString id;
//     QString name;
//     QString description;//描述
//     QList<WorldInfoEntry> entries;//条目
//     QDateTime createdAt;//创建时间
//     QDateTime updatedAt;//更新时间
// };


// class WorldInfoManager : public QObject {
//     Q_OBJECT
// public:
//     static WorldInfoManager& instance();
//     ~WorldInfoManager() override = default;

// // ======================================
// // 世界书文集管理
// // ======================================
//     //获取全部书
//     QList<WorldInfoBook> getAllBooks();
//     //获取书
//     WorldInfoBook getBook(const QString& bookId);
//     //创建书
//     QString createBook(const QString& name);
//     //删除书
//     bool deleteBook(const QString& bookId);
//     //更新书
//     bool updateBook(const WorldInfoBook& book);

// // ======================================
// // 世界书条目管理
// // ======================================
//     //添加条目
//     QString addEntry(const QString& bookId, const WorldInfoEntry& entry);
//     //更新条目
//     bool updateEntry(const QString& bookId, const WorldInfoEntry& entry);
//     //删除条目
//     bool deleteEntry(const QString& bookId, const QString& entryId);

// // ======================================
// // 核心匹配与Prompt生成
// // ======================================
//     //匹配世界书
//     QList<QString> matchWorldInfo(const QString& bookId,
//                                   const QList<QString>& context, int maxDepth = 4);
//     //构建世界书提示词
//     QString buildWorldInfoPrompt(const QString& bookId, const QList<QString>& context);

// signals:
//     //书列表已更改
//     void bookListChanged();
//     //书已更新
//     void bookUpdated(const QString& bookId);
//     //条目列表已更改
//     void entryListChanged(const QString& bookId);

// private:
//     WorldInfoManager(QObject* parent = nullptr);

//     //加载全部书
//     void loadAllBooks();
//     //保存书到文件
//     void saveBookToFile(const WorldInfoBook& book);
//     //解析书数据
//     WorldInfoBook parseBookData(const QJsonObject& data);
//     //书转json
//     QJsonObject bookToJson(const WorldInfoBook& book);
//     //解析条目数据
//     WorldInfoEntry parseEntryData(const QJsonObject& data);
//     //条目转json
//     QJsonObject entryToJson(const WorldInfoEntry& entry);

//     QMap<QString, WorldInfoBook> m_books;//书列表
//     QString m_worldInfoDir;//世界书目录
// };

// #endif // WORLDINFOMANAGER_H
