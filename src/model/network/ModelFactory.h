#ifndef MODELFACTORY_H
#define MODELFACTORY_H

#include <QObject>
#include <QMap>
#include <functional>

#include "BaseApiClient.h"
#include "GlobalConstants.h"
#include "ApiConfigEntity.h"

//模型工厂基类

class ModelFactory : public QObject {
    Q_OBJECT
public:
    static ModelFactory& instance();
    ~ModelFactory() override = default;

    //创建默认客户端
    BaseApiClient* createClient(GlobalConstants::ModelType type);

    //创建通用客户端,根据API配置智能创建并初始化客户端
    // BaseApiClient* createGenericClient(const ApiConfigEntity& config);


private:
    ModelFactory(QObject* parent = nullptr);

    //注册所有模型
    void registerAllModels();

    using ClientCreator = std::function<BaseApiClient*()>;//别名,函数指针
    QMap<GlobalConstants::ModelType, ClientCreator> m_creators;//模型客户端函数列表,维护函数指针
};

#endif // MODELFACTORY_H
