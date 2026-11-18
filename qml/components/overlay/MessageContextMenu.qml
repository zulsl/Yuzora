import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

import Yuzora 1.0

// 一条消息的气泡级菜单：文本操作 + 重新生成 / 删除。
// 放在气泡 Rectangle 里而不是 TextEdit 里，失败/取消的空气泡也能右键；
// 桌面右键、触屏长按共用同一个菜单，避免两层各弹一个。
Item {
    id: root

    anchors.fill: parent
    z: 3

    // 正文控件：复制/全选要作用在它上面（空状态下可以为 null）
    property var textTarget: null
    property bool hasText: false
    property bool canRegenerate: false
    property bool canDelete: false
    // 生成中动作置灰：菜单还能打开（文本操作可用），但删除/重新生成不可点
    property bool actionsEnabled: true

    signal regenerateRequested()
    signal deleteRequested()

    readonly property bool hasSelection: textTarget
            && textTarget.selectedText !== undefined
            && textTarget.selectedText.length > 0

    readonly property var textActions: hasText
        ? [{ action: "copy", text: "复制", enabled: hasSelection },
           { action: "selectAll", text: "全选", enabled: true }]
        : []

    readonly property var messageActions: {
        const actions = []
        if (canRegenerate)
            actions.push({ action: "regenerate", text: "重新生成", danger: false,
                           enabled: root.actionsEnabled })
        if (canDelete)
            actions.push({ action: "delete", text: "删除", danger: true,
                           enabled: root.actionsEnabled })
        return actions
    }

    readonly property bool hasAnyAction: textActions.length > 0 || messageActions.length > 0

    function openMenu(point) {
        // 什么都不给（比如生成中的空占位）：没有可执行项就不弹空菜单
        if (!hasAnyAction)
            return
        // 按需创建：一屏几十条消息，常驻菜单项太浪费
        menuLoader.active = true
        const menu = menuLoader.item
        if (!menu)
            return

        menu.parent = root
        // 靠近右下角时往里收，避免菜单被窗口裁掉
        menu.x = Math.max(0, Math.min(point.x, root.width - menu.implicitWidth))
        menu.y = Math.max(0, Math.min(point.y, root.height - menu.implicitHeight))
        menu.open()
    }

    function runTextAction(action) {
        if (!textTarget)
            return
        if (action === "copy")
            textTarget.copy()
        else if (action === "selectAll")
            textTarget.selectAll()
    }

    // 桌面：右键。
    // Qt 6.8 的 tapped 信号是 (QEventPoint, Qt::MouseButton)，
    // 第一个参数自己就有 position，没有 .point 这一层。
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: (point, button) => root.openMenu(point.position)
    }

    // 触屏：长按。桌面禁用，避免按住左键拖选文本时误触。
    // TapHandler 的 longPressed() 没有参数，位置要用处理器自己的 point。
    TapHandler {
        id: longPressHandler
        acceptedButtons: Qt.LeftButton
        enabled: Theme.isTouchPrimary
        longPressThreshold: 0.4
        onLongPressed: root.openMenu(longPressHandler.point.position)
    }

    Loader {
        id: menuLoader
        active: false
        sourceComponent: menuComponent
    }

    Component {
        id: menuComponent

        Menu {
            Repeater {
                model: root.textActions
                delegate: MenuItem {
                    required property var modelData
                    text: modelData.text
                    enabled: modelData.enabled
                    onClicked: root.runTextAction(modelData.action)
                }
            }

            // 只有同时存在文本操作与消息操作时才插分隔线
            Repeater {
                model: (root.hasText && (root.canRegenerate || root.canDelete)) ? 1 : 0
                delegate: MenuSeparator {}
            }

            Repeater {
                model: root.messageActions
                delegate: MenuItem {
                    required property var modelData
                    text: modelData.text
                    enabled: modelData.enabled
                    // Material 样式下让"删除"的文字变危险色
                    Material.foreground: modelData.danger ? Theme.danger : Theme.textPrimary
                    onClicked: {
                        if (modelData.action === "regenerate")
                            root.regenerateRequested()
                        else if (modelData.action === "delete")
                            root.deleteRequested()
                    }
                }
            }
        }
    }
}
