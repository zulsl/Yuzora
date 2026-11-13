import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 浮点数滑块输入
// 职责：滑块 + 数字框联动的取值控件；用户交互时才发出 edited，
//       外部值变化（命令式同步）不会反向触发 edited，避免绑定回环。
// 本次改造只把写死的尺寸/字号换成 Theme token，行为不变。
Item {
    id: root

    property string labelText: ""
    property real value: 1.0
    property real from: 0.0
    property real to: 2.0
    property int decimals: 2

    signal edited()

    implicitWidth: 420
    implicitHeight: Theme.controlLg + Theme.spaceXl + Theme.spaceSm

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spaceXs

        Label {
            Layout.fillWidth: true
            text: root.labelText
            font.pixelSize: Theme.fontBodySm
            font.family: Theme.fontFamily
            color: Theme.textSecondary
            elide: Text.ElideRight
        }

        RowLayout {
            spacing: Theme.spaceSm

            Slider {
                id: slider
                Layout.fillWidth: true
                Layout.minimumWidth: Theme.controlLg * 4
                from: root.from
                to: root.to
                stepSize: 0.01

                onMoved: {
                    const v = Number(value.toFixed(root.decimals))
                    if (Math.abs(root.value - v) > 1e-9) {
                        root.value = v
                        root.edited()
                    }
                }
            }

            TextField {
                id: inputField
                Layout.preferredWidth: Theme.controlLg * 2
                implicitHeight: Theme.controlSm
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontBody
                font.family: Theme.fontFamily

                TextContextMenu {}

                onEditingFinished: {
                    let num = parseFloat(text)
                    if (isNaN(num)) num = root.value
                    num = Math.max(root.from, Math.min(root.to, num))
                    const v = Number(num.toFixed(root.decimals))
                    if (Math.abs(root.value - v) > 1e-9) {
                        root.value = v
                        root.edited()
                    } else {
                        text = v.toFixed(root.decimals)
                    }
                }
            }
        }
    }

    // 外部（模型）→ 控件：命令式同步，不用绑定
    onValueChanged: {
        if (Math.abs(slider.value - value) > 1e-9)
            slider.value = value
        if (!inputField.activeFocus)
            inputField.text = value.toFixed(decimals)
    }

    Component.onCompleted: {
        slider.value = value
        inputField.text = value.toFixed(decimals)
    }
}
