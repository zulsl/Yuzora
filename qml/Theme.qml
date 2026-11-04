pragma Singleton
import QtQuick 6.8

QtObject {
    id: theme

    // ---- 字体 ----
    property string fontFamily: "Microsoft YaHei"   // 默认字体
    property int    baseFontSize: SettingsViewModel.baseFontSize   // 正文字号大小

    // ========== 主题设置 ==========
    property int materialTheme: SettingsViewModel.themeMode // 0 = Light, 1 = Dark, 2 = System  // 使用 Material 枚举值，避免硬编码
    // ---- 颜色 ----
    property color primaryColor : SettingsViewModel.themeColor // 主色-通常用于顶部状态栏、较大的标题区域的背景色。
    property color accentColor       //强调色-用于按钮、开关、输入框光标等可交互控件的颜色。
    property color foregroundColor  // 前景色
    property color backgroundColor  // 背景色

    property color textColor         // 主要文字色
    property color secondaryTextColor// 次要文字色
    property color borderColor       // 边框色

    // 常量
    readonly property bool isMobile: Screen.width < 640
    readonly property int widget_spacing: 8//控件间距
    readonly property int layout_spacing: 16//组间距
    readonly property int page_padding: 16//页面内边距

    // readonly property int page_margins: 16//外边距
    // readonly property int font_size_title: 18

    // 提供一个计算后的字体对象，方便统一绑定
    readonly property font applicationFont: Qt.font({
        family: theme.fontFamily,
        pixelSize: theme.baseFontSize
    })
}