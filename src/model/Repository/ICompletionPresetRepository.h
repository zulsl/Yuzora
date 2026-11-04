#ifndef ICOMPLETIONPRESETREPOSITORY_H
#define ICOMPLETIONPRESETREPOSITORY_H

#include <QList>
#include "../entity/CompletionPresetEntity.h"

class ICompletionPresetRepository {
public:
    virtual ~ICompletionPresetRepository() = default;

    virtual QList<CompletionPresetEntity> selectAll() = 0;
    virtual CompletionPresetEntity selectById(const QString& id) = 0;
    virtual CompletionPresetEntity selectDefault() = 0;
    virtual QString insert(const CompletionPresetEntity& preset) = 0;
    virtual bool update(const CompletionPresetEntity& preset) = 0;
    virtual bool remove(const QString& id) = 0;
    virtual bool setDefault(const QString& id) = 0;
};

#endif // ICOMPLETIONPRESETREPOSITORY_H