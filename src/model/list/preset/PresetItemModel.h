// #ifndef PRESETITEMMODEL_H
// #define PRESETITEMMODEL_H

// #include <QObject>
// #include "CompletionPresetEntity.h"
// #include "PresetPromptListModel.h"

// class PresetItemModel : public QObject
// {
//     Q_OBJECT
//     Q_PROPERTY(QString id READ id CONSTANT)
//     Q_PROPERTY(QString name READ name CONSTANT)
//     Q_PROPERTY(int modelType READ modelType CONSTANT)
//     Q_PROPERTY(QString modelName READ modelName CONSTANT)
//     Q_PROPERTY(bool isDefault READ isDefault CONSTANT)

// public:
//     explicit PresetItemModel(const CompletionPresetEntity& entity, QObject* parent = nullptr);

//     QString id() const;
//     QString name() const;
//     int modelType() const;
//     QString modelName() const;
//     bool isDefault() const;

// private:
//     QString m_id;
//     QString m_name;
//     bool m_isDefault;

//     int m_modelType;
//     QString m_modelName;


//     int m_maxContextToken;
//     int m_maxResponseToken;
//     bool m_streamEnabled;

//     double temperature;
//     double topP;
//     double topK;
//     double frequencyPenalty;
//     double presencePenalty;
// };

// #endif