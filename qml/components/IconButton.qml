import QtQuick
import QtQuick.Controls

// 图标按钮
// 职责：所有"图标即动作"的按钮统一入口；保证移动端触控目标不小于 48×48dp。
// 可配置属性：tip（悬停提示）、buttonSize（默认 Theme.touchTarget）、
//             danger（危险动作着色）、compact（用 40 高的一档）。
//
// 交互反馈与 AppButton 保持一致：图标颜色不随悬停/按下改变，反馈只体现在背景上。
// 背景用「固定颜色 + 只动 opacity」：
//   若用 ColorAnimation 从 "transparent"(rgba(0,0,0,0)) 插值到浅灰，
//   中间会经过半透明深灰，视觉上就是"先闪黑再变淡灰"。
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
    icon.color: !root.enabled ? Theme.textDisabled
              : root.danger ? Theme.danger
              : Theme.textPrimary

    // 触屏设备不产生 hover 态，也不弹 ToolTip
    hoverEnabled: !Theme.isTouchPrimary
    ToolTip.visible: tip.length > 0 && hovered && !Theme.isTouchPrimary
    ToolTip.text: tip
    ToolTip.delay: 500

    background: Rectangle {
        radius: Theme.radiusSm
        // 颜色固定（按下/悬停各一档），不参与颜色插值
        color: root.pressed ? Theme.accentSubtle : Theme.rowHover
        // 只对透明度做动画：0 → 1，不会经过任何中间色
        opacity: (root.enabled && (root.hovered || root.pressed)) ? 1 : 0
        Behavior on opacity {
            NumberAnimation { duration: Theme.durationFast; easing.type: Theme.easingOut }
        }
    }
}
