#ifndef PRESETPROMPTLISTMODEL_H
#define PRESETPROMPTLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "PromptEntity.h"

class PresetPromptListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        IdentifierRole,
        NameRole,//名称
        MessageTypeRole,//消息类型
        ContentRole,//内容
        TriggerTypeRole,//触发器
        PositionRole,//位置
        IsActiveRole,//启用
        DepthRole,//深度
        SortOrderRole,//顺序
        SystemPromptRole,//系统内置提示词
        MarkerRole,//标记/占位提示词（内容由运行时填充，不可编辑）
        ExpandedStateRole// 展开状态
    };

    explicit PresetPromptListModel(QObject* parent = nullptr);
    ~PresetPromptListModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    void loadList(const QList<PromptEntity>& list);

    void appendItem(PromptEntity& item);
    void removeItem(const QString& id);
    void updateItem(const PromptEntity& entry);
    void clear();
    PromptEntity entryById(const QString& id) const;
    // ViewModel 专用：更新业务字段，不触发持久化
    bool updateField(const QString& id, int role, const QVariant& value);

private:
    QSet<QString> m_expandedIds;

    int indexOfId(const QString& id) const;//按ID查找索引

    QList<PromptEntity> m_items;
};

#endif