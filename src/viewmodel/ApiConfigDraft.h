#ifndef APICONFIGDRAFT_H
#define APICONFIGDRAFT_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

#include "ApiConfigEntity.h"

/**
 * @brief API 配置的编辑草稿（强类型）
 *
 * 和 CharacterDraft / CompletionPresetDraft 是同一套模式：
 *   - 表单只改草稿，不直接写库；
 *   - setter 去重、发字段信号 + changed()；
 *   - ViewModel 监听 changed() 做防抖落库，落库后调 markSaved()。
 *
 * 只暴露"可编辑/只读展示"需要的字段，其余实体字段（modelType、extraHeaders、
 * timeout、createdAt 等）原样保留在内部，保存时一起写回，避免丢字段。
 */
class ApiConfigDraft : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString baseDomain READ baseDomain WRITE setBaseDomain NOTIFY baseDomainChanged)
    Q_PROPERTY(QString apiPath READ apiPath WRITE setApiPath NOTIFY apiPathChanged)
    Q_PROPERTY(QStringList modelList READ modelList WRITE setModelList NOTIFY modelListChanged)
    Q_PROPERTY(QString currentModel READ currentModel WRITE setCurrentModel NOTIFY currentModelChanged)
    // 派生：currentModel 在 modelList 里的下标（-1 = 无）
    Q_PROPERTY(int modelsIndex READ modelsIndex NOTIFY modelsIndexChanged)
    Q_PROPERTY(bool isBuiltin READ isBuiltin NOTIFY isBuiltinChanged)
    Q_PROPERTY(bool isActive READ isActive NOTIFY isActiveChanged)
    Q_PROPERTY(bool dirty READ isDirty NOTIFY dirtyChanged)

public:
    explicit ApiConfigDraft(QObject* parent = nullptr);

    // 整块载入（切换配置、拉取模型列表后重新读库）；会清掉 dirty 并发出全部字段信号
    void setEntity(const ApiConfigEntity& entity);
    ApiConfigEntity entity() const { return m_entity; }

    bool isEmpty() const { return m_entity.id.isEmpty(); }
    bool isDirty() const { return m_dirty; }
    void markSaved();

    QString name() const { return m_entity.name; }
    QString apiKey() const { return m_entity.apiKey; }
    QString baseDomain() const { return m_entity.baseDomain; }
    QString apiPath() const { return m_entity.apiPath; }
    QStringList modelList() const { return m_entity.availableModels; }
    QString currentModel() const { return m_entity.currentModel; }
    int modelsIndex() const { return m_entity.availableModels.indexOf(m_entity.currentModel); }
    bool isBuiltin() const { return m_entity.isBuiltin; }
    bool isActive() const { return m_entity.isActive; }

public slots:
    void setName(const QString& value);
    void setApiKey(const QString& value);
    void setBaseDomain(const QString& value);
    void setApiPath(const QString& value);
    void setModelList(const QStringList& value);
    void setCurrentModel(const QString& value);
    // 下拉框用：越界直接忽略
    Q_INVOKABLE void setCurrentModelsIndex(int index);

signals:
    void changed();//任意字段变化（ViewModel 用它做防抖落库）
    void dirtyChanged();
    void nameChanged();
    void apiKeyChanged();
    void baseDomainChanged();
    void apiPathChanged();
    void modelListChanged();
    void currentModelChanged();
    void modelsIndexChanged();
    void isBuiltinChanged();
    void isActiveChanged();

private:
    void markDirty();
    void emitAllFieldChanged();

    ApiConfigEntity m_entity;
    bool m_dirty = false;
};

#endif // APICONFIGDRAFT_H
