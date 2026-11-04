#include "ModelFactory.h"
#include "DeepSeekApiClient.h"
#include "OpenAICompatibleApiClient.h"
#include <QLoggingCategory>

//自定义日志分类,用来区分不同业务模块的打印信息
static Q_LOGGING_CATEGORY(lcModelFactory, "qt.Yuzora.modelfactory")

ModelFactory::ModelFactory(QObject* parent)
    : QObject(parent)
{
    registerAllModels();
}

ModelFactory& ModelFactory::instance()
{
    static ModelFactory factory;
    return factory;
}

void ModelFactory::registerAllModels()
{
    // 注册模型客户端创建函数,多态
    //匿名函数,BaseApiClient*为返回值
    m_creators[GlobalConstants::ModelType::DeepSeek] = []() -> BaseApiClient* {
        return new OpenAICompatibleApiClient();
    };
    m_creators[GlobalConstants::ModelType::OpenAI] =[]() -> BaseApiClient* {
        return new OpenAICompatibleApiClient();
    };
    // 以下模型为预留扩展，未实现具体客户端，创建时返回 nullptr
    m_creators[GlobalConstants::ModelType::Claude] = nullptr;
    m_creators[GlobalConstants::ModelType::Doubao] = nullptr;
    m_creators[GlobalConstants::ModelType::Gemini] = nullptr;
    m_creators[GlobalConstants::ModelType::Unknown] = nullptr;
}

// void ModelFactory::onModelListFetched(GlobalConstants::ModelType type, const QStringList &models)
// {
//     m_dynamicModelCache[type] = models;
//     emit modelListReady(type, models);
// }
// void ModelFactory::onModelListFetchError(GlobalConstants::ModelType type, const QString &error)
// {
//     emit modelListLoadFailed(type, error);
// }

BaseApiClient* ModelFactory::createClient(GlobalConstants::ModelType type)
{
//创建客户端实例
    //没有包含模型
    if (!m_creators.contains(type)) {
        qCWarning(lcModelFactory) << "没有为该模型类型注册创建者:" << static_cast<int>(type);
        return nullptr;
    }
    //取出模型创建函数
    auto creator = m_creators[type];
    if (!creator) {
        qCWarning(lcModelFactory) << "模型类型的创建函数为空:" << static_cast<int>(type);
        return nullptr;
    }
    //创建模型客户端实例
    BaseApiClient* client = creator();
    if (!client) {
        qCWarning(lcModelFactory) << "无法为模型类型创建客户端:" << static_cast<int>(type);
        return nullptr;
    }
    return client;
}

// void ModelFactory::setDynamicModelNames(GlobalConstants::ModelType type,const QStringList &models)
// {
//     m_dynamicModelCache[type] = models;
//     emit modelListReady(type, models);
// }

// QList<GlobalConstants::ModelType> ModelFactory::getSupportedModels()
// {

//     QList<GlobalConstants::ModelType> supported;
//     for (auto it = m_creators.begin(); it != m_creators.end(); ++it) {
//         if (it.value() != nullptr) {
//             supported.append(it.key());
//         }
//     }
//     return supported;
// }

// QString ModelFactory::getModelDisplayName(GlobalConstants::ModelType type)
// {
//     if (m_displayNames.contains(type)) {
//         return m_displayNames[type];
//     }
//     return "Unknown";
// }

// QStringList ModelFactory::getFallbackModelNames(GlobalConstants::ModelType type)
// {
//     return m_fallbackModels.value(type, {});
// }

// QStringList ModelFactory::getDynamicModelNames(GlobalConstants::ModelType type)
// {
//     // 有动态缓存则返回，否则返回空（上层自动走兜底）
//     if (m_dynamicModelCache.contains(type))
//         return m_dynamicModelCache[type];
//     return {};
// }