#ifndef APICONFIGFORMVIEWMODEL_H
#define APICONFIGFORMVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

#include "ApiConfigEntity.h"

class QJSEngine;

/**
 * @brief API 配置表单视图模型（表单域）
 *
 * 职责：持有"当前正在编辑的那份配置实体"快照，暴露表单字段的读写与保存。
 * 与列表域的边界：本类不知道配置列表、也不做选中，只接收 ApiConfigViewModel
 * 通过 loadConfig() 交来的实体；字段修改后立即写回 Service（沿用原有语义）。
 *
 * 对外暴露为 QML 单例，类型名 ApiConfigFormViewModel。
 */
class ApiConfigFormViewModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    static ApiConfigFormViewModel* instance();
    // 供 QML 使用：QML 单例工厂
    static ApiConfigFormViewModel* create(QQmlEngine*, QJSEngine*)
    {
        // 必须复用 instance() 的同一个对象，不能再定义一份静态实例
        ApiConfigFormViewModel* inst = instance();
        QQmlEngine::setObjectOwnership(inst, QQmlEngine::CppOwnership);
        return inst;
    }

    ~ApiConfigFormViewModel() override;

    // 是否为内置配置（供 QML 使用）
    Q_PROPERTY(bool isCurrentBuiltin READ isBuiltin NOTIFY currentConfigChanged)
    // 是否显示域名/路径输入（供 QML 使用）
    Q_PROPERTY(bool showDomainInput READ showDomainInput NOTIFY currentConfigChanged)
    // 配置名（供 QML 使用）
    Q_PROPERTY(QString configName READ configName WRITE setConfigName NOTIFY configNameChanged)
    // API 密钥（供 QML 使用）
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    // 基础域名（供 QML 使用）
    Q_PROPERTY(QString baseDomain READ baseDomain WRITE setBaseDomain NOTIFY baseDomainChanged)
    // 接口路径（供 QML 使用）
    Q_PROPERTY(QString apiPath READ apiPath WRITE setApiPath NOTIFY apiPathChanged)
    // 可用模型列表（供 QML 使用）
    Q_PROPERTY(QStringList modelList READ modelList NOTIFY modelListChanged)
    // 当前模型索引（供 QML 使用）
    Q_PROPERTY(int currentModelsIndex READ currentModelsIndex WRITE setCurrentModelsIndex
               NOTIFY currentModelsIndexChanged FINAL)

    bool isBuiltin() const;
    bool isActive() const;
    bool showDomainInput() const;
    QString configName() const;
    QString apiKey() const;
    QString baseDomain() const;
    QString apiPath() const;
    QStringList modelList() const;
    int currentModelsIndex() const;

    // ---- 供 QML 使用 ----
    Q_INVOKABLE void setConfigName(const QString& name);
    Q_INVOKABLE void setApiKey(const QString& key);
    Q_INVOKABLE void setBaseDomain(const QString& domain);
    Q_INVOKABLE void setApiPath(const QString& path);
    Q_INVOKABLE void setCurrentModelsIndex(int newIndex);

    // ---- 供 C++（ApiConfigViewModel）使用 ----
    // 载入一份实体到表单并广播全部字段变更
    void loadConfig(const ApiConfigEntity& config);
    // 当前表单实体（只读访问）
    const ApiConfigEntity& entity() const { return m_currentSelectConfig; }

signals:
    // 以下信号供 QML 使用
    void currentConfigChanged();
    void configNameChanged();
    void apiKeyChanged();
    void baseDomainChanged();
    void apiPathChanged();
    void modelListChanged();
    void currentModelsIndexChanged();

private:
    int getModelsIndex();
    void saveCurrentConfig();

    ApiConfigEntity m_currentSelectConfig;
    int m_currentModelsIndex = -1;

    // 构造函数必须 private：QML 单例只有在"无法默认构造"时才会调用 create()。
    // 若保持 public，引擎会自己 new 一个实例，C++ 侧的静态实例就与 QML 拿到的
    // 不是同一个对象（会出现"数据写进 A、界面读的是 B"这类极难定位的问题）。
    explicit ApiConfigFormViewModel(QObject* parent = nullptr);
};

#endif // APICONFIGFORMVIEWMODEL_H
