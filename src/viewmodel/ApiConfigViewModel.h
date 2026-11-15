#ifndef APICONFIGVIEWMODEL_H
#define APICONFIGVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

#include "ApiConfigDraft.h"
#include "ApiConfigListModel.h"
#include "ApiConfigService.h"

class QJSEngine;

/**
 * @brief 模型 API 配置页的视图模型
 *
 * 职责：配置列表/选中、草稿装配、防抖落库、拉取模型列表的异步状态。
 * 字段读写全部走 ApiConfigDraft（见该类的注释），VM 不再自己存一份实体快照。
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

    // 测试/组合根可注入 Service；生产走 create()。
    explicit ApiConfigViewModel(ApiConfigService* service, QObject* parent = nullptr);
    ~ApiConfigViewModel() override;

    Q_PROPERTY(ApiConfigListModel* configListModel READ configListModel CONSTANT)
    Q_PROPERTY(QString currentConfigId READ currentConfigId NOTIFY currentConfigIdChanged)
    Q_PROPERTY(ApiConfigDraft* draft READ draft CONSTANT)
    Q_PROPERTY(bool isFetchingModels READ isFetchingModels NOTIFY isFetchingModelsChanged)

    ApiConfigListModel* configListModel() const;
    QString currentConfigId() const;
    ApiConfigDraft* draft() const;
    bool isFetchingModels() const;

    // ---- 供 QML 使用 ----
    Q_INVOKABLE void selectConfig(const QString& configId);
    Q_INVOKABLE bool addCustomConfig(const QString& configName);
    Q_INVOKABLE void deleteCurrentConfig();
    Q_INVOKABLE void fetchModelList();
    Q_INVOKABLE void applyCurrentConfig();

signals:
    void currentConfigIdChanged();
    void isFetchingModelsChanged();

    // 通用提示（来自 Service）
    void operationError(const QString& errorMsg);
    void operationTip(const QString& tipMsg);

private slots:
    void onFetchModelListState(const QString& configId);
    void onModelListUpdated(const QString& configId);

private:
    void refreshList();
    void loadDraft(const QString& configId);
    // 保证 currentModel 落在列表里（空值/失效则取第一条并立即落库）
    void ensureCurrentModelSelected();
    void scheduleSave();
    void flushPendingSave();
    void cancelPendingSave();

    ApiConfigService* m_service = nullptr;
    ApiConfigListModel* m_configListModel = nullptr;
    ApiConfigDraft* m_draft = nullptr;

    // 防抖：键盘输入期间不写库，停手 300ms 后统一落库
    QTimer m_saveTimer;

    QString m_currentConfigId;
    bool m_fetching = false;

    // 构造函数必须 private：QML 单例只有在"无法默认构造"时才会调用 create()。
    // 若保持 public，引擎会自己 new 一个实例，C++ 侧的静态实例就与 QML 拿到的
    // 不是同一个对象（会出现"数据写进 A、界面读的是 B"这类极难定位的问题）。
    explicit ApiConfigViewModel(QObject* parent = nullptr);
};

#endif // APICONFIGVIEWMODEL_H
