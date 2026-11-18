import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 世界书条目 · 触发策略下拉（收起态也带图标）
// 收起态需要图标，所以自定义 contentItem 自绘「图标 + 文字」。
// contentItem 里是裸 Image，无法像控件的 icon 那样着色，因此：
//   - 圆点图标自带颜色，直接用；
//   - 向量图标原本是纯黑，另存了一张浅色副本 iconDark，深色主题下切换过去。
// 蓝=恒定触发，绿=关键词触发，向量=向量化，
// 对应 WorldInfoEntryListModel::calcTrigger() 的 0 / 1 / 2。
//
// ⚠ 调用点注意：本组件根节点是 ComboBox，它自带 model 属性。
//   在委托里直接写 `entryId: model.id` 时，这个 model 会解析成 ComboBox 自己的
//   选项数组，取到 undefined，刷出 "Unable to assign [undefined] to QString / int"。
//   正确做法：委托根节点先存 `readonly property var row: model`，再传 row.id。
ComboBox {
    id: root
    implicitHeight: Theme.controlSm

    // 外部传入：所属条目 id 与当前值（0 始终 / 1 关键词 / 2 向量化）
    property string entryId: ""
    property int entryValue: 0

    // tint 只给菜单项用（MenuItem 是按钮，支持 icon.color 着色）；
    // iconDark 只给收起态用（自绘 Image 不能着色）。
    readonly property var options: [
        { label: "始终",   icon: "qrc:/icons/res/icons8-Blue.png",   tint: "#2563EB" },
        { label: "关键词", icon: "qrc:/icons/res/icons8-Green.png",  tint: "#22C55E" },
        { label: "向量化", icon: "qrc:/icons/res/icons8-vector.png", tint: "",
          iconDark: "qrc:/icons/res/icons8-vector-light.png" }
    ]

    // 脏数据兜底：entryValue 越界时按第一项显示，避免 undefined 下标
    readonly property int safeIndex: (entryValue >= 0 && entryValue < options.length) ? entryValue : 0
    readonly property var currentOption: options[safeIndex]

    // 必须写成属性而不是函数：函数调用不会被 QML 追踪依赖，
    // 主题切换时图标不会跟着换。
    readonly property string currentIcon: (Theme.isDark && currentOption.iconDark)
                                         ? currentOption.iconDark : currentOption.icon

    model: options
    textRole: "label"

    // 补回被 contentItem 换掉的左侧内边距。
    // Material 默认的 contentItem 本身带 leftPadding=16，换成 RowLayout 后
    // 这 16px 就没了，图标和文字会直接贴到控件左边缘（看起来"挤在左边"）。
    // 右侧的 padding 由 ComboBox 自己留给下拉箭头（rightPadding=40），不要动。
    leftPadding: Theme.spaceLg

    // 用 Binding 而不是 `currentIndex: entryValue`：
    // 用户点选时 ComboBox 会自己改 currentIndex，普通声明式绑定会被这次内部赋值销毁，
    // 之后委托被复用到另一条条目就再也同步不上。
    Binding {
        target: root
        property: "currentIndex"
        value: root.entryValue
        restoreMode: Binding.RestoreNone
    }

    onActivated: (index) => {
        if (root.entryId.length > 0)
            WorldInfoEntryViewModel.setEntryField(root.entryId, "triggerControl", index)
    }

    contentItem: RowLayout {
        spacing: Theme.spaceMd

        Image {
            Layout.alignment: Qt.AlignVCenter
            source: root.currentIcon
            sourceSize: Qt.size(Theme.iconMd, Theme.iconMd)
            fillMode: Image.PreserveAspectFit
        }

        Label {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            text: root.currentText
            elide: Text.ElideRight
            font.pixelSize: Theme.fontBody
            font.family: Theme.fontFamily
            color: Theme.textPrimary
        }
    }

    delegate: ItemDelegate {
        required property var modelData

        // 不写 root.width / root.highlightedIndex：委托是嵌套组件，
        // 引用外层 id 会触发 qmllint 的 unqualified 告警。
        width: ListView.view ? ListView.view.width : 0
        highlighted: ListView.isCurrentItem
        text: modelData.label
        icon.source: modelData.icon
        icon.color: modelData.tint.length > 0 ? modelData.tint : Theme.textPrimary
        icon.width: Theme.iconMd
        icon.height: Theme.iconMd
    }
}

