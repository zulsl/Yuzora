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
    , m_service(&WorldInfoService::instance())
    , m_entryViewModel(WorldInfoEntryViewModel::instance())
    , m_exceptionHandler(&ExceptionHandler::instance())
    , m_bookModel(new WorldInfoBookListModel(this))
{
    connect(m_service, &WorldInfoService::operationError,
            this, &WorldInfoViewModel::operationError);
    connect(m_service, &WorldInfoService::bookListChanged,
            this, &WorldInfoViewModel::refreshBookList);
    connect(m_service, &WorldInfoService::importSuccess,
            this, &WorldInfoViewModel::importSuccess);
    connect(m_service, &WorldInfoService::exportSuccess,
            this, &WorldInfoViewModel::exportSuccess);

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
    QList<WorldInfoBookEntity> books = m_service->getAllBooks();
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
    m_entryViewModel->setCurrentBookId(bookId);
}

void WorldInfoViewModel::createBook()
{
    QString id = m_service->createBook(QStringLiteral("新世界书"));
    if (!id.isEmpty())
        selectBook(id); // 新建后自动选中
}

void WorldInfoViewModel::deleteBook()
{
    try {
        if (m_currentBookId.isEmpty())
            THROW(UnknownError, "[WorldInfoViewModel]m_currentBookId为空");

        bool success = m_service->deleteBook(m_currentBookId);
        if (success) {
            m_currentBookId.clear();
            emit currentBookIdChanged();
            // 清空条目域
            m_entryViewModel->setCurrentBookId(QString());
        }
        refreshBookList();

        m_exceptionHandler->tipOccurred("删除成功");
    }
    catch (const AppException& e) { m_exceptionHandler->handleException(e); }
    catch (const std::exception& e) { m_exceptionHandler->handleException(e); }
    catch (...) { m_exceptionHandler->handleException(); }
}

void WorldInfoViewModel::toggleBookEnabled(const QString& bookId)
{
    m_service->toggleBookEnabled(bookId);
    refreshBookList();
}

void WorldInfoViewModel::renameBook(const QString& bookId, const QString& newName)
{
    QString trimmedName = newName.trimmed();
    if (trimmedName.isEmpty()) {
        emit operationError(QStringLiteral("文集名称不能为空"));
        return;
    }

    WorldInfoBookEntity book = m_service->getBookById(bookId);
    if (book.id.isEmpty()) return;

    book.name = trimmedName;
    if (!m_service->updateBook(book)) {
        return;
    }
    emit currentBookNameChanged();
}

// ======================================
// 导入导出（SillyTavern 格式）
// ======================================
void WorldInfoViewModel::importBook(const QUrl& fileUrl)
{
    QString filePath = fileUrl.toLocalFile();
    m_service->importBookFromSillyTavern(filePath);
}

void WorldInfoViewModel::exportCurrentBook(const QUrl& fileUrl)
{
    QString filePath = fileUrl.toLocalFile();
    if (m_currentBookId.isEmpty()) {
        emit operationError(QStringLiteral("请先选择要导出的文集"));
        return;
    }
    m_service->exportBookToSillyTavern(m_currentBookId, filePath);
}

void WorldInfoViewModel::exportAllBooks(const QString& savePath)
{
    m_service->exportAllBooksToSillyTavern(savePath);
}

// ======================================
// 当前文集名称
// ======================================
QString WorldInfoViewModel::getCurrentBookName() const
{
    if (m_currentBookId.isEmpty())
        return {};

    WorldInfoBookEntity entity = m_service->getBookById(m_currentBookId);
    return entity.name;
}

void WorldInfoViewModel::setCurrentBookName(const QString &newCurrentBookName)
{
    try {
        if (m_currentBookId.isEmpty())
            THROW(UnknownError, "[WorldInfoViewModel]m_currentBookId为空");

        WorldInfoBookEntity entity = m_service->getBookById(m_currentBookId);
        entity.name = newCurrentBookName;
        if (!m_service->updateBook(entity)) {
            return;
        }
        emit currentBookNameChanged();
    }
    catch (const AppException& e) { m_exceptionHandler->handleException(e); }
    catch (const std::exception& e) { m_exceptionHandler->handleException(e); }
    catch (...) { m_exceptionHandler->handleException(); }
}
