#ifndef APICONFIGLISTMODEL_H
#define APICONFIGLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "ApiConfigEntity.h"

//api配置列表

class ApiConfigListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum ConfigRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        ModelTypeRole,              //大模型类型
        BaseUrlRole,                //基础URL
        IsOpenAICompatibleRole,     //是否openai标准
        IsActiveRole                //是否激活
    };
    Q_ENUM(ConfigRoles)

    explicit ApiConfigListModel(QObject* parent = nullptr);

    //总行数
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    //根据 index 和 role 返回对应数据
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    //定义字段名称映射（给QML用，映射role→字段名）
    QHash<int, QByteArray> roleNames() const override;

    //载入list数据
    void loadList(const QList<ApiConfigEntity>& list);

//CURD
    void addItem(const ApiConfigEntity& item);
    void updateItem(const ApiConfigEntity& item);
    void removeItem(const QString& id);
    void clear();

private:
    int indexOfId(const QString& id) const;
    QList<ApiConfigEntity> m_configs;
};

#endif // APICONFIGLISTMODEL_H