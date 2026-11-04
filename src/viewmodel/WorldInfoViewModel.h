#ifndef WORLDINFOVIEWMODEL_H
#define WORLDINFOVIEWMODEL_H

#include <QObject>
#include <QVariantMap>
#include "WorldInfoBookListModel.h"
#include "WorldInfoEntryListModel.h"
#include "WorldInfoEntryEntity.h"

class WorldInfoViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(WorldInfoBookListModel* bookModel READ bookModel CONSTANT)
    Q_PROPERTY(WorldInfoEntryListModel* entryModel READ entryModel CONSTANT)
    Q_PROPERTY(QVariantMap currentEntry READ currentEntry NOTIFY currentEntryChanged)
    Q_PROPERTY(QString currentBookId READ currentBookId NOTIFY currentBookIdChanged)
    Q_PROPERTY(QString currentBookName READ getCurrentBookName WRITE setCurrentBookName NOTIFY currentBookNameChanged FINAL)


public:
    explicit WorldInfoViewModel(QObject* parent = nullptr);
    ~WorldInfoViewModel() override;

    WorldInfoBookListModel* bookModel() const;
    WorldInfoEntryListModel* entryModel() const;
    QVariantMap currentEntry() const;
    QString currentBookId() const;

    // 文集操作
    Q_INVOKABLE void refreshBookList();
    Q_INVOKABLE void selectBook(const QString& bookId);
    Q_INVOKABLE void createBook();
    Q_INVOKABLE void deleteBook();
    Q_INVOKABLE void duplicateBook(const QString& bookId);//复制
    Q_INVOKABLE void toggleBookEnabled(const QString& bookId);//启用
    Q_INVOKABLE void toggleBookGlobal(const QString& bookId);//全局
    Q_INVOKABLE void renameBook(const QString& bookId, const QString& newName);//重命名

    // 条目操作
    Q_INVOKABLE void refreshEntryList();//刷新
    Q_INVOKABLE void selectEntry(const QString& entryId);//选择
    Q_INVOKABLE void createEntry();//新建
    Q_INVOKABLE void saveEntry(const QVariantMap& entryData);//保存
    Q_INVOKABLE void deleteEntry(const QString& entryId);//删除
    Q_INVOKABLE void toggleEntryEnabled(const QString& entryId);//变更开关
    Q_INVOKABLE void moveEntry(const QString& entryId, const QString& targetBookId);//移动
    Q_INVOKABLE void batchDelete(const QStringList& entryIds);//批量删除
    Q_INVOKABLE void batchSetEnabled(const QStringList& entryIds, bool enabled);//批量开关

    // 导入导出
    Q_INVOKABLE void importBook(const QUrl& fileUrl);
    Q_INVOKABLE void exportCurrentBook(const QString& savePath);
    Q_INVOKABLE void exportAllBooks(const QString& savePath);

    QString getCurrentBookName() const;
    void setCurrentBookName(const QString &newCurrentBookName);

signals:
    void currentBookIdChanged();
    void currentEntryChanged();
    void importSuccess(int count);
    void exportSuccess();
    void operationError(const QString& errorMsg);

    void currentBookNameChanged();

private:
    WorldInfoBookListModel* m_bookModel = nullptr;
    WorldInfoEntryListModel* m_entryModel = nullptr;
    QString m_currentBookId;
    WorldInfoEntryEntity m_currentEntry;//()

    QVariantMap entryToMap(const WorldInfoEntryEntity& entry) const;
    WorldInfoEntryEntity mapToEntry(const QVariantMap& map) const;
};

#endif