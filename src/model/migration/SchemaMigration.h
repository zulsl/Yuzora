#ifndef SCHEMAMIGRATION_H
#define SCHEMAMIGRATION_H

#include <QSqlDatabase>
#include <QString>
#include <QVector>
#include <functional>

// 一次结构迁移。version 从 1 开始连续递增，name 发布后不可再改。
struct SchemaMigration {
    int version = 0;
    QString name;
    std::function<bool(QSqlDatabase&, QString&)> apply;
};

#endif // SCHEMAMIGRATION_H
