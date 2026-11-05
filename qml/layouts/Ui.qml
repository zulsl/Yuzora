pragma Singleton
import QtQuick

// 布局层协调单例（供 QML 使用）
// 职责：把"页面想做什么"与"由谁执行"解耦。
// 背景：原先 6 个页面靠动态作用域直接引用 Main.qml 里的 sideDrawer / errorToast /
//       tipToast / mainStack，页面既无法单独预览，改名也只会在运行时静默失效。
// 现在页面只发出 UI 意图，Main 统一执行：
//     Ui.requestDrawer()            → 打开导航抽屉
//     Ui.toast("已保存", false)      → 顶部轻提示
QtObject {
    id: ui

    signal drawerRequested()
    signal toastRequested(string message, bool isError)

    function requestDrawer() {
        drawerRequested()
    }

    function toast(message, isError) {
        if (message === undefined || message === null || message === "")
            return
        toastRequested(String(message), isError === true)
    }
}
