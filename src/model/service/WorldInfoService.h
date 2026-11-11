#ifndef WORLDINFOSERVICE_H
#define WORLDINFOSERVICE_H

#include <QObject>
#include <QList>
#include "WorldInfoBookDao.h"
#include "WorldInfoBookEntity.h"
#include "WorldInfoEntryEntity.h"
#include "WorldInfoEntryDao.h"

class WorldInfoService : public QObject
{
    Q_OBJECT
public:
    static WorldInfoService& instance();
    ~WorldInfoService() override;

    // 测试/组合根可注入 DAO；生产仍使用 instance()。
    explicit WorldInfoService(WorldInfoBookDao* bookDao,
                              WorldInfoEntryDao* entryDao,
                              QObject* parent = nullptr);

    // ========== 外部接口 ==========
    QList<WorldInfoEntryEntity> getEntryForEnabledBooks();
    // ========== 文集业务 ==========
    QList<WorldInfoBookEntity> getAllBooks();
    WorldInfoBookEntity getBookById(const QString& bookId);
    QList<WorldInfoBookEntity> getEffectiveBooksForCharacter(const QString& charId);


    QString createBook(const QString& name, const QString& description = {});
    bool updateBook(const WorldInfoBookEntity& book);
    bool deleteBook(const QString& bookId);
    QString duplicateBook(const QString& sourceId);
    bool toggleBookEnabled(const QString& bookId);

    // ========== 条目业务 ==========
    QList<WorldInfoEntryEntity> getEntriesByBook(const QString& bookId);

    QString createEntry(const QString& bookId, const QString& name = "新条目");
    bool updateEntry(const WorldInfoEntryEntity& entry);
    bool deleteEntry(const QString& entryId);
    bool toggleEntryEnabled(const QString& entryId);
    bool moveEntryToBook(const QString& entryId, const QString& targetBookId);
    bool batchDeleteEntries(const QStringList& entryIds);
    bool batchToggleEntries(const QStringList& entryIds, bool enabled);

    // ========== 导入导出 ==========
    int importBookFromSillyTavern(const QString& filePath);//导入
    bool exportBookToSillyTavern(const QString& bookId, const QString& savePath);
    bool exportAllBooksToSillyTavern(const QString& savePath);

    //写入数据库
    QString importWorldBook(QJsonObject & bookData);


signals:
    void bookListChanged();
    void bookUpdated(const QString& bookId);
    void entryListChanged(const QString& bookId);
    void entryUpdated(const QString& entryId);
    void entryDeleted(const QString& entryId);
    void importSuccess(int count);
    void exportSuccess();
    void operationError(const QString& errorMsg);

private:
    explicit WorldInfoService(QObject* parent = nullptr);
    QJsonObject bookToStJson(const WorldInfoBookEntity& book, const QList<WorldInfoEntryEntity>& entries);
    WorldInfoBookEntity parseJsonToBook(const QJsonObject& json);
    WorldInfoEntryEntity parseJsonToEntry(const QJsonObject& json, const QString& bookId);

    WorldInfoBookDao* m_BookDao = nullptr;
    WorldInfoEntryDao* m_EntryDao = nullptr;
};

#endif // WORLDINFOSERVICE_H