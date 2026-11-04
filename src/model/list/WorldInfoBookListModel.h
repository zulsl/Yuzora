#ifndef WORLDINFOBOOKLISTMODEL_H
#define WORLDINFOBOOKLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "WorldInfoBookItemModel.h"

class WorldInfoBookListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        DescRole,
        EnabledRole,
        GlobalRole
    };

    explicit WorldInfoBookListModel(QObject* parent = nullptr);
    ~WorldInfoBookListModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void resetData(const QList<WorldInfoBookItemModel*>& list);
    void clear();
    WorldInfoBookItemModel* getItemById(const QString& id) const;
private:
    QList<WorldInfoBookItemModel*> m_items;
};

#endif