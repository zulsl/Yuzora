#ifndef CHARACTERITEMVIEWMODEL_H
#define CHARACTERITEMVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include "CharacterEntity.h"

/**
 * @brief 角色列表项视图模型
 * 职责：列表展示数据适配、视图层格式化（路径拼接、时间友好化）
 * 仅承载列表渲染所需字段，不包含完整角色详情，无业务逻辑
 */
class CharacterItemViewModel : public QObject
{
    Q_OBJECT
    //(可优化)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString name READ name NOTIFY dataChanged)
    Q_PROPERTY(QString description READ description NOTIFY dataChanged)
    Q_PROPERTY(QString avatarPath READ avatarPath NOTIFY dataChanged)
    Q_PROPERTY(bool isFavorite READ isFavorite NOTIFY dataChanged)
    Q_PROPERTY(QString updateTimeText READ updateTimeText NOTIFY dataChanged)

public:
    explicit CharacterItemViewModel(QObject* parent = nullptr);
    ~CharacterItemViewModel() override;

    // ========== 属性读取接口 ==========
    QString id() const;
    QString name() const;
    QString description() const;
    QString avatarPath() const;
    bool isFavorite() const;
    QString updateTimeText() const;

    // ========== 数据更新接口 ==========
    void updateFromEntity(const CharacterEntity& entity);

signals:
    void dataChanged();

private:
    // 视图格式化辅助：时间转友好显示文本
    QString formatFriendlyTime(const QDateTime& time) const;

    // ========== 原始数据成员 ==========
    QString m_id;
    QString m_name;
    QString m_description;
    QString m_avatarRelativePath;
    bool m_isFavorite = false;
    QDateTime m_updatedAt;
};

#endif // CHARACTERITEMVIEWMODEL_H