import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import Yuzora 1.0

// 导航宿主
// 职责：按断点切换导航形态 —— 宽窗口常驻侧栏、窄窗口抽屉。
// 移动端专项：抽屉在触屏设备上关闭拖拽（interactive: false），
//       避免与系统边缘返回手势抢边缘，只能由按钮打开。
Item {
    id: root

    property alias content: contentHost.data
    property Item contentArea: contentHost
    property var items: []
    property int currentIndex: 0

    signal activated(int index)

    readonly property bool wide: width >= Theme.bpExpanded
    readonly property bool drawerOpened: drawer.opened
    readonly property int railWidth: Theme.controlLg * 2 + Theme.spaceXl
    readonly property int drawerWidth: Math.min(320, Math.round(root.width * 0.84))

    function openDrawer() { if (!wide) drawer.open() }
    function closeDrawer() { drawer.close() }

    // ---- 导航项（侧栏与抽屉共用，保证两种形态外观一致）----
    Component {
        id: navItemComponent

        ItemDelegate {
            id: navDelegate
            required property int index
            required property var modelData

            width: ListView.view.width
            height: Theme.touchTarget + Theme.spaceSm
            padding: Theme.spaceMd
            highlighted: root.currentIndex === index

            onClicked: {
                root.activated(index)
                drawer.close()
            }

            contentItem: RowLayout {
                spacing: Theme.spaceMd

                Image {
                    width: Theme.iconMd
                    height: Theme.iconMd
                    source: navDelegate.modelData.icon || ""
                    sourceSize: Qt.size(Theme.iconMd, Theme.iconMd)
                    fillMode: Image.PreserveAspectFit
                }

                Label {
                    Layout.fillWidth: true
                    text: navDelegate.modelData.title || ""
                    font.pixelSize: Theme.fontBody
                    font.family: Theme.fontFamily
                    color: navDelegate.highlighted ? Theme.primaryColor : Theme.textPrimary
                    elide: Text.ElideRight
                }
            }

            background: Rectangle {
                radius: Theme.radiusSm
                color: navDelegate.highlighted ? Theme.accentSubtle
                     : navDelegate.hovered ? Theme.rowHover
                     : "transparent"
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ---- 展开态：常驻侧栏 ----
        Rectangle {
            visible: root.wide
            Layout.fillHeight: true
            Layout.preferredWidth: root.wide ? root.railWidth : 0
            color: Theme.bgSurface

            Rectangle {
                anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
                width: 1
                color: Theme.borderDefault
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Theme.avatarLg + Theme.spaceXl
                    Layout.topMargin: DeviceHelper.safeAreaTop

                    CircleAvatar {
                        anchors.centerIn: parent
                        size: Theme.avatarMd
                        source: SettingsViewModel.userAvatar
                        fallbackText: SettingsViewModel.userName.length > 0
                                      ? SettingsViewModel.userName.charAt(0) : "?"

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: avatarDialog.open()
                        }
                    }
                }

                ListView {
                    id: railList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: root.items
                    delegate: navItemComponent
                    clip: true
                    spacing: Theme.spaceXs
                }
            }
        }

        // ---- 页面内容宿主 ----
        Item {
            id: contentHost
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    // ---- 窄态：抽屉 ----
    Drawer {
        id: drawer
        width: root.drawerWidth
        height: root.height
        edge: Qt.LeftEdge
        modal: true
        // 触屏设备禁用拖拽：把屏幕边缘手势留给系统返回
        interactive: !Theme.isTouchPrimary && !Theme.isCompact
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: 0

        background: Rectangle {
            color: Theme.bgSurface
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.avatarLg + Theme.space2xl
                Layout.topMargin: DeviceHelper.safeAreaTop
                Layout.leftMargin: Theme.spaceMd

                RowLayout {
                    anchors.fill: parent
                    spacing: Theme.spaceMd

                    CircleAvatar {
                        id: drawerAvatar
                        size: Theme.avatarMd
                        source: SettingsViewModel.userAvatar
                        fallbackText: SettingsViewModel.userName.length > 0
                                      ? SettingsViewModel.userName.charAt(0) : "?"

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: avatarDialog.open()
                        }
                    }

                    // 双击解锁编辑，回车/失焦后锁定并保存（沿用原有交互）
                    TextInput {
                        id: nameInput
                        Layout.fillWidth: true
                        text: SettingsViewModel.userName
                        font.pixelSize: Theme.fontBody
                        font.bold: true
                        font.family: Theme.fontFamily
                        color: Theme.textPrimary
                        readOnly: true
                        selectByMouse: true

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            cursorShape: Qt.IBeamCursor
                            onDoubleClicked: {
                                nameInput.readOnly = false
                                nameInput.forceActiveFocus()
                            }
                        }

                        onAccepted: nameInput.readOnly = true
                        onActiveFocusChanged: {
                            if (!activeFocus) nameInput.readOnly = true
                        }
                        Keys.onEscapePressed: nameInput.readOnly = true
                        onReadOnlyChanged: {
                            if (readOnly && text.trim().length > 0)
                                SettingsViewModel.userName = text.trim()
                        }
                    }
                }
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: Theme.spaceSm
                Layout.rightMargin: Theme.spaceSm
                model: root.items
                delegate: navItemComponent
                clip: true
                spacing: Theme.spaceXs
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: DeviceHelper.safeAreaBottom
            }
        }
    }

    // 头像选择
    FileDialog {
        id: avatarDialog
        title: "选择头像图片"
        nameFilters: ["图片 (*.png *.jpg *.jpeg *.bmp *.webp)"]
        onAccepted: SettingsViewModel.userAvatar = selectedFile
    }
}
