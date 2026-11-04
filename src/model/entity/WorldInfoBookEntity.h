#ifndef WORLDINFOBOOKENTITY_H
#define WORLDINFOBOOKENTITY_H

#include <QString>
#include <QDateTime>

//世界书数据实体

struct WorldInfoBookEntity {
    QString id;
    QString name;
    bool isEnabled = false;
    QDateTime createdAt;
    QDateTime updatedAt;
};

#endif