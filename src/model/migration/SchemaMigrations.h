#ifndef SCHEMAMIGRATIONS_H
#define SCHEMAMIGRATIONS_H

#include "SchemaMigration.h"

/**
 * @brief 数据库结构迁移清单
 *
 * V1 是第一个正式版本，直接创建完整结构；以后所有结构变化都追加新版本，
 * 不要修改已经发布的基线/迁移。新库按 1..N 顺序执行，旧库补跑缺失版本。
 */
class SchemaMigrations
{
public:
    static QVector<SchemaMigration> all();
    static int latestVersion();

    // 只用 CREATE TABLE IF NOT EXISTS 建表/索引，可重复执行。
    static bool applyBaseline(QSqlDatabase& db, QString& error);
};

#endif // SCHEMAMIGRATIONS_H
