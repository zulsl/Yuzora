#ifndef WorldInfoBookItemModel_H
#define WorldInfoBookItemModel_H

#include <QObject>
#include "WorldInfoBookEntity.h"

class WorldInfoBookItemModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
    Q_PROPERTY(bool isEnabled READ isEnabled CONSTANT)
    Q_PROPERTY(bool isGlobal READ isGlobal CONSTANT)
public:
    explicit WorldInfoBookItemModel(const WorldInfoBookEntity& entity, QObject* parent = nullptr);
    QString id() const;
    QString name() const;
    QString description() const;
    bool isEnabled() const;
    bool isGlobal() const;
private:
    QString m_id;
    QString m_name;
    QString m_description;
    bool m_isEnabled;
    bool m_isGlobal;
};
#endif