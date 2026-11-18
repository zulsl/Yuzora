import QtQuick
import QtQuick.Controls

// 空态占位
// 职责：列表/表单无数据时的统一提示，替代各页面各自 anchors.centerIn + Label。
// 可配置属性：text、detail、iconSource。
Item {
    id: root

    property string text: ""
    property string detail: ""
    property string iconSource: ""

    Column {
        anchors.centerIn: parent
        spacing: Theme.spaceSm

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.iconSource.length > 0
            width: Theme.iconLg * 2
            height: Theme.iconLg * 2
            source: root.iconSource
            sourceSize: Qt.size(Theme.iconLg * 2, Theme.iconLg * 2)
            fillMode: Image.PreserveAspectFit
            opacity: 0.35
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.text
            font.pixelSize: Theme.fontBody
            font.family: Theme.fontFamily
            color: Theme.textSecondary
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.detail.length > 0
            text: root.detail
            font.pixelSize: Theme.fontCaption
            font.family: Theme.fontFamily
            color: Theme.textDisabled
        }
    }
}
