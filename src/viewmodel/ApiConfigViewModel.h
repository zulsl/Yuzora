#ifndef APICONFIGVIEWMODEL_H
#define APICONFIGVIEWMODEL_H

#include <QObject>
#include "ApiConfigListModel.h"
#include "ApiConfigService.h"

class ApiConfigViewModel : public QObject {
    Q_OBJECT
    //数据源
    Q_PROPERTY(ApiConfigListModel* configListModel READ configListModel CONSTANT)
// 当前选中配置索引与ID
    //当前选中项
    Q_PROPERTY(QString currentConfigId READ currentConfigId WRITE setCurrentConfigId NOTIFY currentConfigIdChanged)


// 动态表单控制
    //是否为内置预设
    Q_PROPERTY(bool isCurrentBuiltin READ isCurrentBuiltin NOTIFY currentConfigChanged)
    //域名,同上(弃用)
    Q_PROPERTY(bool showDomainInput READ showDomainInput NOTIFY currentConfigChanged)
// 当前配置表单字段读写
    //配置名
    Q_PROPERTY(QString configName READ configName WRITE setConfigName NOTIFY configNameChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString baseDomain READ baseDomain WRITE setBaseDomain NOTIFY baseDomainChanged)
    Q_PROPERTY(QString apiPath READ apiPath WRITE setApiPath NOTIFY apiPathChanged)
    // 子模型列表
    Q_PROPERTY(QStringList modelList READ modelList NOTIFY modelListChanged)
    Q_PROPERTY(int currentModelsIndex READ currentModelsIndex WRITE setCurrentModelsIndex NOTIFY currentModelsIndexChanged FINAL)
    // 测试状态
    Q_PROPERTY(bool testingConnection READ testingConnection NOTIFY testingConnectionChanged)
    // 拉取状态
    Q_PROPERTY(bool fetchModelListState READ fetchModelListState NOTIFY fetchModelListStateChanged)
    Q_PROPERTY(bool currentIsActive READ currentIsActive NOTIFY currentIsActiveChanged)

public:
    explicit ApiConfigViewModel(QObject* parent = nullptr);
    ~ApiConfigViewModel() override;

//get
    ApiConfigListModel* configListModel() const;    //读取配置列表
    QString currentConfigId() const;                //当前配置id
    bool isCurrentBuiltin() const;                  //是否预设模型
    bool currentIsActive()const;                    //当前项是否激活
    bool showDomainInput() const;                   //自定义配置显示域名和路径输入
    QString configName() const;                     //配置名
    QString apiKey() const;                         //api密钥
    QString baseDomain() const;                     //基础域名
    QString apiPath() const;                        //api路径
    QStringList modelList() const;                  //子模型列表
    bool testingConnection() const;                 //测试连接状态
    bool fetchModelListState()const;                //获取模型状态

//set
    Q_INVOKABLE void selectConfig(const QString& configId);//选中
    Q_INVOKABLE void setConfigName(const QString& name);        //设置配置名字
    Q_INVOKABLE void setApiKey(const QString& key);
    Q_INVOKABLE void setBaseDomain(const QString& domain);
    Q_INVOKABLE void setApiPath(const QString& path);
    Q_INVOKABLE void setCurrentConfigId(const QString& id);

    // 操作接口
    Q_INVOKABLE void refreshList();      //刷新配置列表
    Q_INVOKABLE void addCustomConfig(const QString &configName);     //点击添加按钮新建自定义配置
    Q_INVOKABLE void deleteCurrentConfig(); //删除配置
    Q_INVOKABLE void testCurrentConnection();   //测试连接
    Q_INVOKABLE void fetchModelList();  //获取模型列表
    Q_INVOKABLE void applyCurrentConfig(); // 设为当前使用的配置

    //子模型列表索引
    int currentModelsIndex() const;
    Q_INVOKABLE void setCurrentModelsIndex(int newIndex);

signals:
    void modelListChanged();
    void currentConfigIdChanged();    //当前配置ID已更改
    void currentConfigChanged();
    void configNameChanged();
    void apiKeyChanged();
    void baseDomainChanged();
    void apiPathChanged();
    void currentIsActiveChanged();
    void testingConnectionChanged();//连接状态
    void fetchModelListStateChanged();//拉取状态
    void deleteSuccess();
    void testSuccess();
    void operationError(const QString& errorMsg);//错误信息
    void operationTip(const QString& tipMsg);//提示信息


    void currentModelsIndexChanged();

private slots:
    void onTestSuccess(const QString& configId);
    void onRequestFailed(const QString& configId, const QString& error);

    void onFetchModelListState(const QString& configId);

private:
    //工具函数
    int getModelsIndex();

    void loadCurrentConfigDetail();//加载当前配置表单
    void saveCurrentConfig();   //保存当前编辑配置


    ApiConfigService* m_service = nullptr;//api配置核心单例业务
    ApiConfigListModel* m_configListModel = nullptr;//列表数据源

    // int m_currentIndex = -1;//当前配置列表索引
    int m_currentModelsIndex = -1;


    QString m_currentConfigId;//当前配置id(索引和查询用)
    ApiConfigEntity m_currentSelectConfig;//当前选中项临时表单信息

    bool m_testingConnection = false;
    bool m_fetchModelListState = false;//转圈状态
};

#endif // APICONFIGVIEWMODEL_H