#include "WorldInfoViewModel.h"
#include "WorldInfoEntryViewModel.h"
#include "WorldInfoService.h"
#include "WorldInfoBookItemModel.h"
#include "WorldInfoBookEntity.h"
#include "ExceptionHandler.h"

// ======================================
// 构造与析构
// ======================================
WorldInfoViewModel::WorldInfoViewModel(QObject* parent)
    : QObject(parent)
    , m_bookModel(new WorldInfoBookListModel(this))
{
    auto& service = WorldInfoService::instance();

    connect(&service, &WorldInfoService::operationError,
            this, &WorldInfoViewModel::operationError);

    // 初始化加载文集列表
    refreshBookList();
}

WorldInfoViewModel::~WorldInfoViewModel() = default;

// ======================================
// 属性读取
// ======================================
WorldInfoBookListModel* WorldInfoViewModel::bookModel() const
{
    return m_bookModel;
}

QString WorldInfoViewModel::currentBookId() const
{
    return m_currentBookId;
}

// ======================================
// 文集操作
// ======================================
void WorldInfoViewModel::refreshBookList()
{
    QList<WorldInfoBookEntity> books = WorldInfoService::instance().getAllBooks();
    QList<WorldInfoBookItemModel*> items;
    items.reserve(books.size());

    for (const auto& book : books)
        items.append(new WorldInfoBookItemModel(book));

    m_bookModel->resetData(items);
}

void WorldInfoViewModel::selectBook(const QString& bookId)
{
    if (m_currentBookId == bookId) return;

    m_currentBookId = bookId;

    emit currentBookIdChanged();
    emit currentBookNameChanged();

    // 条目域跟着切换（清空当前条目并重新加载列表）
    WorldInfoEntryViewModel::instance()->setCurrentBookId(bookId);
}

void WorldInfoViewModel::createBook()
{
    QString id = WorldInfoService::instance().createBook(QStringLiteral("新世界书"));
    if (!id.isEmpty())
        selectBook(id); // 新建后自动选中
}

void WorldInfoViewModel::deleteBook()
{
    try {
        if (m_currentBookId.isEmpty())
            THROW(UnknownError, "[WorldInfoViewModel]m_currentBookId为空");

        bool success = WorldInfoService::instance().deleteBook(m_currentBookId);
        if (success) {
            m_currentBookId.clear();
            emit currentBookIdChanged();
            // 清空条目域
            WorldInfoEntryViewModel::instance()->setCurrentBookId(QString());
        }
        refreshBookList();

        ExceptionHandler::instance().tipOccurred("删除成功");
    }
    catch (const AppException& e) { ExceptionHandler::instance().handleException(e); }
    catch (const std::exception& e) { ExceptionHandler::instance().handleException(e); }
    catch (...) { ExceptionHandler::instance().handleException(); }
}

void WorldInfoViewModel::duplicateBook(const QString& bookId)
{
    QString newId = WorldInfoService::instance().duplicateBook(bookId);
    if (!newId.isEmpty())
        selectBook(newId);
}

void WorldInfoViewModel::toggleBookEnabled(const QString& bookId)
{
    WorldInfoService::instance().toggleBookEnabled(bookId);
    refreshBookList();
}

void WorldInfoViewModel::toggleBookGlobal(const QString& bookId)
{
    Q_UNUSED(bookId)
}

void WorldInfoViewModel::renameBook(const QString& bookId, const QString& newName)
{
    QString trimmedName = newName.trimmed();
    if (trimmedName.isEmpty()) {
        emit operationError(QStringLiteral("文集名称不能为空"));
        return;
    }

    WorldInfoBookEntity book = WorldInfoService::instance().getBookById(bookId);
    if (book.id.isEmpty()) return;

    book.name = trimmedName;
    WorldInfoService::instance().updateBook(book);
}

// ======================================
// 导入导出（SillyTavern 格式）
// ======================================
void WorldInfoViewModel::importBook(const QUrl& fileUrl)
{
    QString filePath = fileUrl.toLocalFile();
    WorldInfoService::instance().importBookFromSillyTavern(filePath);
}

void WorldInfoViewModel::exportCurrentBook(const QString& savePath)
{
    if (m_currentBookId.isEmpty()) {
        emit operationError(QStringLiteral("请先选择要导出的文集"));
        return;
    }
    WorldInfoService::instance().exportBookToSillyTavern(m_currentBookId, savePath);
}

void WorldInfoViewModel::exportAllBooks(const QString& savePath)
{
    WorldInfoService::instance().exportAllBooksToSillyTavern(savePath);
}

// ======================================
// 当前文集名称
// ======================================
QString WorldInfoViewModel::getCurrentBookName() const
{
    if (m_currentBookId.isEmpty())
        return {};

    WorldInfoBookEntity entity = WorldInfoService::instance().getBookById(m_currentBookId);
    return entity.name;
}

void WorldInfoViewModel::setCurrentBookName(const QString &newCurrentBookName)
{
    try {
        if (m_currentBookId.isEmpty())
            THROW(UnknownError, "[WorldInfoViewModel]m_currentBookId为空");

        WorldInfoBookEntity entity = WorldInfoService::instance().getBookById(m_currentBookId);
        entity.name = newCurrentBookName;
        WorldInfoService::instance().updateBook(entity);
        emit currentBookNameChanged();
    }
    catch (const AppException& e) { ExceptionHandler::instance().handleException(e); }
    catch (const std::exception& e) { ExceptionHandler::instance().handleException(e); }
    catch (...) { ExceptionHandler::instance().handleException(); }
}
