#ifndef CHATSESSIONLISTMODEL_H
#define CHATSESSIONLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "ChatSessionEntity.h"

class ChatSessionListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum ConfigRoles {
        IdRole = Qt::UserRole + 1,
        CharacterIdRole,
        TitleRole
    };
    explicit ChatSessionListModel(QObject *parent = nullptr);

    // ========== 基类重写 ==========
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    //载入list数据
    void loadList(const QList<ChatSessionEntity>& list);

//CURD
    void addItem(const ChatSessionEntity& item);
    void updateItem(const ChatSessionEntity& item);
    void removeItem(const QString& id);
    void clear();
private:
    int indexOfId(const QString& id) const;
    QList<ChatSessionEntity> m_items;
};

#endif // CHATSESSIONLISTMODEL_H
