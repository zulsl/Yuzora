#ifndef COMPLETIONPRESETDRAFT_H
#define COMPLETIONPRESETDRAFT_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include "CompletionPresetEntity.h"

/**
 * @brief 预设基础参数的强类型草稿
 *
 * 只包含当前表单可编辑的字段；toEntity(base) 以已保存预设为底，
 * 避免未暴露字段（提示词顺序、扩展参数等）在保存时丢失。
 */
class CompletionPresetDraft : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool isDefault READ isDefault WRITE setIsDefault NOTIFY isDefaultChanged)
    Q_PROPERTY(int maxContextToken READ maxContextToken WRITE setMaxContextToken NOTIFY maxContextTokenChanged)
    Q_PROPERTY(int maxResponseToken READ maxResponseToken WRITE setMaxResponseToken NOTIFY maxResponseTokenChanged)
    Q_PROPERTY(bool streamEnabled READ streamEnabled WRITE setStreamEnabled NOTIFY streamEnabledChanged)
    Q_PROPERTY(double temperature READ temperature WRITE setTemperature NOTIFY temperatureChanged)
    Q_PROPERTY(double topP READ topP WRITE setTopP NOTIFY topPChanged)
    Q_PROPERTY(double topK READ topK WRITE setTopK NOTIFY topKChanged)
    Q_PROPERTY(double frequencyPenalty READ frequencyPenalty WRITE setFrequencyPenalty NOTIFY frequencyPenaltyChanged)
    Q_PROPERTY(double presencePenalty READ presencePenalty WRITE setPresencePenalty NOTIFY presencePenaltyChanged)

public:
    explicit CompletionPresetDraft(QObject* parent = nullptr);

    Q_INVOKABLE QVariant fieldValue(const QString& field) const;
    Q_INVOKABLE bool setField(const QString& field, const QVariant& value);

    void setEntity(const CompletionPresetEntity& entity);
    CompletionPresetEntity toEntity(const CompletionPresetEntity& base) const;
    bool equals(const CompletionPresetEntity& entity) const;

    QString id() const { return m_id; }
    QString name() const { return m_name; }
    bool isDefault() const { return m_isDefault; }
    int maxContextToken() const { return m_maxContextToken; }
    int maxResponseToken() const { return m_maxResponseToken; }
    bool streamEnabled() const { return m_streamEnabled; }
    double temperature() const { return m_temperature; }
    double topP() const { return m_topP; }
    double topK() const { return m_topK; }
    double frequencyPenalty() const { return m_frequencyPenalty; }
    double presencePenalty() const { return m_presencePenalty; }

public slots:
    void setId(const QString& value);
    void setName(const QString& value);
    void setIsDefault(bool value);
    void setMaxContextToken(int value);
    void setMaxResponseToken(int value);
    void setStreamEnabled(bool value);
    void setTemperature(double value);
    void setTopP(double value);
    void setTopK(double value);
    void setFrequencyPenalty(double value);
    void setPresencePenalty(double value);

signals:
    void changed();
    void idChanged();
    void nameChanged();
    void isDefaultChanged();
    void maxContextTokenChanged();
    void maxResponseTokenChanged();
    void streamEnabledChanged();
    void temperatureChanged();
    void topPChanged();
    void topKChanged();
    void frequencyPenaltyChanged();
    void presencePenaltyChanged();

private:
    void emitAllChanged();

    bool m_updating = false;
    QString m_id;
    QString m_name;
    bool m_isDefault = false;
    int m_maxContextToken = 4096;
    int m_maxResponseToken = 1024;
    bool m_streamEnabled = true;
    double m_temperature = 0.7;
    double m_topP = 0.9;
    double m_topK = 40;
    double m_frequencyPenalty = 0.0;
    double m_presencePenalty = 0.0;
};

#endif // COMPLETIONPRESETDRAFT_H
