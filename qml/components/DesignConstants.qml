pragma Singleton
import QtQuick 6.8


QtObject {
    // ========== 颜色规范 ==========
    readonly property color primary: "#2563eb"
    readonly property color secondary: "#64748b"
    readonly property color success: "#10b981"
    readonly property color danger: "#ef4444"
    readonly property color bgDark: "#0f172a"
    readonly property color bgLight: "#ffffff"

    // ========== 尺寸间距 ==========
    readonly property int spacingXs: 4
    readonly property int spacingSm: 8
    readonly property int spacingMd: 12
    readonly property int spacingLg: 16
    readonly property int spacingXl: 24

    readonly property int radiusSm: 6
    readonly property int radiusMd: 10
    readonly property int radiusLg: 16

    // ========== 字体 ==========
    readonly property int fontH1: 28
    readonly property int fontH2: 24
    readonly property int fontBody: 14
    readonly property int fontTip: 12

    // ========== 通用动画时长 ==========
    readonly property int animFast: 100
    readonly property int animNormal: 200
    readonly property int animSlow: 350
}