#ifndef CHARACTERCARDPARSER_H
#define CHARACTERCARDPARSER_H

#include <QObject>
#include <QJsonObject>
#include "CharacterEntity.h"

//角色卡解析器

class CharacterCardParser : public QObject {
    Q_OBJECT
public:
    static CharacterCardParser& instance();
    ~CharacterCardParser() override = default;

    CharacterEntity parseFromPng(const QString& filePath);
    bool exportToPng(const CharacterEntity& character, const QString& savePath);
    CharacterEntity parseFromJson(const QJsonObject& json);
    QJsonObject toJson(const CharacterEntity& character);

private:
    CharacterCardParser() = default;
    CharacterEntity parseV2Data(const QJsonObject& data);
    CharacterEntity parseV1Data(const QJsonObject& data);
    QJsonObject toV2Data(const CharacterEntity& character);
};

#endif // CHARACTERCARDPARSER_H