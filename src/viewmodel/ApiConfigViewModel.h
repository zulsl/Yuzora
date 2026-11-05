#ifndef APICONFIGVIEWMODEL_H
#define APICONFIGVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include "ApiConfigListModel.h"
#include "ApiConfigService.h"

class QQmlEngine;
class QJSEngine;

/**
 * @brief 模型 API 配置视图模型（列表域）
 *
 * 职责：配置列表数据源、当前选中项、激活状态，以及"测试连接 / 拉取模型列表"的进度状态。
 * 表单字段与编辑动作已拆分到 ApiConfigFormViewModel；本类只负责选中后把实体交给它。
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
        return &instance;
    }

    explicit ApiConfigViewModel(QObject* parent = nullptr);
    ~ApiConfigViewModel() override;

    // 配置列表数据源（供 QML 使用）
    Q_PROPERTY(ApiConfigListModel* configListModel READ configListModel CONSTANT)
    // 当前选中配置 id（供 QML 使用）
    Q_PROPERTY(QString currentConfigId READ currentConfigId WRITE setCurrentConfigId
               NOTIFY currentConfigIdChanged)
    // 测试连接中（供 QML 使用）
    Q_PROPERTY(bool testingConnection READ testingConnection NOTIFY testingConnectionChanged)
    // 拉取模型列表中（供 QML 使用）
    Q_PROPERTY(bool fetchModelListState READ fetchModelListState NOTIFY fetchModelListStateChanged)
    // 当前选中项是否为正在使用的配置（供 QML 使用）
    Q_PROPERTY(bool currentIsActive READ currentIsActive NOTIFY currentIsActiveChanged)

    ApiConfigListModel* configListModel() const;
    QString currentConfigId() const;
    bool currentIsActive() const;
    bool testingConnection() const;
    bool fetchModelListState() const;

    // ---- 供 QML 使用 ----
    Q_INVOKABLE void selectConfig(const QString& configId);
    Q_INVOKABLE void setCurrentConfigId(const QString& id);
    Q_INVOKABLE void refreshList();
    Q_INVOKABLE void addCustomConfig(const QString& configName);
    Q_INVOKABLE void deleteCurrentConfig();
    Q_INVOKABLE void testCurrentConnection();
    Q_INVOKABLE void fetchModelList();
    Q_INVOKABLE void applyCurrentConfig();

signals:
    // 以下信号供 QML 使用
    void currentConfigIdChanged();
    void currentIsActiveChanged();
    void testingConnectionChanged();
    void fetchModelListStateChanged();
    void deleteSuccess();
    void testSuccess();
    void operationError(const QString& errorMsg);
    void operationTip(const QString& tipMsg);

private slots:
    void onTestSuccess(const QString& configId);
    void onRequestFailed(const QString& configId, const QString& error);
    void onFetchModelListState(const QString& configId);

private:
    void loadCurrentConfigDetail();

    ApiConfigService* m_service = nullptr;
    ApiConfigListModel* m_configListModel = nullptr;

    QString m_currentConfigId;
    bool m_testingConnection = false;
    bool m_fetchModelListState = false;
};

#endif // APICONFIGVIEWMODEL_H
