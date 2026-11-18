import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 带标签的字段行
// 职责：把"标签 + 控件"收成一处，并按断点自动在横排/竖排之间切换。
// 用法：LabeledField { label: "主题模式"; ComboBox { Layout.fillWidth: true } }
// 说明：本组件本身就是 GridLayout，子元素直接成为网格单元 ——
//       宽窗口是 [标签 | 控件] 两列，窄窗口自动变成上下两行。
GridLayout {
    id: root

    property string label: ""
    property int labelWidth: 96
    readonly property bool stacked: Theme.isCompact

    columns: stacked ? 1 : 2
    columnSpacing: Theme.spaceMd
    rowSpacing: Theme.spaceXs
    Layout.fillWidth: true

    Label {
        text: root.label
        font.pixelSize: Theme.fontBody
        font.family: Theme.fontFamily
        color: Theme.textSecondary
        wrapMode: Text.WordWrap
        Layout.preferredWidth: root.stacked ? -1 : root.labelWidth
        Layout.fillWidth: root.stacked
        Layout.alignment: root.stacked ? Qt.AlignLeft : (Qt.AlignLeft | Qt.AlignVCenter)
    }
}
