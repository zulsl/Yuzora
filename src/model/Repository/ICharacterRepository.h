#ifndef ICHARACTERREPOSITORY_H
#define ICHARACTERREPOSITORY_H

#include <QList>
#include "../entity/CharacterEntity.h"

class ICharacterRepository {
public:
    virtual ~ICharacterRepository() = default;

    virtual QList<CharacterEntity> selectAll() = 0;
    virtual CharacterEntity selectById(const QString& id) = 0;
    virtual bool insert(const CharacterEntity& character) = 0;
    virtual bool update(const CharacterEntity& character) = 0;
    virtual bool remove(const QString& id) = 0;
    virtual bool exists(const QString& id) = 0;
};

#endif // ICHARACTERREPOSITORY_H