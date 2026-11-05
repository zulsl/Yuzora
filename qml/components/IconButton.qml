import QtQuick
import QtQuick.Controls

// 图标按钮
// 职责：所有"图标即动作"的按钮统一入口；保证移动端触控目标不小于 48×48dp。
// 可配置属性：tip（悬停提示）、buttonSize（默认 Theme.touchTarget）、
//             danger（危险动作着色）、compact（用 40 高的一档）。
// 替换：MyToolButton.qml，以及各页面里散落的裸 ToolButton。
ToolButton {
    id: root

    property string tip: ""
    property bool danger: false
    property bool compact: false
    property int buttonSize: compact ? Theme.controlMd : Theme.touchTarget

    implicitWidth: buttonSize
    implicitHeight: buttonSize

    icon.width: Theme.iconMd
    icon.height: Theme.iconMd
    icon.color: {
        if (!enabled) return Theme.textDisabled
        if (danger) return Theme.danger
        if (pressed || hovered) return Theme.primaryColor
        return Theme.textPrimary
    }

    // 触屏设备不产生 hover 态，也不弹 ToolTip
    hoverEnabled: !Theme.isTouchPrimary
    ToolTip.visible: tip.length > 0 && hovered && !Theme.isTouchPrimary
    ToolTip.text: tip
    ToolTip.delay: 500

    background: Rectangle {
        radius: Theme.radiusSm
        color: {
            if (!root.enabled) return "transparent"
            if (root.pressed) return Theme.accentSubtle
            if (root.hovered) return Theme.rowHover
            return "transparent"
        }
        Behavior on color {
            ColorAnimation { duration: Theme.durationFast; easing.type: Theme.easingOut }
        }
    }
}
