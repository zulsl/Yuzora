import QtQuick
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

    readonly property bool wide: width >= Theme.bpExpanded  //宽态
    readonly property int railWidth: Theme.controlLg * 3 + Theme.spaceLg    //菜单宽度
    readonly property int drawerWidth: Math.min(320, Math.round(root.width * 0.6))
        //窄态抽屉宽度

    function openDrawer() { if (!wide) drawerOpen = true }
    function closeDrawer() { drawerOpen = false }

    // 窗口拉宽到侧栏形态时自动收起抽屉
    onWideChanged: if (wide) drawerOpen = false

    // ========== 快捷键 ==========
    // Esc 是桌面习惯；安卓返回键到 Qt 是 Qt.Key_Back（不是 Esc），两个都接，
    // 否则手机上打开导航抽屉后按返回键只能看着它不动。
    Shortcut {
        sequences: ["Esc", "Back"]
        enabled: root.drawerOpen
        onActivated: root.closeDrawer()
    }

    // ==========================================================
    // 宽态
    // ==========================================================
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ---- 常驻侧栏 ----
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

            NavigationPanel {
                anchors.fill: parent
                items: root.items
                currentIndex: root.currentIndex
                onActivated: (index) => {
                    root.closeDrawer()
                    root.activated(index)
                }
                onAvatarRequested: avatarDialog.open()
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

            NavigationPanel {
                anchors.fill: parent
                items: root.items
                currentIndex: root.currentIndex
                showBottomSafeArea: true
                onActivated: (index) => {
                    root.closeDrawer()
                    root.activated(index)
                }
                onAvatarRequested: avatarDialog.open()
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
