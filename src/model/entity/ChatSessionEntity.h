#ifndef CHATSESSIONENTITY_H
#define CHATSESSIONENTITY_H

#include <QString>
#include <QDateTime>
#include <QVariantMap>

//聊天会话数据实体

struct ChatSessionEntity {
    QString id;
    QString characterId;    //所属角色id
    QString title;          //标题
    QDateTime createdAt;    //创建时间
    QDateTime lastModified; //最后修改时间
};

#endif