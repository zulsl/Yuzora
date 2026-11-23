#ifndef PRESETPROMPTLISTMODEL_H
#define PRESETPROMPTLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QStringList>
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

    // 拖动换位置：只在内存里挪行（带 beginMoveRows/endMoveRows），落库由 ViewModel 负责。
    // from/to 都是当前行号；to 越界时返回 false。不传 QML，只给 C++ 调用。
    bool moveRow(int from, int to);
    // 当前列表顺序（identifier 数组），提交时按它重写 promptOrder
    QStringList identifiers() const;
    // 起拖时把所有展开的行收起来：收起后每行等高，让位动画和落点计算才稳定。
    Q_INVOKABLE void collapseAll();

private:
    QSet<QString> m_expandedIds;

    int indexOfId(const QString& id) const;//按ID查找索引

    QList<PromptEntity> m_items;
};

#endif