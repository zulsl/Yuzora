import QtQuick
import QtQuick.Controls

import Yuzora 1.0

// 世界书条目 · 数字字段（插入深度 / 插入顺序 / 扫描深度共用）
// 职责：把"SpinBox + 写回 ViewModel"收成一处，避免每个数字字段各写一遍 onValueModified。
// field 是 WorldInfoEntryViewModel.setEntryField 认识的字段名。
SpinBox {
    id: root
    implicitHeight: Theme.controlSm

    // 外部传入：所属条目 id、字段名、当前值
    property string entryId: ""
    property string field: ""
    property int entryValue: 0

    editable: true
    from: 0
    to: 1000000

    // 同 TriggerStrategyComboBox：SpinBox 编辑时也会自己改 value，
    // 普通绑定会被销毁，委托复用后就不再跟随数据。
    Binding {
        target: root
        property: "value"
        value: root.entryValue
        restoreMode: Binding.RestoreNone
    }

    onValueModified: {
        if (root.entryId.length > 0 && root.field.length > 0)
            WorldInfoEntryViewModel.setEntryField(root.entryId, root.field, value)
    }
}
