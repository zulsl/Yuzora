pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 导航面板
// 职责：头像 + 用户名 + 导航项列表，供宽态侧栏和窄态抽屉共用。
// 点击用户名弹出对话框改名；不负责响应式判断和抽屉动画，这些由 NavigationHost 处理。
// 可配置属性：items、currentIndex、showBottomSafeArea。
ColumnLayout {
    id: root

    property var items: []
    property int currentIndex: 0
    // 仅窄态抽屉需要底部安全区；宽态侧栏由页面自行处理
    property bool showBottomSafeArea: false

    signal activated(int index)
    signal avatarRequested()

    spacing: 0

    // 顶部：头像 + 用户名
    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: Theme.avatarLg + Theme.spaceXl
        Layout.topMargin: DeviceHelper.safeAreaTop
        Layout.leftMargin: Theme.spaceMd
        Layout.rightMargin: Theme.spaceMd

        RowLayout {
            anchors.fill: parent
            spacing: Theme.spaceMd

            CircleAvatar {
                size: Theme.avatarMd
                source: SettingsViewModel.userAvatar
                fallbackText: SettingsViewModel.userName.length > 0
                              ? SettingsViewModel.userName.charAt(0) : "?"


                CameraMask{
                    onMaskClicked: root.avatarRequested()
                }

            }

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: SettingsViewModel.userName
                font.pixelSize: Theme.fontBody
                font.bold: true
                font.family: Theme.fontFamily
                color: Theme.textPrimary
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: renameDialog.open()
                }
            }
        }
    }

    // 导航项（宽态侧栏和窄态抽屉共用同一份委托）
    ListView {
        id: navList
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: Theme.spaceSm
        Layout.rightMargin: Theme.spaceSm
        model: root.items
        clip: true
        spacing: Theme.spaceXs

        delegate: ItemDelegate {
            id: navDelegate
            required property int index
            required property var modelData

            width: ListView.view.width
            height: Theme.touchTarget + Theme.spaceSm
            padding: Theme.spaceMd
            highlighted: root.currentIndex === index

            // 用控件自带的 icon 渲染（IconLabel）而不是裸 Image：
            // 裸 Image 加载 SVG 时用的是文件里写死的黑色 fill，不会跟随主题；
            // icon.color 会对单色图标做着色，深色主题下才是白的。
            display: AbstractButton.TextBesideIcon
            icon.source: navDelegate.modelData.icon || ""
            icon.width: Theme.iconMd
            icon.height: Theme.iconMd
            icon.color: navDelegate.highlighted ? Theme.primaryColor : Theme.textPrimary
            text: navDelegate.modelData.title || ""
            font.pixelSize: Theme.fontBody
            font.family: Theme.fontFamily

            onClicked: root.activated(index)

            background: Rectangle {
                radius: Theme.radiusSm
                color: navDelegate.highlighted ? Theme.accentSubtle
                     : navDelegate.hovered ? Theme.rowHover
                     : "transparent"
            }
        }
    }

    // 抽屉底部安全区；宽态传 false，不占高度
    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: root.showBottomSafeArea ? DeviceHelper.safeAreaBottom : 0
        visible: root.showBottomSafeArea && DeviceHelper.safeAreaBottom > 0
    }

    TextInputDialog {
        id: renameDialog
        title: "修改用户名"
        label: "用户名"
        text: SettingsViewModel.userName
        placeholderText: "请输入用户名"
        onSubmitted: (value) => SettingsViewModel.userName = value
    }
}
