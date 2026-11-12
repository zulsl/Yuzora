import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 世界书条目 · 次关键字逻辑
// 一个下拉同时管两件事：
//   selective=false            → 完全忽略次关键字（对应"不启用"这一项）
//   selective=true + logic 0~3 → 次关键字如何与主关键字结果组合
// 为什么不拆成"开关 + 下拉"：单个控件就能表达全部状态，
// 而且能直接暴露出导入数据里 selective=false 的条目（只给 4 项的话，
// 那种条目会显示"任一命中"却完全不生效，属于静默失效）。
ColumnLayout {
    id: root

    property string entryId: ""
    property bool selective: true
    property int logic: 0

    // 下拉的 5 个序号：0 不启用，1~4 对应 SelectiveLogic 的 0~3
    readonly property int safeIndex: selective
                                      ? Math.min(3, Math.max(0, logic)) + 1 : 0

    spacing: Theme.spaceXs

    Label {
        Layout.fillWidth: true
        text: "次关键字逻辑"
        font.pixelSize: Theme.fontBodySm
        font.family: Theme.fontFamily
        color: Theme.textSecondary
    }

    ComboBox {
        id: combo
        implicitHeight: Theme.controlSm
        Layout.fillWidth: true

        model: [
            "不启用（忽略次关键字）",
            "任一命中（AND_ANY）",
            "全部命中（AND_ALL）",
            "任一不命中（NOT_ANY）",
            "全部不命中（NOT_ALL）"
        ]

        // 用 Binding 而不是 `currentIndex: safeIndex`：用户点选时
        // ComboBox 会自己改 currentIndex，普通绑定会被这次内部赋值销毁。
        Binding {
            target: combo
            property: "currentIndex"
            value: root.safeIndex
            restoreMode: Binding.RestoreNone
        }

        onActivated: (index) => {
            if (root.entryId.length === 0)
                return

            if (index === 0) {
                WorldInfoEntryViewModel.setEntryField(root.entryId, "selective", false)
                return
            }

            WorldInfoEntryViewModel.setEntryField(root.entryId, "selective", true)
            WorldInfoEntryViewModel.setEntryField(root.entryId, "selectiveLogic", index - 1)
        }
    }
}
