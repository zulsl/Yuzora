import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 预设的提示词条目列表
// 职责：条目的新增/插入/删除，以及名称、角色、位置、启用开关和正文编辑。
// 展开行复用 ExpandableRow（与世界书条目列表同一套实现）。
ColumnLayout {
    id: root

    property string pendingDeleteId: ""
    property string pendingDeleteName: ""
    property int pendingDeleteIndex: -1

    spacing: Theme.spaceSm

    // 菜单里的上移/下移/置顶/置底：挪模型后立刻落库（拖动是松手才落库，这里是一次性动作）
    function moveRowTo(from, to) {
        if (from < 0 || from === to)
            return
        const last = promptList.count - 1
        const target = Math.max(0, Math.min(last, to))
        if (target === from)
            return
        CompletionPresetViewModel.movePrompt(from, target)
        CompletionPresetViewModel.commitPromptOrder()
        promptList.currentIndex = target
        promptList.positionViewAtIndex(target, ListView.Contain)
    }

    // ---------- 工具栏 ----------
    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.spaceSm

        AppButton {
            text: "添加"
            iconSource: "qrc:/icons/res/plus.svg"
            onClicked: {
                CompletionPresetViewModel.addPrompt()
                promptList.currentIndex = promptList.count - 1
                if (promptList.currentIndex >= 0)
                    promptList.positionViewAtEnd()
            }
        }

        AppButton {
            text: "插入"
            enabled: promptList.currentIndex >= 0
            onClicked: {
                if (!promptList.currentItem)
                    return

                const insertIndex = promptList.currentIndex
                const beforeId = promptList.currentItem.promptIdentifier
                CompletionPresetViewModel.insertPromptBefore(beforeId)
                promptList.currentIndex = Math.min(insertIndex, promptList.count - 1)
                if (promptList.currentIndex >= 0)
                    promptList.positionViewAtIndex(promptList.currentIndex, ListView.Contain)
            }
        }

        Item { Layout.fillWidth: true }

        Label {
            text: "提示词修改自动保存"
            font.pixelSize: Theme.fontBodySm
            font.family: Theme.fontFamily
            color: Theme.textSecondary
        }
    }

    // ---------- 列表 ----------
    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true

        ListView {
            id: promptList
            anchors.fill: parent
            clip: true
            spacing: Theme.spaceXs
            model: CompletionPresetViewModel.presetPromptModel
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            // ---------- 拖动换位置的状态 ----------
            // 交互：行头按住 400ms 起拖；桌面端额外支持"按住移动直接拖"。
            // 拖动过程中实时重排模型（其余行让位动画），松手才落库。
            property bool dragging: false
            property int dragIndex: -1
            property int pressIndex: -1
            property real pressViewY: 0
            property real pressOffset: 0
            property real ghostY: 0
            property real lastViewY: 0
            property real autoScrollSpeed: 0
            property string dragName: ""
            // 起拖时展开行全部收起，因此每行等高，落点可以按固定步长算
            readonly property real rowPitch: Theme.touchTarget + spacing
            readonly property bool desktop: Qt.platform.os !== "android"
                                            && Qt.platform.os !== "ios"

            // 记录"按在哪一行、按在行内哪个高度"，拖动手柄和行头都用这一套
            function pressRow(row, localY, viewY, name) {
                pressIndex = row
                pressOffset = localY
                pressViewY = viewY
                dragName = name
            }

            function releaseRow() {
                holdTimer.stop()
                if (dragging)
                    endDrag()
            }

            function beginDrag() {
                if (dragging || pressIndex < 0 || pressIndex >= count)
                    return

                dragging = true
                dragIndex = pressIndex
                ghostY = Math.max(0, Math.min(height - Theme.touchTarget,
                                              pressViewY - pressOffset))
                lastViewY = ghostY + pressOffset
                interactive = false
                // 展开的行会撑高行高，让位和落点都算不准，起拖先全部收起
                CompletionPresetViewModel.presetPromptModel.collapseAll()
            }

            function updateDrag(viewY) {
                if (!dragging)
                    return

                lastViewY = viewY
                ghostY = Math.max(0, Math.min(height - Theme.touchTarget, viewY - pressOffset))

                // 靠边自动滚：越靠边滚得越快
                const edge = 60
                const maxY = height - Theme.touchTarget
                if (ghostY < edge)
                    autoScrollSpeed = -Math.max(2, (edge - ghostY) / 4)
                else if (ghostY > maxY - edge)
                    autoScrollSpeed = Math.max(2, (ghostY - (maxY - edge)) / 4)
                else
                    autoScrollSpeed = 0

                const contentCenter = ghostY + contentY + Theme.touchTarget / 2
                const target = Math.max(0, Math.min(count - 1,
                                                    Math.floor(contentCenter / rowPitch)))
                if (target !== dragIndex) {
                    CompletionPresetViewModel.movePrompt(dragIndex, target)
                    dragIndex = target
                }
            }

            function endDrag() {
                if (!dragging)
                    return

                holdTimer.stop()
                dragging = false
                interactive = true
                autoScrollSpeed = 0
                pressIndex = -1
                dragName = ""
                if (dragIndex >= 0)
                    CompletionPresetViewModel.commitPromptOrder()
                dragIndex = -1
            }

            Timer {
                id: holdTimer
                interval: 400
                repeat: false
                onTriggered: promptList.beginDrag()
            }

            // 拖到列表顶端/底端时自动滚动，否则长列表根本拖不到远处
            Timer {
                id: autoScrollTimer
                interval: 16
                repeat: true
                running: promptList.dragging && promptList.autoScrollSpeed !== 0
                onTriggered: {
                    const maxContentY = Math.max(0, promptList.contentHeight - promptList.height)
                    promptList.contentY = Math.max(0, Math.min(maxContentY,
                                                               promptList.contentY + promptList.autoScrollSpeed))
                    promptList.updateDrag(promptList.lastViewY)
                }
            }

            // 重排时其他行平滑让位
            displaced: Transition {
                NumberAnimation {
                    properties: "y"
                    duration: Theme.durationFast
                    easing.type: Easing.OutCubic
                }
            }

            // ---------- 展开后的详情 ----------
            Component {
                id: promptDetail

                ColumnLayout {
                    // 实例根节点必须有自己的 id：组件内部要用它取 bodyData。
                    // 不能写 detailRoot.bodyData —— promptDetail 是 Component 对象，没有这个属性，
                    // 那样取到的永远是 undefined（提示词正文会显示为空）。
                    id: detailRoot

                    property var bodyData

                    // 占位提示词（marker）：正文由运行时按角色卡/世界书/历史填充，
                    // 用户改了也没用（下次请求会被覆盖），所以整块内容只读。
                    readonly property bool isMarker: bodyData ? bodyData.marker === true : false

                    spacing: Theme.spaceSm

                    LabeledField {
                        label: "角色"
                        ComboBox {
                            Layout.fillWidth: true
                            implicitHeight: Theme.controlMd
                            model: ["系统", "用户", "AI 助手"]
                            currentIndex: detailRoot.bodyData ? detailRoot.bodyData.role : 0
                            onActivated: (index) => {
                                if (detailRoot.bodyData)
                                    CompletionPresetViewModel.setPromptField(
                                        detailRoot.bodyData.id, "role", index)
                            }
                        }
                    }

                    LabeledField {
                        label: "位置"
                        ComboBox {
                            Layout.fillWidth: true
                            implicitHeight: Theme.controlMd
                            model: ["相对", "聊天中"]
                            currentIndex: detailRoot.bodyData ? detailRoot.bodyData.position : 0
                            onActivated: (index) => {
                                if (detailRoot.bodyData)
                                    CompletionPresetViewModel.setPromptField(
                                        detailRoot.bodyData.id, "position", index)
                            }
                        }
                    }

                    Label {
                        text: detailRoot.isMarker ? "系统占位，不可编辑" : "提示词内容"
                        font.pixelSize: Theme.fontBodySm
                        font.family: Theme.fontFamily
                        color: detailRoot.isMarker ? Theme.textDisabled : Theme.textSecondary
                    }

                    TextArea {
                        id: contentArea
                        visible: !detailRoot.isMarker
                        TextContextMenu {}
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.max(Theme.controlLg * 5,
                                                         contentHeight + topPadding + bottomPadding)
                        textFormat: TextEdit.PlainText
                        wrapMode: TextArea.Wrap
                        placeholderText: "提示词"
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily
                        color: Theme.textPrimary
                        text: detailRoot.bodyData ? (detailRoot.bodyData.content || "") : ""
                        onTextChanged: {
                            if (activeFocus && detailRoot.bodyData)
                                CompletionPresetViewModel.setPromptField(detailRoot.bodyData.id,
                                                                         "content", text)
                        }
                    }
                }
            }

            // ---------- 条目行 ----------
            delegate: Item {
                id: promptDelegate

                width: ListView.view.width
                height: rowItem.implicitHeight

                // 暴露给“插入”工具栏按钮，用于定位当前选中项
                property string promptIdentifier: model.identifier
                // 行号会随重排变化，所以每次都从 index 取，别缓存
                property int rowIndex: index
                property string rowId: model.id
                property string rowName: model.name || ""
                property bool rowIsSystem: model.systemPrompt === true
                // 正在被拖的行留成空位：幽灵行跟手指走，这一行原地消失
                opacity: promptList.dragging && promptList.dragIndex === index ? 0 : 1

                // 行头按压区：点一下选中；按住 400ms 起拖。
                // z:-1 压在 ExpandableRow 下面，输入框/按钮照常优先拿到事件。
                MouseArea {
                    id: headerArea
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: rowItem.headerHeight
                    z: -1

                    onPressed: (mouse) => {
                        promptList.currentIndex = index
                        promptList.pressIndex = index
                        promptList.pressOffset = mouse.y
                        promptList.dragName = model.name || ""
                        const p = headerArea.mapToItem(promptList, mouse.x, mouse.y)
                        promptList.pressViewY = p.y
                        holdTimer.start()
                    }

                    onPositionChanged: (mouse) => {
                        if (!headerArea.pressed)
                            return

                        const p = headerArea.mapToItem(promptList, mouse.x, mouse.y)
                        if (promptList.dragging) {
                            promptList.updateDrag(p.y)
                            return
                        }

                        // 还没起拖就动了：这是在滚动列表，取消长按
                        if (Math.abs(p.y - promptList.pressViewY) > 10)
                            holdTimer.stop()

                        // 桌面端没有"拖动=滚动"的冲突，按住移动就能直接拖
                        if (promptList.desktop && !holdTimer.running
                                && Math.abs(p.y - promptList.pressViewY) > 8)
                            promptList.beginDrag()
                    }

                    onReleased: {
                        holdTimer.stop()
                        if (promptList.dragging)
                            promptList.endDrag()
                    }

                    onCanceled: {
                        holdTimer.stop()
                        if (promptList.dragging)
                            promptList.endDrag()
                    }
                }

                ExpandableRow {
                    id: rowItem
                    width: parent.width
                    headerHeight: Theme.touchTarget
                    expanded: model.expandedState
                    bodyComponent: promptDetail
                    bodyData: model

                    // 拖动手柄：名称是输入框，长按会被输入框吃掉（真机实测过），
                    // 所以拖动入口做成手柄——按在它上面一动就起拖，同时填饱 48dp 触摸目标。
                    Item {
                        id: dragHandle
                        implicitWidth: Theme.controlMd
                        implicitHeight: rowItem.headerHeight

                        IconButton {
                            anchors.centerIn: parent
                            compact: true
                            tip: "按住拖动调整顺序"
                            icon.source: "qrc:/icons/res/menu-order.svg"
                        }

                        MouseArea {
                            id: handleArea
                            anchors.fill: parent
                            cursorShape: Qt.SizeVerCursor

                            onPressed: (mouse) => {
                                promptList.currentIndex = index
                                const p = handleArea.mapToItem(promptList, mouse.x, mouse.y)
                                promptList.pressRow(index, mouse.y, p.y, model.name)
                            }

                            onPositionChanged: (mouse) => {
                                if (!handleArea.pressed)
                                    return
                                const p = handleArea.mapToItem(promptList, mouse.x, mouse.y)
                                if (!promptList.dragging) {
                                    // 手柄上没有"拖动=滚动"的歧义，动一点点就起拖
                                    if (Math.abs(p.y - promptList.pressViewY) < 3)
                                        return
                                    promptList.beginDrag()
                                }
                                promptList.updateDrag(p.y)
                            }

                            onReleased: promptList.releaseRow()
                            onCanceled: promptList.releaseRow()
                        }
                    }

                    IconButton {
                        compact: true
                        tip: model.expandedState ? "收起" : "展开"
                        icon.source: model.expandedState
                                     ? "qrc:/icons/res/circle-chevron-up.svg"
                                     : "qrc:/icons/res/circle-chevron-down.svg"
                        onClicked: {
                            promptList.currentIndex = index
                            model.expandedState = !model.expandedState
                        }
                    }

                    TextField {
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlSm
                        text: model.name || ""
                        TextContextMenu {}
                        placeholderText: "提示词名称"
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily
                        onTextEdited: CompletionPresetViewModel.setPromptField(
                                          model.id, "name", text)
                        onActiveFocusChanged: {
                            if (activeFocus)
                                promptList.currentIndex = index
                        }
                    }

                    IconButton {
                        id: moreButton
                        compact: true
                        tip: "更多"
                        icon.source: "qrc:/icons/res/dots-vertical.svg"
                        onClicked: {
                            promptList.currentIndex = index
                            rowMenu.open()
                        }
                    }

                    Switch {
                        checked: model.isActive
                        onToggled: {
                            promptList.currentIndex = index
                            CompletionPresetViewModel.setPromptField(model.id, "isActive", checked)
                        }
                    }
                }

                // 位置与删除都收进溢出菜单：行头少一个常驻图标，手机端更宽松，
                // 删除也变成两步操作，少一半误触。
                Menu {
                    id: rowMenu
                    x: Math.max(Theme.spaceSm, promptDelegate.width - width - Theme.spaceSm)
                    y: Theme.touchTarget

                    MenuItem {
                        text: "上移"
                        enabled: promptDelegate.rowIndex > 0
                        onTriggered: root.moveRowTo(promptDelegate.rowIndex,
                                                    promptDelegate.rowIndex - 1)
                    }
                    MenuItem {
                        text: "下移"
                        enabled: promptDelegate.rowIndex < promptList.count - 1
                        onTriggered: root.moveRowTo(promptDelegate.rowIndex,
                                                    promptDelegate.rowIndex + 1)
                    }
                    MenuItem {
                        text: "置顶"
                        enabled: promptDelegate.rowIndex > 0
                        onTriggered: root.moveRowTo(promptDelegate.rowIndex, 0)
                    }
                    MenuItem {
                        text: "置底"
                        enabled: promptDelegate.rowIndex < promptList.count - 1
                        onTriggered: root.moveRowTo(promptDelegate.rowIndex,
                                                    promptList.count - 1)
                    }
                    MenuSeparator {}
                    MenuItem {
                        text: "删除"
                        enabled: !promptDelegate.rowIsSystem
                        onTriggered: {
                            root.pendingDeleteId = promptDelegate.rowId
                            root.pendingDeleteName = promptDelegate.rowName
                            root.pendingDeleteIndex = promptDelegate.rowIndex
                            deletePromptDialog.open()
                        }
                    }
                }
            }
        }

        // 拖动时跟着手指走的“幽灵行”：被拖的那一行自己已隐藏，空位就是落点
        Rectangle {
            id: dragGhost
            visible: promptList.dragging
            x: 0
            y: promptList.ghostY
            width: promptList.width
            height: Theme.touchTarget
            z: 20
            radius: Theme.radiusSm
            color: Theme.bgElevated
            border.width: 1
            border.color: Theme.borderStrong
            opacity: 0.97
            scale: 1.02

            Label {
                anchors.fill: parent
                anchors.leftMargin: Theme.spaceSm
                anchors.rightMargin: Theme.spaceSm
                verticalAlignment: Text.AlignVCenter
                text: promptList.dragName
                elide: Text.ElideRight
                font.pixelSize: Theme.fontBody
                font.family: Theme.fontFamily
                color: Theme.textPrimary
            }
        }

        Text {
            anchors.centerIn: parent
            visible: promptList.count === 0 && !promptList.dragging
            text: "暂无提示词"
            font.pixelSize: Theme.fontBodySm
            font.family: Theme.fontFamily
            color: Theme.textSecondary
        }
    }

    ConfirmDialog {
        id: deletePromptDialog
        title: "确认删除"
        text: root.pendingDeleteName.length > 0
              ? "确定要删除提示词“" + root.pendingDeleteName + "”吗？此操作不可撤销。"
              : "确定要删除该提示词吗？此操作不可撤销。"
        acceptText: "删除"
        danger: true

        onConfirmed: {
            if (root.pendingDeleteId.length === 0)
                return

            CompletionPresetViewModel.deletePrompt(root.pendingDeleteId)
            promptList.currentIndex = Math.min(root.pendingDeleteIndex,
                                               promptList.count - 1)
            if (promptList.currentIndex >= 0)
                promptList.positionViewAtIndex(promptList.currentIndex, ListView.Contain)
        }
    }
}
