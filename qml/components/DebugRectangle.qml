import QtQuick 6.8
import QtQuick.Controls 6.8

Rectangle {
    //填充
    property var toFill: parent
    anchors.fill: toFill

    //边框
    border.width : 2        //边框宽度
    border.color : "red"

    z: 999

    //颜色
    property color debugColor: "transparent"
    color: debugColor

    // opacity: 0.3 // 不透明度
}
