#ifndef APICONFIGVIEWMODEL_H
#define APICONFIGVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

#include "ApiConfigEntity.h"
#include "ApiConfigListModel.h"
#include "ApiConfigService.h"

class QJSEngine;

/**
 * @brief 模型 API 配置视图模型（页面级）
 *
 * 职责：配置列表/选中、表单字段快照、测试与拉取状态。
 * 方案 A：原 ApiConfigFormViewModel 已合并回本类，避免两个 QML 单例
 * 各自持有一份“当前配置”导致同步问题。
 *
 * 对外暴露为 QML 单例，类型名 ApiConfigViewModel。
 */
class ApiConfigViewModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // 供 QML 使用：QML 单例工厂
    static ApiConfigViewModel* create(QQmlEngine*, QJSEngine*)
    {
        static ApiConfigViewModel instance;
        // 生命周期归 C++：静态实例必须由我们销毁，否则退出时被销毁两次导致卡死
        QQmlEngine::setObjectOwnership(&instance, QQmlEngine::CppOwnership);
        return &instance;
    }

    ~ApiConfigViewModel() override;

    // ---- 列表/选中 ----
    Q_PROPERTY(ApiConfigListModel* configListModel READ configListModel CONSTANT)
    Q_PROPERTY(QString currentConfigId READ currentConfigId WRITE setCurrentConfigId
               NOTIFY currentConfigIdChanged)
    Q_PROPERTY(bool currentIsActive READ currentIsActive NOTIFY currentIsActiveChanged)

    // ---- 表单字段 ----
    Q_PROPERTY(bool isCurrentBuiltin READ isBuiltin NOTIFY currentConfigChanged)
    Q_PROPERTY(bool showDomainInput READ showDomainInput NOTIFY currentConfigChanged)
    Q_PROPERTY(QString configName READ configName WRITE setConfigName NOTIFY configNameChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString baseDomain READ baseDomain WRITE setBaseDomain NOTIFY baseDomainChanged)
    Q_PROPERTY(QString apiPath READ apiPath WRITE setApiPath NOTIFY apiPathChanged)
    Q_PROPERTY(QStringList modelList READ modelList NOTIFY modelListChanged)
    Q_PROPERTY(int currentModelsIndex READ currentModelsIndex WRITE setCurrentModelsIndex
               NOTIFY currentModelsIndexChanged FINAL)

    // ---- 异步状态 ----
    Q_PROPERTY(bool fetchModelListState READ fetchModelListState NOTIFY fetchModelListStateChanged)

    // ---- 列表/选中 ----
    ApiConfigListModel* configListModel() const;
    QString currentConfigId() const;
    bool currentIsActive() const;
    bool fetchModelListState() const;

    // ---- 表单 ----
    bool isBuiltin() const;
    bool showDomainInput() const;
    QString configName() const;
    QString apiKey() const;
    QString baseDomain() const;
    QString apiPath() const;
    QStringList modelList() const;
    int currentModelsIndex() const;

    // ---- 供 QML 使用 ----
    Q_INVOKABLE void selectConfig(const QString& configId);
    Q_INVOKABLE void setCurrentConfigId(const QString& id);
    Q_INVOKABLE void refreshList();
    Q_INVOKABLE bool addCustomConfig(const QString& configName);
    Q_INVOKABLE void deleteCurrentConfig();
    Q_INVOKABLE void fetchModelList();
    Q_INVOKABLE void applyCurrentConfig();

    Q_INVOKABLE void setConfigName(const QString& name);
    Q_INVOKABLE void setApiKey(const QString& key);
    Q_INVOKABLE void setBaseDomain(const QString& domain);
    Q_INVOKABLE void setApiPath(const QString& path);
    Q_INVOKABLE void setCurrentModelsIndex(int newIndex);

signals:
    // 列表/选中
    void currentConfigIdChanged();
    void currentIsActiveChanged();
    void fetchModelListStateChanged();
    void deleteSuccess();

    // 表单
    void currentConfigChanged();
    void configNameChanged();
    void apiKeyChanged();
    void baseDomainChanged();
    void apiPathChanged();
    void modelListChanged();
    void currentModelsIndexChanged();

    // 通用提示
    void operationError(const QString& errorMsg);
    void operationTip(const QString& tipMsg);

private slots:
    void onFetchModelListState(const QString& configId);

private:
    void loadCurrentConfigDetail();
    void saveCurrentConfig();
    int getModelsIndex();

    ApiConfigService* m_service = nullptr;
    ApiConfigListModel* m_configListModel = nullptr;

    QString m_currentConfigId;

    // 当前编辑快照（唯一一份）
    ApiConfigEntity m_currentSelectConfig;
    int m_currentModelsIndex = -1;

    bool m_fetchModelListState = false;

    // 构造函数必须 private：QML 单例只有在"无法默认构造"时才会调用 create()。
    // 若保持 public，引擎会自己 new 一个实例，C++ 侧的静态实例就与 QML 拿到的
    // 不是同一个对象（会出现"数据写进 A、界面读的是 B"这类极难定位的问题）。
    explicit ApiConfigViewModel(QObject* parent = nullptr);
};

#endif // APICONFIGVIEWMODEL_H
