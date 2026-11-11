import QtQuick
import QtQuick.Controls

// 确认对话框
// 职责：统一"危险操作二次确认"的外观与按钮文案，替代 4 处各自声明的 MessageDialog。
// 可配置属性：title、text、detail、acceptText、rejectText、danger。
// 用法：dialog.open()，用户确认后发出 confirmed()。
Dialog {
    id: root

    property string text: ""
    property string detail: ""
    property string acceptText: "确定"
    property string rejectText: "取消"
    property bool danger: false

    signal confirmed()
    signal declined()

    title: ""
    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(parent ? parent.width - Theme.spaceXl * 2 : 380, 420)
    padding: Theme.spaceLg
    closePolicy: Popup.CloseOnEscape

    background: Rectangle {
        color: Theme.bgElevated
        radius: Theme.radiusMd
        border.width: 1
        border.color: Theme.borderDefault
    }

    header: Label {
        text: root.title
        visible: root.title.length > 0
        font.pixelSize: Theme.fontTitle
        font.bold: true
        font.family: Theme.fontFamily
        color: Theme.textPrimary
        topPadding: Theme.spaceLg
        leftPadding: Theme.spaceLg
        rightPadding: Theme.spaceLg
    }

    contentItem: Column {
        spacing: Theme.spaceSm
        Label {
            width: parent.width
            text: root.text
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.fontBody
            font.family: Theme.fontFamily
            color: Theme.textPrimary
        }
        Label {
            width: parent.width
            visible: root.detail.length > 0
            text: root.detail
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.fontBodySm
            font.family: Theme.fontFamily
            color: Theme.textSecondary
        }
    }

    footer: Item {
        implicitHeight: Theme.touchTarget + Theme.spaceSm
        Row {
            anchors.right: parent.right
            anchors.rightMargin: Theme.spaceLg
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.spaceSm

            AppButton {
                text: root.rejectText
                variant: "ghost"
                onClicked: {
                    root.close()
                    root.declined()
                }
            }
            AppButton {
                text: root.acceptText
                variant: root.danger ? "danger" : "primary"
                onClicked: {
                    root.close()
                    root.confirmed()
                }
            }
        }
    }
}
