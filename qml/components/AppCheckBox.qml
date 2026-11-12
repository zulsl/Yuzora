import QtQuick
import QtQuick.Controls

// 复选框
// 职责：统一"勾选"控件的外观。
// 为什么自绘 indicator：Material 默认方框只有 18px，且在 CheckBox 内部还有 12px 横向缩进，
// 放进 48 高的列表行里既小、又比行内其它内容右移一截。这里改成 20px 圆角方块、左缘与行内边距对齐。
// 注意：自定义 indicator 不会被 CheckBox 自动定位（实测恒为 0,0），位置必须在下面自己给。
CheckBox {
    id: root

    // 方框边长；触控热区沿垂直方向由 implicitHeight 提供
    property int boxSize: 20

    implicitWidth: boxSize + Theme.spaceXs
    implicitHeight: Theme.touchTarget

    leftPadding: 0
    rightPadding: 0

    indicator: Rectangle {
        id: box

        // 横向贴住内容左缘，纵向相对复选框居中
        x: 0
        anchors.verticalCenter: parent.verticalCenter
        implicitWidth: root.boxSize
        implicitHeight: root.boxSize

        radius: Theme.radiusSm - 2
        color: root.checked ? Theme.primaryColor : "transparent"
        border.width: root.checked ? 0 : 2
        border.color: root.checked ? Theme.primaryColor
                    : root.hovered ? Theme.textSecondary : Theme.borderStrong

        // 对勾用两段圆角矩形拼出来：
        // 文字 "✓" 的字形/基线由字体决定，粗细不可控、不同平台还会发虚；
        // 路径描边在 20px 这个尺寸下观感偏"锐"，两段矩形的粗细和圆头更稳。
        // 下面的数字按 20px 方框标定，u 让它们随 boxSize 等比缩放。
        Item {
            id: tick
            readonly property real u: root.boxSize / 20

            anchors.centerIn: parent
            visible: root.checked
            width: Math.round(root.boxSize * 0.6)
            height: width

            // 短笔：左下端 → 拐点
            Rectangle {
                width: 2.2 * tick.u
                height: 5.6 * tick.u
                radius: width / 2
                color: Theme.textOnAccent
                x: 1.6 * tick.u
                y: 5.0 * tick.u
                rotation: -45
                transformOrigin: Item.Center
            }

            // 长笔：拐点 → 右上端
            Rectangle {
                width: 2.2 * tick.u
                height: 10.0 * tick.u
                radius: width / 2
                color: Theme.textOnAccent
                x: 6.4 * tick.u
                y: 0.4 * tick.u
                rotation: 45
                transformOrigin: Item.Center
            }
        }
    }
}
