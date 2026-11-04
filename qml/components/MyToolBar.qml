import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8
import QtQuick.Controls.Material 6.8

/**
 * 通用页面顶部工具栏
 * 用法示例：
 *  Page {
 *      header: MyToolBar {
 *          title: "角色管理"
 *          onLeftClicked: sideDrawer.open()
 *      }
 *  }
 */
ToolBar {
    id: root
    height: 50

    // 对外暴露属性
    property string title: ""
    property string leftIconSource: "qrc:/icons/res/menu.svg"
    property bool showLeftButton: true
    // 左侧按钮点击信号
    signal leftClicked()
    // // 右侧自定义内容插槽（可选）
    // default property alias rightContent: rightLayout.children

    RowLayout {
        anchors.fill: parent
        spacing: 8

        // 左侧按钮
        ToolButton {
            id: leftBtn
            visible: showLeftButton
            icon.source: leftIconSource
            onClicked: root.leftClicked()
        }

        // 居中标题
        Label {
            Layout.fillWidth: true
            text: root.title
            font.pixelSize: 18
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        //占位弹簧
        Item {Layout.preferredWidth: leftBtn.width}
    }
}