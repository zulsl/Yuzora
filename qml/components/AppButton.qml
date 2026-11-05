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

    readonly property bool _primary: variant === "primary"
    readonly property bool _danger: variant === "danger"
    readonly property bool _ghost: variant === "ghost"

    implicitHeight: Theme.controlMd
    leftPadding: Theme.spaceMd
    rightPadding: Theme.spaceMd
    font.pixelSize: Theme.fontBody
    font.family: Theme.fontFamily

    icon.source: iconSource
    icon.width: iconSource.length > 0 ? Theme.iconSm : 0
    icon.height: iconSource.length > 0 ? Theme.iconSm : 0
    icon.color: _primary ? Theme.textOnAccent
              : _danger ? Theme.danger
              : Theme.textPrimary

    // 自定义 contentItem 会取代 Button 自带的图标渲染，
    // 因此图标与文字必须自己排布，否则会出现"有图标宽度但图标不显示"的空档。
    contentItem: Row {
        spacing: root.iconSource.length > 0 ? Theme.spaceSm : 0
        anchors.centerIn: parent

        Image {
            visible: root.iconSource.length > 0
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.iconSm
            height: Theme.iconSm
            source: root.iconSource
            sourceSize: Qt.size(Theme.iconSm, Theme.iconSm)
            fillMode: Image.PreserveAspectFit
            // 用 SVG 自带的单色 + 着色叠加，保证深浅主题下都清晰
            opacity: root.enabled ? 1.0 : 0.5
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            font: root.font
            color: root.icon.color
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

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
        Behavior on color {
            ColorAnimation { duration: Theme.durationFast; easing.type: Theme.easingOut }
        }
    }

    // 禁用态文字直接跟随 contentItem 的颜色计算
    opacity: enabled ? 1.0 : 0.6
}
