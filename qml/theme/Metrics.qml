import QtQuick

// 尺寸与间距标尺
// 职责：集中所有间距、圆角、控件高度、图标、触控目标尺寸。
// 除本文件与 Theme 外，任何 QML 文件不得再出现裸数字间距。
QtObject {
    // ---- 间距 ----
    readonly property int spaceXs: 4
    readonly property int spaceSm: 8
    readonly property int spaceMd: 12
    readonly property int spaceLg: 16
    readonly property int spaceXl: 24
    readonly property int space2xl: 32

    // ---- 圆角 ----
    readonly property int radiusSm: 6
    readonly property int radiusMd: 10
    readonly property int radiusLg: 16
    readonly property int radiusPill: 999

    // ---- 图标 ----
    readonly property int iconSm: 16
    readonly property int iconMd: 20
    readonly property int iconLg: 24

    // ---- 控件高度档位 ----
    readonly property int controlSm: 32
    readonly property int controlMd: 40
    readonly property int controlLg: 48

    // 移动端最小触控目标（48dp）
    readonly property int touchTarget: 48

    // ---- 页面级 ----
    readonly property int pagePaddingCompact: 16
    readonly property int pagePaddingExpanded: 24
    readonly property int gutter: 12
    readonly property int sectionGap: 16

    // ---- 头像 ----
    readonly property int avatarSm: 32
    readonly property int avatarMd: 40
    readonly property int avatarLg: 52

    // ---- 阴影（静态预渲染参数，供 MultiEffect / 位图使用）----
    readonly property int shadowBlur: 16
    readonly property int shadowOffsetY: 2
    readonly property real shadowOpacity: 0.18

    // ---- 长文本阈值（字符数）----
    // 超过这个长度的文本字段改用"只读预览 + 点开编辑"，避免整段文本被 TextArea 排版。
    // 实测：37 000 字 ≈ 330 ms，3 000 字以内基本无感。
    readonly property int longTextThreshold: 3000
}
