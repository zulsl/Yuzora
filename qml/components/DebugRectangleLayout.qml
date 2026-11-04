import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8

Rectangle {
    //布局
    Layout.fillWidth: true
    Layout.fillHeight: true

    //边框
    border.width : 2        //边框宽度
    border.color : "red"

    z: 999

    //颜色
    property color debugColor: "transparent"
    color: debugColor

    // opacity: 0.3 // 不透明度
}