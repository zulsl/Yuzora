import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 页面顶栏
// 职责：左按钮 + 居中标题 + 右侧动作槽，并自动吸收系统安全区上边距。
// 可配置属性：title、leftIconSource、showLeftButton、rightItems（右侧动作列表）。
// 替换：MyToolBar.qml，以及 ChatPage / ApiConfigPage 里各自手写的 ToolBar。
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

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spaceSm
        anchors.rightMargin: Theme.spaceSm
        anchors.topMargin: root.safeTop
        spacing: Theme.spaceSm

        IconButton {// 菜单按钮
            visible: root.showLeftButton
            icon.source: root.leftIconSource
            onClicked: root.leftClicked()
        }

        Column {
            Layout.fillWidth: true
            spacing: 0

            Label {// 标题
                width: parent.width
                text: root.title
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
                font.pixelSize: Theme.fontCaption
                font.family: Theme.fontFamily
                color: Theme.textSecondary
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }
        }

        RowLayout {
            id: rightRow
            spacing: Theme.spaceXs
        }
    }
}
