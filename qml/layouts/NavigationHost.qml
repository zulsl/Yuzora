import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import Yuzora 1.0

// 导航宿主
// 职责：
//   宽窗口（>= 1024）：常驻左侧菜单栏，不提供汉堡按钮；
//   窄窗口（< 1024）：默认不显示菜单，由页面左上角按钮打开抽屉。
// 抽屉用自绘遮罩实现（而不是 Popup/Drawer）：点击遮罩空白处一定关闭，
// 不受 Popup 关闭策略与层级影响；关闭时整层 disabled，不拦截页面点击。
Item {
    id: root

    property alias content: contentHost.data
    property Item contentArea: contentHost
    property var items: []
    property int currentIndex: 0

    // 抽屉开关状态（供 QML 使用）
    property bool drawerOpen: false
    readonly property bool drawerOpened: drawerOpen

    signal activated(int index)

    readonly property bool wide: width >= Theme.bpExpanded
    readonly property int railWidth: Theme.controlLg * 2 + Theme.spaceXl
    readonly property int drawerWidth: Math.min(320, Math.round(root.width * 0.84))

    function openDrawer() { if (!wide) drawerOpen = true }
    function closeDrawer() { drawerOpen = false }

    // 窗口拉宽到侧栏形态时自动收起抽屉
    onWideChanged: if (wide) drawerOpen = false

    // Esc 关闭抽屉
    Shortcut {
        sequence: "Esc"
        enabled: root.drawerOpen
        onActivated: root.closeDrawer()
    }

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

            onClicked: {
                root.closeDrawer()
                root.activated(index)
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

    // ==========================================================
    // 窄态：自绘抽屉（遮罩 + 滑入面板）
    // ==========================================================
    Item {
        id: drawerLayer
        anchors.fill: parent
        z: 100
        // 关闭后整层失效，避免遮罩继续拦截页面点击
        enabled: root.drawerOpen
        // 关闭动画期间保持可见，动画结束才隐藏
        visible: root.drawerOpen || drawerPanel.x > -drawerPanel.width + 1

        // 遮罩：点击任意空白处关闭
        Rectangle {
            id: drawerScrim
            anchors.fill: parent
            color: Theme.overlayScrim
            opacity: root.drawerOpen ? 1 : 0
            Behavior on opacity {
                NumberAnimation { duration: Theme.durationBase; easing.type: Theme.easingOut }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.closeDrawer()
            }
        }

        // 面板
        Rectangle {
            id: drawerPanel
            width: root.drawerWidth
            height: parent.height
            x: root.drawerOpen ? 0 : -width
            color: Theme.bgSurface

            Behavior on x {
                NumberAnimation {
                    duration: Theme.durationState
                    easing.type: Theme.easingInOut
                }
            }

            // 面板自身吞掉点击，避免穿透到遮罩
            MouseArea {
                anchors.fill: parent
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // 顶部：头像（点击更换）+ 用户名（双击改名）
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Theme.avatarLg + Theme.space2xl
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

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: avatarDialog.open()
                            }
                        }

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
    }

    // 头像选择
    FileDialog {
        id: avatarDialog
        title: "选择头像图片"
        nameFilters: ["图片 (*.png *.jpg *.jpeg *.bmp *.webp)"]
        onAccepted: SettingsViewModel.userAvatar = selectedFile
    }
}
