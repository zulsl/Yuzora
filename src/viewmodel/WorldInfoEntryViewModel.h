#ifndef WORLDINFOENTRYVIEWMODEL_H
#define WORLDINFOENTRYVIEWMODEL_H

#include <QObject>
#include <QVariantMap>
#include <QStringList>
#include <QHash>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

#include "WorldInfoEntryListModel.h"
#include "WorldInfoEntryEntity.h"

class QJSEngine;
class QTimer;
class WorldInfoService;

/**
 * @brief 世界书条目视图模型
 *
 * 职责（与原 WorldInfoViewModel 拆分后的一半）：
 *   - 条目列表数据源（entryModel）与当前编辑快照（currentEntry）；
 *   - 条目级增删改、开关、移动、批量操作；
 *   - 条目实体 ↔ QVariantMap 的互转。
 *
 * 与文集域的边界：本类不关心"有哪些文集"，只关心"当前文集下的条目"。
 * 文集切换由 WorldInfoViewModel::selectBook() 通过 setCurrentBookId() 通知本类。
 *
 * 对外暴露为 QML 单例，类型名 WorldInfoEntryViewModel。
 */
class WorldInfoEntryViewModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // C++ 侧获取同一实例（QML 单例也用这个实例，避免出现两份状态）
    static WorldInfoEntryViewModel* instance();
    // 供 QML 使用：QML 单例工厂
    static WorldInfoEntryViewModel* create(QQmlEngine*, QJSEngine*)
    {
        WorldInfoEntryViewModel* inst = instance();
        // 生命周期归 C++，避免引擎析构时删除静态单例导致退出卡死
        QQmlEngine::setObjectOwnership(inst, QQmlEngine::CppOwnership);
        return inst;
    }

    ~WorldInfoEntryViewModel() override;

    // 条目列表数据源（供 QML 使用）
    Q_PROPERTY(WorldInfoEntryListModel* entryModel READ entryModel CONSTANT)

    WorldInfoEntryListModel* entryModel() const;

    // ---- 供 C++（WorldInfoViewModel）使用，不是 QML 接口 ----
    void setCurrentBookId(const QString& bookId);
    QString currentBookId() const { return m_currentBookId; }

    // ---- 供 QML 使用 ----
    Q_INVOKABLE void refreshEntryList();
    Q_INVOKABLE void createEntry();
    Q_INVOKABLE void setEntryField(const QString& entryId, const QString& field,
                                   const QVariant& value);
    Q_INVOKABLE void flushPendingEntryEdits();
    Q_INVOKABLE void deleteEntry(const QString& entryId);
    Q_INVOKABLE void toggleEntryEnabled(const QString& entryId);
    Q_INVOKABLE void moveEntry(const QString& entryId, const QString& targetBookId);
    Q_INVOKABLE void batchDelete(const QStringList& entryIds);
    Q_INVOKABLE void batchSetEnabled(const QStringList& entryIds, bool enabled);
    // 展开/收起是"UI 状态"，不走 setEntryField 的落库路径，只写列表模型
    // （模型用 ExpandedStateRole 单独保存，见 WorldInfoEntryListModel::setData）。
    // QML 侧的 Repeater 委托拿不到 model 对象，写不回 role，所以单独开这个入口。
    Q_INVOKABLE void setEntryExpanded(const QString& entryId, bool expanded);

signals:
    // 供 QML 使用
    void operationError(const QString& errorMsg);

private:
    // 构造函数必须 private：QML 单例只有在"无法默认构造"时才会调用 create()。
    // 若这里保持 public，引擎会自己 new 一个实例，导致 C++ 侧 instance() 与
    // QML 侧拿到的不是同一个对象（表现为"选中世界书后条目列表始终为空"）。
    explicit WorldInfoEntryViewModel(QObject* parent = nullptr);

    int roleForField(const QString& field) const;

    WorldInfoService* m_service = nullptr;
    WorldInfoEntryListModel* m_entryModel = nullptr;
    QString m_currentBookId;
    QTimer* m_saveTimer = nullptr;
    QHash<QString, QVariantMap> m_pendingFields;
};

#endif // WORLDINFOENTRYVIEWMODEL_H
