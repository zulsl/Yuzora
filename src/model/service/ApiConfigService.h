#ifndef APICONFIGSERVICE_H
#define APICONFIGSERVICE_H

#include <QObject>
#include <QList>
#include "ApiConfigDao.h"
#include "ApiConfigEntity.h"

//api配置核心业务

class ApiConfigService : public QObject {
    Q_OBJECT
public:
    static ApiConfigService& instance();
    ~ApiConfigService() override;

// 列表与查询(DAO)
    QList<ApiConfigEntity> getAllConfigs();
    ApiConfigEntity getConfigById(const QString& id);//按id选择
    ApiConfigEntity getActiveConfig()const;//获取当前激活配置

// 配置管理
    QString createCustomConfig(const QString& name); // 新建空白自定义配置
    bool updateConfig(const ApiConfigEntity& config);//更新
    bool deleteConfig(const QString& id); // 仅可删除自定义配置
    bool setActiveConfig(const QString& id);//设置当前激活的配置

    // 子模型管理(待优化)
    bool updateModelList(const QString& configId,const QStringList& modelNames);

    // 连接测试
    void testConnection(const QString& configId);
    //获取模型列表
    void fetchModelList(const QString& configId);

signals:
    void configUpdated(const QString& configId);//配置更新
    void configDeleted(const QString& configId);//配置删除

    //网络操作信号
    void testSuccess(const QString& configId);

    void fetchModelListEnd(const QString& configId);

    void operationTip(const QString& tipMsg);//提示信息
    void operationError(const QString& errorMsg);//操作错误

private:
    explicit ApiConfigService(QObject* parent = nullptr);

    void initBuiltinConfigs(); // 首次启动初始化内置预设
    bool isBuiltinConfig(const QString& id);

    ApiConfigDao* m_dao;
};

#endif // APICONFIGSERVICE_H