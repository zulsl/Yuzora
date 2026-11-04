import QtQuick 6.8
import QtQuick.Controls 6.8

SpinBox {
    id: control
    // 自定义属性
    property int decimals: 2
    property real realValue: 0.0
    property real realStep: 0.1
    property real realFrom: 0.0
    property real realTo: 100.0

    // 根据小数位数计算缩放因子
    readonly property int factor: Math.pow(10, decimals)

    // 将SpinBox的整数值与 realValue 同步
    value: realValue * factor
    from: realFrom * factor
    to: realTo * factor
    stepSize: realStep * factor

    // 显示时转换回浮点数
    textFromValue: function(value, locale) {
        return Number(value / factor).toFixed(decimals)
    }

    // 用户输入时转换回整数存储
    valueFromText: function(text, locale) {
        return parseFloat(text) * factor
    }

    // 当SpinBox的值变化时，更新 realValue
    onValueChanged: {
        realValue = value / factor
    }
}