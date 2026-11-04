import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Controls.Material 6.8

ToolButton {
    icon.source: "qrc:/icons/res/main_page/book-2.svg"
    ToolTip.text:"角色世界书"

    hoverEnabled: true
    ToolTip.visible: hovered

    // icon.color: "black"
    // background: Rectangle {
    //     // radius: 4//圆角
    //     color: parent.pressed ? "#bbbbbb" : parent.hovered ? "#cccccc" : "transparent"
    // }
}