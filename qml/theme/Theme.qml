pragma Singleton
import QtQuick

// 主题单例（唯一对外入口）
// 职责：
//   1. 把 PaletteLight / PaletteDark / Metrics / Motion 聚合成一个稳定的 `Theme.*` API；
//   2. 暴露断点与窗口类别（由 Main.qml 通过 Binding 注入 windowWidth/windowHeight，
//      不再使用 Screen.width，以符合"按窗口宽度断点 600/1024"的基线）；
//   3. 持有动画总开关 animationsEnabled，实现"所有动画可一键移除"。
// 约束：本文件与 theme/ 下其它文件是唯一允许出现裸色值与裸数字的地方。
QtObject {
    id: theme

    // ========== 窗口尺寸（供 QML 使用）==========
    // 由 Main.qml 以 Binding 注入真实窗口尺寸，默认值仅用于无窗口场景兜底。
    property real windowWidth: 1280
    property real windowHeight: 720

    // ========== 断点 ==========
    readonly property int bpCompact: 600    //手机
    readonly property int bpExpanded: 1024  //电脑

    readonly property bool isCompact: windowWidth < bpCompact
    readonly property bool isExpanded: windowWidth >= bpExpanded
    readonly property bool isMedium: !isCompact && !isExpanded

    // 兼容旧名：历史代码用 Theme.isMobile，语义改为"窗口窄于 600"
    readonly property bool isMobile: isCompact
    // 是否触屏优先设备（用于 hover 态、鼠标光标等降级）
    // 【重要】QML 单例不在内部绑定 C++ 单例：否则引擎析构阶段会卡死（关窗后进程不退出）。
    // 这些值统一由 Main.qml 用 Connections 推入。
    property bool isTouchPrimary: false

    // ========== 主题模式 ==========
    // 取值沿用 Material 语义：0 = Light, 1 = Dark, 2 = System
    // 由 Main.qml 从 SettingsViewModel.themeMode 推入
    property int themeMode: 0
    // 系统当前是否深色，由 Main.qml 从 DeviceHelper.systemDarkMode 推入
    property bool systemDarkMode: false

    readonly property int materialTheme: themeMode
    readonly property bool isDark: themeMode === 1
                                 || (themeMode === 2 && systemDarkMode)

    // ========== 色板 ==========
    readonly property PaletteLight lightPalette: PaletteLight {}
    readonly property PaletteDark darkPalette: PaletteDark {}
    // 用 var 承接：PaletteLight / PaletteDark 是两个不同的 QMLObject 类型，
    // 声明成 QtObject 会让静态检查认为所有色板成员都不存在。
    readonly property var palette: isDark ? darkPalette : lightPalette

    // 背景
    readonly property color bgCanvas: palette.bgCanvas
    readonly property color bgSurface: palette.bgSurface
    readonly property color bgSurfaceVariant: palette.bgSurfaceVariant
    readonly property color bgElevated: palette.bgElevated

    // 文字
    readonly property color textPrimary: palette.textPrimary
    readonly property color textSecondary: palette.textSecondary
    readonly property color textDisabled: palette.textDisabled
    readonly property color textOnAccent: palette.textOnAccent

    // 描边
    readonly property color borderDefault: palette.borderDefault
    readonly property color borderStrong: palette.borderStrong

    // 状态
    readonly property color success: palette.success
    readonly property color successSubtle: palette.successSubtle
    readonly property color warning: palette.warning
    readonly property color danger: palette.danger
    readonly property color dangerSubtle: palette.dangerSubtle

    // 聊天气泡
    readonly property color bubbleUser: palette.bubbleUser
    readonly property color bubbleUserText: palette.bubbleUserText
    readonly property color bubbleAssistant: palette.bubbleAssistant
    readonly property color bubbleAssistantBorder: palette.bubbleAssistantBorder

    // 交互态
    readonly property color rowHover: palette.rowHover
    readonly property color rowSelected: palette.rowSelected

    // 覆盖与阴影
    readonly property color overlayScrim: palette.overlayScrim
    readonly property color shadowColor: palette.shadowColor

    // ========== 主色（唯一来源：SettingsViewModel.themeColor，由 Main 推入）==========
    property color primaryColor: "#4169E1"
    readonly property color accentColor: primaryColor              // 兼容旧名
    readonly property color accentHover: Qt.lighter(primaryColor, 1.12)
    readonly property color accentPressed: Qt.darker(primaryColor, 1.12)
    // 按下/选中用的主色淡层：0.12 在浅灰背景上几乎看不出，提到 0.18
    readonly property color accentSubtle: Qt.rgba(primaryColor.r, primaryColor.g,
                                                  primaryColor.b, 0.18)

    // 兼容旧名（历史上声明未赋值的几个语义色，现在指向真实值）
    readonly property color backgroundColor: bgCanvas
    readonly property color foregroundColor: textPrimary
    readonly property color textColor: textPrimary
    readonly property color secondaryTextColor: textSecondary
    readonly property color borderColor: borderDefault

    // ========== 字体 ==========
    // Windows 用雅黑。
    // 安卓必须给一个**真实的字体族名**，不能留空：空 family 在 Qt 6 的 fontconfig
    // 后端下没有"用系统默认"这种语义，而是拿一个空 pattern 去匹配——会匹配到
    // Roboto 的**斜体** face，于是英文（API / AI）是斜的、中文靠字形回退反而是正的。
    // Roboto 是 AOSP 自带字体；中文没有的字形会自动回退到 Noto Sans CJK。
    // 不用 Qt.application.font：它不是稳定的跨平台属性，取不到时反而会抛异常。
    readonly property string fontFamily: {
        if (Qt.platform.os === "windows")
            return "Microsoft YaHei"
        if (Qt.platform.os === "android")
            return "Roboto"
        return ""
    }
    // 正文字号（由 Main.qml 从 SettingsViewModel.baseFontSize 推入）
    property int baseFontSize: 14

    readonly property int fontDisplay: 24
    readonly property int fontTitle: 18
    readonly property int fontBody: baseFontSize
    readonly property int fontBodySm: Math.max(11, baseFontSize - 1)
    readonly property int fontCaption: Math.max(10, baseFontSize - 3)

    readonly property font applicationFont: Qt.font({
        family: fontFamily,
        pixelSize: baseFontSize
    })

    // ========== 尺寸（转发 Metrics）==========
    readonly property Metrics metrics: Metrics {}
    readonly property Motion motion: Motion {}

    //间距
    readonly property int spaceXs: metrics.spaceXs
    readonly property int spaceSm: metrics.spaceSm
    readonly property int spaceMd: metrics.spaceMd
    readonly property int spaceLg: metrics.spaceLg
    readonly property int spaceXl: metrics.spaceXl
    readonly property int space2xl: metrics.space2xl

    //圆角
    readonly property int radiusSm: metrics.radiusSm
    readonly property int radiusMd: metrics.radiusMd
    readonly property int radiusLg: metrics.radiusLg
    readonly property int radiusPill: metrics.radiusPill

    //图标
    readonly property int iconSm: metrics.iconSm
    readonly property int iconMd: metrics.iconMd
    readonly property int iconLg: metrics.iconLg

    //控件高度
    readonly property int controlSm: metrics.controlSm
    readonly property int controlMd: metrics.controlMd
    readonly property int controlLg: metrics.controlLg
    readonly property int touchTarget: metrics.touchTarget

    //头像
    readonly property int avatarSm: metrics.avatarSm
    readonly property int avatarMd: metrics.avatarMd
    readonly property int avatarLg: metrics.avatarLg

    // 页面内边距：窄窗口用紧一档
    readonly property int pagePadding: isCompact ? metrics.pagePaddingCompact
                                                 : metrics.pagePaddingExpanded
    readonly property int gutter: metrics.gutter
    readonly property int sectionGap: metrics.sectionGap

    // 兼容旧名
    readonly property int widget_spacing: metrics.spaceSm
    readonly property int layout_spacing: metrics.spaceLg
    readonly property int page_padding: pagePadding

    // ========== 动效 ==========
    // 一键移除动画：置 false 后所有用 Theme.duration* 的 Behavior 变为 0ms
    property bool animationsEnabled: true

    readonly property int durationFast: animationsEnabled ? motion.durationFast : 0
    readonly property int durationBase: animationsEnabled ? motion.durationBase : 0
    readonly property int durationSlow: animationsEnabled ? motion.durationSlow : 0
    readonly property int durationEnter: animationsEnabled ? motion.enterDuration : 0
    readonly property int durationExit: animationsEnabled ? motion.exitDuration : 0
    readonly property int durationState: animationsEnabled ? motion.stateDuration : 0

    readonly property int easingIn: motion.easingIn
    readonly property int easingOut: motion.easingOut
    readonly property int easingInOut: motion.easingInOut

    readonly property real springDamping: motion.springDamping
    readonly property real springMass: motion.springMass
    readonly property real springStiffness: motion.springStiffness
}
