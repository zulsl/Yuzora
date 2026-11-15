#include "ChatService.h"

#include "completion/ApiCompletionRunner.h"
#include "greeting/GreetingService.h"
#include "SessionService.h"
#include "CharacterDao.h"
#include "CharacterService.h"
#include "ChatMessageDao.h"
#include "ChatSessionDao.h"
#include "DatabaseManager.h"
#include "SettingsService.h"

// ======================================
// 生产组合根
// ======================================
// 这里只负责"把真家伙拼起来"：DAO 与网关归 ChatService 所有，
// CharacterService / SettingsService 是全局单例，只借用。
// 业务逻辑一行都不写在这个文件里，所以单测可以不链接它。

namespace {

// 生产事务实现：直接转 DatabaseManager（无状态，常驻即可）
class DbTransactionScope : public TransactionScope {
public:
    bool begin() override { return DatabaseManager::instance().beginTransaction(); }
    bool commit() override { return DatabaseManager::instance().commitTransaction(); }
    void rollback() override { DatabaseManager::instance().rollbackTransaction(); }
};

TransactionScope* productionTransactionScope()
{
    static DbTransactionScope scope;
    return &scope;
}

ChatServiceDependencies productionDependencies()
{
    ChatServiceDependencies deps;
    deps.messageDao = new ChatMessageDao();
    deps.characterDao = new CharacterDao();
    deps.characterService = &CharacterService::instance();
    deps.settingsService = &SettingsService::instance();
    deps.completion = new ApiCompletionRunner();
    deps.transaction = productionTransactionScope();
    deps.greeting = new GreetingService(deps.messageDao, deps.characterService);

    // 会话 DAO 归 SessionService 所有（父对象 = SessionService），ChatService 不再直接碰会话表
    auto* sessionDao = new ChatSessionDao();
    auto* sessions = new SessionService(sessionDao, deps.characterDao, deps.settingsService);
    sessionDao->setParent(sessions);
    deps.sessions = sessions;

    return deps;
}

} // namespace

ChatService& ChatService::instance()
{
    static ChatService instance;
    return instance;
}

ChatService::ChatService(QObject* parent)
    : ChatService(productionDependencies(), parent)
{
    m_messageDao->setParent(this);
    m_characterDao->setParent(this);
    m_completion->setParent(this);
    m_greeting->setParent(this);
    m_sessions->setParent(this);
}
