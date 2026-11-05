import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 分组卡片
// 职责：统一的"标题 + 内容"容器，替代各页面里的 GroupBox / 裸 Pane。
// 可配置属性：title（留空则不显示标题区）、padding。
// 子元素直接写在花括号里即进入内容区。
Rectangle {
    id: root

    property string title: ""
    property int innerPadding: Theme.spaceLg

    default property alias content: body.data

    implicitWidth: layout.implicitWidth + innerPadding * 2
    implicitHeight: layout.implicitHeight + innerPadding * 2
    color: Theme.bgSurface
    radius: Theme.radiusMd
    border.width: 1
    border.color: Theme.borderDefault

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: root.innerPadding
        spacing: Theme.spaceMd

        Label {
            visible: root.title.length > 0
            text: root.title
            font.pixelSize: Theme.fontBody
            font.bold: true
            font.family: Theme.fontFamily
            color: Theme.textPrimary
            Layout.fillWidth: true
        }

        ColumnLayout {
            id: body
            Layout.fillWidth: true
            spacing: Theme.spaceSm
        }
    }
}
