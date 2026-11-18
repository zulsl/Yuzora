import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 文本输入对话框
// 职责：统一的“单行文本 + 保存/取消”弹窗，供改名等场景复用。
// 可配置属性：title、label、text、placeholderText、acceptText、rejectText。
// 确认后发出 submitted(string value)；空文本不会提交。
Dialog {
    id: root

    property string label: ""
    property string text: ""
    property string placeholderText: ""
    property string acceptText: "保存"
    property string rejectText: "取消"

    signal submitted(string value)

    title: ""
    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(Overlay.overlay ? Overlay.overlay.width - Theme.spaceXl * 2 : 380, 420)
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

    contentItem: ColumnLayout {
        spacing: Theme.spaceSm

        Label {
            Layout.fillWidth: true
            visible: root.label.length > 0
            text: root.label
            font.pixelSize: Theme.fontBodySm
            font.family: Theme.fontFamily
            color: Theme.textSecondary
        }

        TextField {
            id: input
            Layout.fillWidth: true
            placeholderText: root.placeholderText
            selectByMouse: true
            onAccepted: root.submit()

            TextContextMenu {}
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
                onClicked: root.close()
            }
            AppButton {
                text: root.acceptText
                variant: "primary"
                enabled: input.text.trim().length > 0
                onClicked: root.submit()
            }
        }
    }

    onOpened: {
        input.text = root.text
        input.forceActiveFocus()
        input.selectAll()
    }

    function submit() {
        const value = input.text.trim()
        if (value.length === 0) return
        close()
        root.submitted(value)
    }
}
