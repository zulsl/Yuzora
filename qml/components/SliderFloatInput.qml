// SliderFloatInput.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property string labelText: "温度"
    property real value: 1.00
    property real from: 0.0
    property real to: 2.0
    property int decimals: 2

    // 只在用户交互（拖滑块 / 改输入框）时发出
    signal edited

    width: 600
    height: 80

    // 外部（模型）→ 内部：命令式同步，不用绑定
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

    ColumnLayout {
        anchors.fill: parent
        Text {
            text: labelText
            font.pixelSize: 16
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
        }
        RowLayout {
            spacing: 12

            Slider {
                id: slider
                Layout.fillWidth: true
                Layout.minimumWidth: 300
                from: root.from
                to: root.to
                stepSize: 0.01

                // 用户拖动 / 点击 —— 唯一触发 edited 的地方
                onMoved: {
                    var v = Number(value.toFixed(root.decimals))
                    if (Math.abs(root.value - v) > 1e-9) {
                        root.value = v
                        root.edited()
                    }
                }
            }

            TextField {
                id: inputField
                Layout.preferredWidth: 160
                font.pixelSize: 32
                horizontalAlignment: Text.AlignHCenter

                onEditingFinished: {
                    var num = parseFloat(text)
                    if (isNaN(num)) num = root.value
                    num = Math.max(root.from, Math.min(root.to, num))
                    var v = Number(num.toFixed(root.decimals))
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
}