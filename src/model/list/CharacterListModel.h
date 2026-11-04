#ifndef CHARACTERLISTMODEL_H
#define CHARACTERLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "CharacterItemViewModel.h"

/**
 * @brief 角色列表数据模型
 * 为 QML ListView 提供标准化数据供给，封装角色项的增删改查与刷新
 */
class CharacterListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        DescriptionRole,
        AvatarPathRole,
        IsFavoriteRole,
        LastUsedTextRole,
        UseCountRole,
        UpdateTimeRole
    };

    explicit CharacterListModel(QObject* parent = nullptr);
    ~CharacterListModel() override;

    // ========== 基类重写 ==========
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ========== 列表增量操作 ==========
    void appendItem(CharacterItemViewModel* item);
    void insertItem(int row, CharacterItemViewModel* item);
    void updateItem(const QString& charId);
    void removeItem(const QString& charId);
    void clear();

    //载入list数据
    void loadList(const QList<CharacterItemViewModel*>& list);

    // ========== 辅助查询 ==========
    CharacterItemViewModel* getItemById(const QString& charId) const;
    CharacterItemViewModel* lastItem() const;

private:
    int indexOfId(const QString& charId) const;

    QList<CharacterItemViewModel*> m_items;
};

#endif // CHARACTERLISTMODEL_H