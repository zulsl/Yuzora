import QtQuick
import QtQuick.Controls

// 文本按钮
// 职责：统一按钮的四档语义样式，避免各页面各自堆 Button + 内联颜色。
// 可配置属性：variant（primary / secondary / ghost / danger）、text、iconSource。
// 替换：各页面里裸写的 Button，以及带内联 color 的自绘按钮。
Button {
    id: root

    // "primary" 主操作用主色；"secondary" 常规；"ghost" 无边框；"danger" 破坏性操作
    property string variant: "secondary"
    property string iconSource: ""
    // 覆盖图标颜色。默认透明＝不覆盖，用下面按 variant 推导的语义色。
    // 用途：世界书触发策略要保留蓝/绿点自身的颜色，不能被统一染成前景色。
    property color iconColorOverride: "transparent"

    readonly property bool _primary: variant === "primary"
    readonly property bool _danger: variant === "danger"
    readonly property bool _ghost: variant === "ghost"

    implicitHeight: Theme.controlMd
    leftPadding: Theme.spaceMd
    rightPadding: Theme.spaceMd
    font.pixelSize: Theme.fontBody
    font.family: Theme.fontFamily

    // 用控件自带的 icon 渲染（IconLabel）而不是自绘 Image：
    // 裸 Image 加载 SVG 时用文件里写死的黑色 fill，不跟随主题；
    // icon.color 会对单色图标着色，深色主题下才是正确颜色。
    icon.source: iconSource
    icon.width: iconSource.length > 0 ? Theme.iconSm : 0
    icon.height: iconSource.length > 0 ? Theme.iconSm : 0
    icon.color: iconColorOverride.a > 0 ? iconColorOverride
              : _primary ? Theme.textOnAccent
              : _danger ? Theme.danger
              : Theme.textPrimary

    display: (iconSource.length > 0 && text.length > 0) ? AbstractButton.TextBesideIcon
           : (iconSource.length > 0 ? AbstractButton.IconOnly
                                    : AbstractButton.TextOnly)

    background: Rectangle {
        radius: Theme.radiusSm
        color: {
            if (!root.enabled) return Theme.bgSurfaceVariant
            if (root._primary) {
                if (root.pressed) return Theme.accentPressed
                if (root.hovered) return Theme.accentHover
                return Theme.primaryColor
            }
            if (root._danger) {
                if (root.pressed) return Theme.dangerSubtle
                if (root.hovered) return Theme.dangerSubtle
                return "transparent"
            }
            if (root._ghost) {
                if (root.pressed) return Theme.accentSubtle
                if (root.hovered) return Theme.rowHover
                return "transparent"
            }
            // secondary
            if (root.pressed) return Theme.bgSurfaceVariant
            if (root.hovered) return Theme.rowHover
            return Theme.bgSurface
        }
        border.width: (root._ghost || root._primary) ? 0 : 1
        border.color: root._danger ? Theme.danger : Theme.borderDefault
        // ghost / danger 静止态是 "transparent"，对它做颜色插值会经过半透明深灰
        // （视觉上"闪一下黑"）。这两种变体改为瞬时切换，其余变体保留淡入。
        Behavior on color {
            enabled: root._primary || (!root._ghost && !root._danger) || !root.enabled
            ColorAnimation { duration: Theme.durationFast; easing.type: Theme.easingOut }
        }
    }

    // 禁用态文字直接跟随 contentItem 的颜色计算
    opacity: enabled ? 1.0 : 0.6
}
