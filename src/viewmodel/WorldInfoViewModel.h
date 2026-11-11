#ifndef WORLDINFOVIEWMODEL_H
#define WORLDINFOVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

#include "WorldInfoBookListModel.h"

class QJSEngine;
class WorldInfoService;
class WorldInfoEntryViewModel;
class ExceptionHandler;

/**
 * @brief 世界书文集视图模型（文集域）
 *
 * 职责：文集列表数据源、当前选中文集、文集级增删改/开关/重命名/导入导出。
 * 条目域（条目列表、条目增删改、当前条目快照）已拆分到 WorldInfoEntryViewModel，
 * 本类在 selectBook() 时把当前文集 id 交给它。
 *
 * 对外暴露为 QML 单例，类型名 WorldInfoViewModel。
 */
class WorldInfoViewModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // 供 QML 使用：QML 单例工厂
    static WorldInfoViewModel* create(QQmlEngine*, QJSEngine*)
    {
        static WorldInfoViewModel instance;
        // 生命周期归 C++：静态实例必须由我们销毁，否则退出时被销毁两次导致卡死
        QQmlEngine::setObjectOwnership(&instance, QQmlEngine::CppOwnership);
        return &instance;
    }

    ~WorldInfoViewModel() override;

    // 文集列表数据源（供 QML 使用）
    Q_PROPERTY(WorldInfoBookListModel* bookModel READ bookModel CONSTANT)
    // 当前选中文集 id（供 QML 使用）
    Q_PROPERTY(QString currentBookId READ currentBookId NOTIFY currentBookIdChanged)
    // 当前选中文集名称，可写（供 QML 使用）
    Q_PROPERTY(QString currentBookName READ getCurrentBookName
               WRITE setCurrentBookName NOTIFY currentBookNameChanged FINAL)

    WorldInfoBookListModel* bookModel() const;
    QString currentBookId() const;

    // ---- 供 QML 使用 ----
    Q_INVOKABLE void refreshBookList();
    Q_INVOKABLE void selectBook(const QString& bookId);
    Q_INVOKABLE void createBook();
    Q_INVOKABLE void deleteBook();
    Q_INVOKABLE void toggleBookEnabled(const QString& bookId);
    Q_INVOKABLE void renameBook(const QString& bookId, const QString& newName);
    Q_INVOKABLE void importBook(const QUrl& fileUrl);
    Q_INVOKABLE void exportCurrentBook(const QString& savePath);
    Q_INVOKABLE void exportAllBooks(const QString& savePath);

    QString getCurrentBookName() const;
    void setCurrentBookName(const QString &newCurrentBookName);

signals:
    // 以下信号供 QML 使用
    void currentBookIdChanged();
    void importSuccess(int count);
    void exportSuccess();
    void operationError(const QString& errorMsg);
    void currentBookNameChanged();

private:
    WorldInfoService* m_service = nullptr;
    WorldInfoEntryViewModel* m_entryViewModel = nullptr;
    ExceptionHandler* m_exceptionHandler = nullptr;

    WorldInfoBookListModel* m_bookModel = nullptr;
    QString m_currentBookId;

    // 构造函数必须 private：QML 单例只有在"无法默认构造"时才会调用 create()。
    // 若保持 public，引擎会自己 new 一个实例，C++ 侧的静态实例就与 QML 拿到的
    // 不是同一个对象（会出现"数据写进 A、界面读的是 B"这类极难定位的问题）。
    explicit WorldInfoViewModel(QObject* parent = nullptr);
};

#endif // WORLDINFOVIEWMODEL_H
