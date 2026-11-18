#ifndef MIGRATIONRUNNER_H
#define MIGRATIONRUNNER_H

#include "SchemaMigration.h"

/**
 * @brief 顺序执行 SchemaMigration
 *
 * - schema_migrations 记录已执行版本；
 * - 第一次启动从 V1 开始建库；
 * - 已有表但没有迁移记录时，补建缺失表并记录 V1；
 * - 数据库版本高于程序支持版本时拒绝启动；
 * - 已有业务表时迁移前自动备份。
 */
class MigrationRunner
{
public:
    static bool run(QSqlDatabase& db,
                    const QString& dbFilePath,
                    const QVector<SchemaMigration>& migrations,
                    QString* error = nullptr);
};

#endif // MIGRATIONRUNNER_H
