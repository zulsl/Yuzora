#include "PresetListModel.h"

PresetListModel::PresetListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

PresetListModel::~PresetListModel()
{
    m_items.clear();
}

int PresetListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

QVariant PresetListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return {};
    }

    const CompletionPresetEntity& e = m_items.at(index.row());
    switch (role) {
        case IdRole: return e.id;
        case NameRole: return e.name;
        case IsDefaultRole: return e.isDefault;
        default: return {};
    }
}

QHash<int, QByteArray> PresetListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[IsDefaultRole] = "isDefault";
    return roles;
}

void PresetListModel::loadList(const QList<CompletionPresetEntity>& list)
{
    beginResetModel();
    // qDeleteAll(m_items); // 自动递归删除子模型
    m_items.clear();
    m_items = list;
    endResetModel();
}

void PresetListModel::appendItem(CompletionPresetEntity& item)
{
    beginInsertRows({}, m_items.size(), m_items.size());
    m_items.append(item);
    endInsertRows();
}

// void PresetListModel::removeItem(const QString& id)
// {
    // for (int i = 0; i < m_items.size(); ++i) {
    //     if (m_items.at(i).id == id) {
    //         beginRemoveRows({}, i, i);
    //         m_items.at(i).deleteLater();
    //         m_items.removeAt(i);
    //         endRemoveRows();
    //         return;
    //     }
    // }
// }

// void PresetListModel::updateItem(const QString& id)
// {
    // for (int i = 0; i < m_items.size(); ++i) {
    //     if (m_items.at(i)->id() == id) {
    //         emit dataChanged(index(i), index(i));
    //         return;
    //     }
    // }
// }

// void PresetListModel::clear()
// {
//     beginResetModel();
//     // qDeleteAll(m_items);
//     m_items.clear();
//     endResetModel();
// }

// PresetItemModel *PresetListModel::getItem(int row) const
// {
//     if (row < 0 || row >= m_items.size()) return nullptr;
//         return m_items[row];
// }

// PresetItemModel *PresetListModel::findById(const QString &id) const
// {
//     for (auto* item : m_items) {
//         if (item->id() == id) return item;
//     }
//     return nullptr;
// }