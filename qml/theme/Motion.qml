import QtQuick

// 动效标尺
// 职责：集中所有时长与缓动曲线，配合 Theme.animationsEnabled 实现"一键移除动画"。
// 基线：进入 120–180ms OutCubic，退出 100–150ms InCubic，状态切换 150–200ms InOutQuad。
QtObject {
    // ---- 时长 ----
    readonly property int durationFast: 120    // 退出 / 微交互
    readonly property int durationBase: 160    // 进入 / 常规过渡
    readonly property int durationSlow: 200    // 状态切换 / 展开收起

    // ---- 缓动 ----
    readonly property int easingIn: Easing.InCubic
    readonly property int easingOut: Easing.OutCubic
    readonly property int easingInOut: Easing.InOutQuad

    // ---- 常用组合 ----
    readonly property int enterDuration: durationBase
    readonly property int exitDuration: durationFast
    readonly property int stateDuration: durationSlow

    // ---- 弹簧参数（用于 SpringAnimation 的柔和回弹）----
    readonly property real springDamping: 0.85
    readonly property real springMass: 1.0
    readonly property real springStiffness: 180
}
