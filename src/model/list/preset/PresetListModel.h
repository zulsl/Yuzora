#ifndef PRESETLISTMODEL_H
#define PRESETLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "CompletionPresetEntity.h"

class PresetListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        IsActiveRole
    };

    explicit PresetListModel(QObject* parent = nullptr);
    ~PresetListModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void loadList(const QList<CompletionPresetEntity>& list);

    void appendItem(CompletionPresetEntity& item);
    // void removeItem(const QString& id);
    // void updateItem(const QString& id);
    // void clear();

    // //按行获取Item指针
    // PresetItemModel* getItem(int row) const;
    // //按ID查找Item
    // PresetItemModel* findById(const QString& id) const;

private:
    QList<CompletionPresetEntity> m_items;
};

#endif // PRESETLISTMODEL_H