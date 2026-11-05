import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

import Yuzora 1.0

// 主窗口
// 职责收窄为三件事：导航宿主、页面栈、全局提示。
// 具体页面不再反查本文件的 id，而是通过 layouts/Ui 单例发出 UI 意图，
// 从而可以单独预览、单独测试。
ApplicationWindow {
    id: mainWindow

    width: 1280
    height: 720
    minimumWidth: 360
    minimumHeight: 480
    visible: true

    // 断点必须按"窗口宽度"判断，不能用 Screen.width：
    // Theme 是单例、拿不到窗口，这里把真实尺寸注入进去。
    Component.onCompleted: {
        Theme.windowWidth = Qt.binding(() => mainWindow.width)
        Theme.windowHeight = Qt.binding(() => mainWindow.height)
    }

    // ---- Material 主题（Material 控件跟随）----
    Material.theme: Theme.materialTheme
    Material.primary: Theme.primaryColor
    Material.accent: Theme.accentColor

    font: Theme.applicationFont
    color: Theme.bgCanvas

    // 当前页索引（导航高亮用）
    property int currentPage: 0

    // ========== 导航宿主（宽屏侧栏 / 窄屏抽屉）==========
    NavigationHost {
        id: navHost
        anchors.fill: parent
        currentIndex: mainWindow.currentPage
        items: [
            { title: "聊天", icon: "qrc:/icons/res/main_page/message-circle.svg" },
            { title: "API 连接配置", icon: "qrc:/icons/res/main_page/plug-connected.svg" },
            { title: "对话补全预设", icon: "qrc:/icons/res/main_page/adjustments-horizontal.svg" },
            { title: "角色管理", icon: "qrc:/icons/res/main_page/users.svg" },
            { title: "世界书管理", icon: "qrc:/icons/res/main_page/book-2.svg" },
            { title: "系统设置", icon: "qrc:/icons/res/main_page/settings.svg" }
        ]
        onActivated: (index) => mainWindow.switchPage(index)
    }

    // ========== 页面栈 ==========
    StackView {
        id: mainStack
        anchors.fill: navHost.contentArea
        initialItem: chatPage
    }

    property var pageComponents: [
        chatPage, apiConfigPage, presetPage, characterPage, worldInfoPage, settingsPage
    ]

    function switchPage(index) {
        if (index < 0 || index >= pageComponents.length) return
        mainWindow.currentPage = index
        mainStack.replace(pageComponents[index])
    }

    // ========== 系统返回（Android 返回键 / 手势）==========
    function handleBack() {
        if (navHost.drawerOpened) {
            navHost.closeDrawer()
            return
        }
        if (mainStack.depth > 1) {
            mainStack.pop()
            return
        }
        mainWindow.close()
    }

    Keys.onBackPressed: (event) => {
        mainWindow.handleBack()
        event.accepted = true
    }

    // ========== 全局提示 ==========
    AppToast {
        id: globalToast
    }

    Connections {
        target: Ui

        function onDrawerRequested() {
            navHost.openDrawer()
        }

        function onToastRequested(message, isError) {
            globalToast.showToast(message, isError)
        }
    }

    // 全局异常 → 统一吐司
    Connections {
        target: ExceptionHandler
        function onErrorOccurred(message) {
            globalToast.showToast(message, true)
        }
        function onTipOccurred(message) {
            globalToast.showToast(message, false)
        }
    }

    // 角色切换到聊天页
    Connections {
        target: CharacterViewModel
        function onNavigateToChatPage() {
            mainWindow.switchPage(0)
        }
    }

    // ========== 页面组件 ==========
    Component {
        id: chatPage
        ChatPage {}
    }

    Component {
        id: apiConfigPage
        ApiConfigPage {}
    }

    Component {
        id: presetPage
        CompletionPresetPage {}
    }

    Component {
        id: characterPage
        CharacterPage {}
    }

    Component {
        id: worldInfoPage
        WorldInfoPage {}
    }

    Component {
        id: settingsPage
        SettingsPage {}
    }
}
