#ifndef WORLDINFOENTRYLISTMODEL_H
#define WORLDINFOENTRYLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "WorldInfoEntryEntity.h"

class WorldInfoEntryListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        EntryIdRole = Qt::UserRole + 1,
        BookIdRole,//书id
        NameRole,//名称
        ContentRole,//内容
        KeywordsRole,//主要关键字
        SecondaryKeywordsRole,//次要关键字
        TriggerControlRole,//触发策略
        ConstantRole,//永久
        VectorizedRole,//向量化
        InsertPositionRole,//插入位置
        InsertRoleRole,//插入角色
        InsertDepthRole,//深度
        ScanDepthRole,//扫描深度
        SortOrderRole,//排序顺序
        UseProbabilityRole,//启用概率触发
        ProbabilityRole,//触发概率(%)
        SelectiveRole,//选择性(默认开启)
        SelectiveLogicRole,//选择性逻辑(0:与任意)
        IsEnabledRole,//启用
        ExpandedStateRole// 展开状态
    };

    explicit WorldInfoEntryListModel(QObject* parent = nullptr);
    ~WorldInfoEntryListModel() override;

    // ---- QAbstractListModel 接口 ----
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;//这是 QML 专用接口
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;


    // 数据源操作
    void resetData(const QList<WorldInfoEntryEntity>& entries);
    void clear();
    int rowById(const QString& id) const;
    WorldInfoEntryEntity entryAt(int row) const;
    WorldInfoEntryEntity entryById(const QString& id) const;
    // ViewModel 专用：更新业务字段，不触发持久化
    bool updateField(const QString& id, int role, const QVariant& value);

    // ---- 增量更新（关键：不重建整个列表） ----
    void updateEntry(const WorldInfoEntryEntity& entry);     // 替换某一行并 dataChanged
    void removeEntry(const QString& id);
    void appendEntry(const WorldInfoEntryEntity& entry);

    // 展开状态
    Q_INVOKABLE void toggleExpanded(const QString& id);
    Q_INVOKABLE void setExpanded(const QString& id, bool expanded);
    Q_INVOKABLE void collapseAll();

private:
    QList<WorldInfoEntryEntity> m_entries;
    QSet<QString> m_expandedIds;

    int rowByIdInternal(const QString& id) const;
    static QString buildKeywordPreview(const WorldInfoEntryEntity& entry);
    int calcPositionIndex(const WorldInfoEntryEntity& en)const;
    void parsePositionIndex(WorldInfoEntryEntity& en,int index);
    int calcTrigger(const WorldInfoEntryEntity& en)const;
    void parseTrigger(WorldInfoEntryEntity& en,int index);
};

#endif