import QtQuick

// 浅色色板
// 职责：只声明"低饱和"的中性色与语义色值，不含尺寸、动效或任何逻辑。
// 主色（accent）不在此处，唯一来源是 SettingsViewModel.themeColor，由 Theme 派生明暗变体。
QtObject {
    // ---- 背景层次 ----
    readonly property color bgCanvas: "#f4f5f7"          // 页面最底层
    readonly property color bgSurface: "#ffffff"         // 卡片 / 面板
    readonly property color bgSurfaceVariant: "#eef0f3"  // 次级面板 / 未选中行
    readonly property color bgElevated: "#ffffff"        // 浮层（菜单、对话框）

    // ---- 文字 ----
    readonly property color textPrimary: "#1f2328"
    readonly property color textSecondary: "#5c6470"
    readonly property color textDisabled: "#9aa1ab"
    readonly property color textOnAccent: "#ffffff"

    // ---- 描边 ----
    readonly property color borderDefault: "#dfe3e8"
    readonly property color borderStrong: "#c4cad2"

    // ---- 状态 ----
    readonly property color success: "#2f9e63"
    readonly property color successSubtle: "#e6f4ec"
    readonly property color warning: "#b7791f"
    readonly property color danger: "#d9534f"
    readonly property color dangerSubtle: "#fbeaea"

    // ---- 聊天气泡 ----
    readonly property color bubbleUser: "#e8f3e2"
    readonly property color bubbleUserText: "#1f2328"
    readonly property color bubbleAssistant: "#ffffff"
    readonly property color bubbleAssistantBorder: "#e3e6ea"

    // ---- 交互态 ----
    // 悬停要比 bgSurface(#ffffff) 明显深一档，否则几乎看不见；
    // 选中再用偏蓝的一档压过悬停，形成「悬停 → 选中」的层次。
    readonly property color rowHover: "#e4e8ee"
    readonly property color rowSelected: "#d3e0f5"

    // ---- 覆盖与阴影 ----
    readonly property color overlayScrim: "#66000000"    // 模态遮罩
    readonly property color shadowColor: "#1a2b3a"       // 深色变体，非纯黑
}
