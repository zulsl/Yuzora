import QtQuick
import QtQuick.Controls

import Yuzora 1.0

// 世界书条目 · 插入位置下拉
// 职责：九个插入位置只在这里定义一次，标题行（宽态）与详情（窄态）共用。
// 顺序必须与 WorldInfoEntryEntity::Position 一致（0 角色定义之前 … 8 @D[AI] 在深度），
// 否则 parsePositionIndex() 会写进错误的枚举。
ComboBox {
    id: root
    implicitHeight: Theme.controlSm

    // 外部传入：所属条目 id 与当前值（对应 Position 枚举的整数）
    property string entryId: ""
    property int entryValue: 0

    model: [
        "角色定义之前", "角色定义之后", "示例消息之前", "示例消息之后",
        "作者注释之前", "作者注释之后", "@D[系统]在深度", "@D[用户]在深度", "@D[AI]在深度"
    ]

    Binding {
        target: root
        property: "currentIndex"
        value: root.entryValue
        restoreMode: Binding.RestoreNone
    }

    onActivated: (index) => {
        if (root.entryId.length > 0)
            WorldInfoEntryViewModel.setEntryField(root.entryId, "insertPosition", index)
    }
}
