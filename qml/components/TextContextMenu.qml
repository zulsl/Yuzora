pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

// 文本控件的右键菜单
// 用法：直接放进 TextEdit / TextArea / TextField 内部，一行即可：
//     TextArea { TextContextMenu {} }
//
// 原理：覆盖一层只接收右键的透明层。左键事件不受影响 —— 实测在覆盖层上拖选，
// 宿主控件仍能正常选中文本，右键回调触发时选中内容也在。
// z 必须压过 Controls 控件内部的 contentItem，否则右键会先被内部 TextEdit 吃掉。
//
// 只读控件（消息气泡等）只生成 复制 / 全选 两项；可编辑控件才多出 剪切 / 粘贴。
// 注意是"不生成"而不是 visible: false —— 菜单内容是列表，隐藏项照样占位置，
// 只读时会空出一大块。
Item {
    id: root

    anchors.fill: parent
    z: 1

    // 宿主控件：TextEdit / TextArea / TextField 都提供 copy/cut/paste/selectAll
    readonly property var target: parent
    readonly property bool isReadOnly: target && target.readOnly === true
    readonly property bool hasSelection:
        target && target.selectedText !== undefined && target.selectedText.length > 0

    // 菜单项按模式生成：只读态干脆没有剪切/粘贴，避免占位留白
    readonly property var menuActions: root.isReadOnly
        ? [{ action: "copy", text: "复制" },
           { action: "selectAll", text: "全选" }]
        : [{ action: "copy", text: "复制" },
           { action: "cut", text: "剪切" },
           { action: "paste", text: "粘贴" },
           { action: "selectAll", text: "全选" }]

    // 右键点的位置（相对宿主控件），菜单就弹在这里
    property point pressPoint: Qt.point(0, 0)

    function runAction(action) {
        const host = root.target
        if (!host)
            return

        if (action === "copy")
            host.copy()
        else if (action === "cut")
            host.cut()
        else if (action === "paste")
            host.paste()
        else if (action === "selectAll")
            host.selectAll()
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: (point, button) => {
            root.pressPoint = point.position

            // 菜单按需创建：聊天页一屏就有几十个气泡，常驻几个菜单项太浪费
            menuLoader.active = true
            const menu = menuLoader.item
            if (!menu)
                return

            // 不用判 opened 再 close：菜单自己的 CloseOnPressOutside 会在按下时先关掉，
            // 而 TapHandler 是抬起时触发，到这里旧菜单已经关了。
            // （也避免在 Loader.item 这种 QObject 静态类型上访问 opened 触发的 lint 告警。）
            menu.parent = root
            menu.x = root.pressPoint.x
            menu.y = root.pressPoint.y
            menu.open()
        }
    }

    Loader {
        id: menuLoader
        active: false
        sourceComponent: menuComponent
    }

    Component {
        id: menuComponent

        Menu {
            // Repeater 填充 Menu 是可用的（Instantiator 不行：实测 count 恒为 0）
            Repeater {
                model: root.menuActions

                MenuItem {
                    required property var modelData

                    text: modelData.text
                    // 复制/剪切要有选中内容；粘贴和全选恒可用
                    enabled: (modelData.action === "copy" || modelData.action === "cut")
                             ? root.hasSelection : true
                    onTriggered: root.runAction(modelData.action)
                }
            }
        }
    }
}
