// #ifndef MODELFACTORY_T_H
// #define MODELFACTORY_T_H

// #include <QObject>
// #include <QMap>
// #include <functional>

// #include "BaseApiClient.h"
// #include "GlobalConstants.h"

// //模型工厂基类

// class ModelFactory : public QObject {
//     Q_OBJECT
// public:
//     static ModelFactory& instance();
//     ~ModelFactory() override = default;

//     //创建客户端
//     BaseApiClient* createClient(GlobalConstants::ModelType type);

//     //创建通用客户端(未完成)
//     BaseApiClient* createGenericClient(const QString& baseUrl, const QString& apiKey, const QJsonObject& extraHeaders);


//     // //设置当前已动态拉取的模型列表
//     // void setDynamicModelNames(GlobalConstants::ModelType type,const QStringList &models);
//     // //获取当前已动态拉取的模型列表（优先使用接口数据）
//     // QStringList getDynamicModelNames(GlobalConstants::ModelType type);
//     // // 3. 兜底：内置静态默认模型（接口失败时使用）
//     // QStringList getFallbackModelNames(GlobalConstants::ModelType type);
//     // //获取支持的型号(需优化)
//     // QList<GlobalConstants::ModelType> getSupportedModels();
//     // //获取大模型名称(可优化)
//     // QString getModelDisplayName(GlobalConstants::ModelType type);
//     // //获取子模型名称
//     // QList<QString> getDefaultModelNames(GlobalConstants::ModelType type);
// signals:
//     // //模型列表就绪
//     // void modelListReady(GlobalConstants::ModelType type, const QStringList& models);
//     // //模型列表加载失败
//     // void modelListLoadFailed(GlobalConstants::ModelType type, const QString& error);

// private:
//     ModelFactory(QObject* parent = nullptr);

//     //注册所有模型
//     void registerAllModels();

//     using ClientCreator = std::function<BaseApiClient*()>;//别名,函数指针
//     QMap<GlobalConstants::ModelType, ClientCreator> m_creators;//模型客户端函数列表,维护函数指针

//     // QMap<GlobalConstants::ModelType, BaseApiClient*> m_displayNames;//ai大模型列表
//     // QMap<GlobalConstants::ModelType, QStringList> m_dynamicModelCache;//子模型列表
//     // QMap<GlobalConstants::ModelType, QStringList> m_fallbackModels; //兜底静态子模型
// };

// #endif // MODELFACTORY_T_H
