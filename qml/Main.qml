import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

import Yuzora 1.0

// 主窗口
// 职责：导航宿主、页面栈、全局提示。
// 页面不反查本文件的 id，而是通过 layouts/Ui 单例发出 UI 意图。
ApplicationWindow {
    id: mainWindow

    width: 1280
    height: 720
    visible: true

    // 断点按"窗口宽度"判断。
    // 【重要】Theme 是 QML 单例，不能在它内部绑定 C++ 单例（会导致退出时卡死），
    // 所有外部值统一由这里推入。
    Component.onCompleted: {
        Theme.windowWidth = mainWindow.width
        Theme.windowHeight = mainWindow.height

        Theme.themeMode = SettingsViewModel.themeMode
        Theme.baseFontSize = SettingsViewModel.baseFontSize
        Theme.primaryColor = SettingsViewModel.themeColor
        Theme.systemDarkMode = DeviceHelper.systemDarkMode
        Theme.isTouchPrimary = DeviceHelper.isTouchDevice
    }

    onWidthChanged: Theme.windowWidth = width
    onHeightChanged: Theme.windowHeight = height

    // ---- 设置变化 → 推入 Theme ----
    Connections {
        target: SettingsViewModel
        function onThemeModeChanged() { Theme.themeMode = SettingsViewModel.themeMode }
        function onBaseFontSizeChanged() { Theme.baseFontSize = SettingsViewModel.baseFontSize }
        function onThemeColorChanged() { Theme.primaryColor = SettingsViewModel.themeColor }
    }

    Connections {
        target: DeviceHelper
        function onSystemColorSchemeChanged() {
            Theme.systemDarkMode = DeviceHelper.systemDarkMode
        }
    }

    // ---- Material 主题（Material 控件跟随）----
    Material.theme: Theme.materialTheme
    Material.primary: Theme.primaryColor
    Material.accent: Theme.accentColor

    font: Theme.applicationFont
    color: Theme.bgCanvas

    // 当前页索引（导航高亮用）
    property int currentPage: 0
    // 顶层页面只创建一次，首次访问后保活；避免每次切页都重建重表单。
    // 数组长度与 pageComponents 一致，首项同 initialItem。
    property var pageLoaded: [true, false, false, false, false, false]

    // ========== 窗口根 Item ==========
    // ApplicationWindow 不是 Item：Keys 只能挂在 Item 上，
    // 页面宿主也只能锚定到自己的父/兄弟。
    Item {
        id: contentRoot
        anchors.fill: parent
        focus: true

        Keys.onBackPressed: (event) => {
            mainWindow.handleBack()
            event.accepted = true
        }

        // ========== 导航宿主（宽屏侧栏 / 窄屏抽屉）==========
        NavigationHost {
            id: navHost
            anchors.fill: parent
            // 抽屉遮罩要盖在页面栈之上
            z: 2
            currentIndex: mainWindow.currentPage
            items: [
                { title: "聊天", icon: "qrc:/icons/res/main_page/message-circle.svg" },
                { title: "API 连接配置", icon: "qrc:/icons/res/main_page/plug-connected.svg" },
                { title: "对话预设", icon: "qrc:/icons/res/main_page/adjustments-horizontal.svg" },
                { title: "角色管理", icon: "qrc:/icons/res/main_page/users.svg" },
                { title: "世界书管理", icon: "qrc:/icons/res/main_page/book-2.svg" },
                { title: "系统设置", icon: "qrc:/icons/res/main_page/settings.svg" }
            ]
            onActivated: (index) => mainWindow.switchPage(index)
        }

        // ========== 页面宿主 ==========
        // 与 NavigationHost 是兄弟关系：用左外边距让开侧栏。
        // 不用 StackView.replace：它会在离开时销毁页面，切回来要重建整个角色详情
        // （多个长文本 + 备选开场白），再快的机器也会闪一下。
        // 这里每个页面只创建一次，之后仅切换 visible；保活成本是六个页面实例，
        // 换来的是切页零重建、输入焦点/滚动位置/展开状态都不丢。
        Item {
            id: pageHost
            anchors.fill: parent
            anchors.leftMargin: navHost.wide ? navHost.railWidth : 0

            Repeater {
                model: pageComponents

                delegate: Loader {
                    id: pageLoader
                    required property int index
                    required property var modelData

                    anchors.fill: parent
                    active: mainWindow.pageLoaded[index]
                    visible: index === mainWindow.currentPage
                    sourceComponent: pageLoader.modelData
                }
            }
        }
    }

    property var pageComponents: [
        chatPage, apiConfigPage, presetPage, characterPage, worldInfoPage, settingsPage
    ]

    function switchPage(index) {
        if (index < 0 || index >= pageComponents.length) return

        // 先确保目标页已加载，再切 currentPage：首次访问仍有构建成本，
        // 但之后的来回切换只改 visible，不再销毁/重建页面。
        if (!pageLoaded[index]) {
            const next = pageLoaded.slice()
            next[index] = true
            pageLoaded = next
        }
        mainWindow.currentPage = index
    }

    // ========== 系统返回（Android 返回键 / 手势）==========
    function handleBack() {
        if (navHost.drawerOpened) {
            navHost.closeDrawer()
            return
        }
        mainWindow.close()
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
