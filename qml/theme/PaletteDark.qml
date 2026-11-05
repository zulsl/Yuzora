import QtQuick

// 深色色板
// 职责：与 PaletteLight 一一对应的同义属性名，保证 Theme 切换色板时页面零改动。
// 低饱和、低对比冲击：背景用中性冷灰而非纯黑，文字不用纯白。
QtObject {
    // ---- 背景层次 ----
    readonly property color bgCanvas: "#16181c"
    readonly property color bgSurface: "#1e2126"
    readonly property color bgSurfaceVariant: "#262a30"
    readonly property color bgElevated: "#23272d"

    // ---- 文字 ----
    readonly property color textPrimary: "#e6e8eb"
    readonly property color textSecondary: "#a5acb6"
    readonly property color textDisabled: "#6b727c"
    readonly property color textOnAccent: "#ffffff"

    // ---- 描边 ----
    readonly property color borderDefault: "#333841"
    readonly property color borderStrong: "#454b55"

    // ---- 状态 ----
    readonly property color success: "#4cb782"
    readonly property color successSubtle: "#1f3a2d"
    readonly property color warning: "#d0a04a"
    readonly property color danger: "#e07e7a"
    readonly property color dangerSubtle: "#3d2626"

    // ---- 聊天气泡 ----
    readonly property color bubbleUser: "#2f4a3a"
    readonly property color bubbleUserText: "#e8f0e9"
    readonly property color bubbleAssistant: "#262a30"
    readonly property color bubbleAssistantBorder: "#363b43"

    // ---- 交互态 ----
    // 深色下 bgSurface 是 #1e2126，悬停必须明显亮一档才看得见；
    // 选中再用偏蓝且更亮的一档。
    readonly property color rowHover: "#333944"
    readonly property color rowSelected: "#3d4d66"

    // ---- 覆盖与阴影 ----
    readonly property color overlayScrim: "#99000000"
    readonly property color shadowColor: "#0b0d10"
}
