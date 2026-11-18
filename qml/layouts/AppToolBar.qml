import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 页面顶栏
// 职责：左按钮 + 居中标题 + 右侧动作槽，并自动吸收系统安全区上边距。
// 可配置属性：title、leftIconSource、showLeftButton、rightItems（右侧动作列表）。
// 替换：MyToolBar.qml，以及 ChatPage / ApiConfigPage 里各自手写的 ToolBar。
//
// 标题为什么不用 RowLayout 居中：RowLayout 里标题只占"左按钮之后、右侧动作之前"
// 的那一段，右侧动作比左侧多时标题会被推偏（实测偏左 54px，和页面中线对不上）。
// 这里改成 anchoring：标题钉在整条工具栏的中线上，两侧按"较宽的一边"对称留白。
ToolBar {
    id: root

    property string title: ""
    property string subtitle: ""
    property string leftIconSource: "qrc:/icons/res/menu.svg"
    property bool showLeftButton: true
    // 右侧动作：AppToolBar { rightItems: [ IconButton {...}, AppButton {...} ] }
    property alias rightItems: rightRow.data

    signal leftClicked()

    // 安全区（状态栏 / 刘海）由这里吸收，页面内容区不用再单独处理
    readonly property int safeTop: DeviceHelper.safeAreaTop

    implicitHeight: Theme.controlLg + Theme.spaceSm + safeTop
    padding: 0

    background: Rectangle {
        color: Theme.bgSurface
        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: 1
            color: Theme.borderDefault
        }
    }

    Item {
        id: inner
        anchors.fill: parent
        anchors.leftMargin: Theme.spaceSm
        anchors.rightMargin: Theme.spaceSm
        anchors.topMargin: root.safeTop

        IconButton {// 菜单按钮
            id: leftButton
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            visible: root.showLeftButton
            icon.source: root.leftIconSource
            onClicked: root.leftClicked()
        }

        RowLayout {
            id: rightRow
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.spaceXs
        }

        Column {
            id: titleColumn
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0

            // 两侧动作槽取较宽的一边（左按钮在宽窗口下是隐藏的，宽度按 0 算），
            // 这样标题的中点落在工具栏中线上，不管右边挂了几个按钮
            readonly property int sideSlot: Math.max(
                leftButton.visible ? leftButton.implicitWidth : 0,
                rightRow.implicitWidth) + Theme.spaceSm

            // 可用宽度扣掉两侧，长标题就地省略，不会压到按钮上
            width: Math.max(0, parent.width - 2 * sideSlot)

            Label {// 标题
                width: parent.width
                text: root.title
                textFormat: Text.PlainText
                font.pixelSize: Theme.fontTitle
                font.bold: true
                font.family: Theme.fontFamily
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }

            Label {// 次要标题
                width: parent.width
                visible: root.subtitle.length > 0
                text: root.subtitle
                textFormat: Text.PlainText
                font.pixelSize: Theme.fontCaption
                font.family: Theme.fontFamily
                color: Theme.textSecondary
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }
        }
    }
}
