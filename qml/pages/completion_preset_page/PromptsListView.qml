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

            // ---------- 展开后的详情 ----------
            Component {
                id: promptDetail

                ColumnLayout {
                    // 实例根节点必须有自己的 id：组件内部要用它取 bodyData。
                    // 不能写 detailRoot.bodyData —— promptDetail 是 Component 对象，没有这个属性，
                    // 那样取到的永远是 undefined（提示词正文会显示为空）。
                    id: detailRoot

                    property var bodyData

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
                        text: "提示词内容"
                        font.pixelSize: Theme.fontBodySm
                        font.family: Theme.fontFamily
                        color: Theme.textSecondary
                    }

                    TextArea {
                        id: contentArea
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

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: rowItem.headerHeight
                    z: -2
                    radius: Theme.radiusSm
                    color: promptList.currentIndex === index ? Theme.rowHover : "transparent"
                }

                // 点击行空白处选中；输入框/按钮自己处理点击，另外补选中
                MouseArea {
                    anchors.fill: parent
                    z: -1
                    onClicked: promptList.currentIndex = index
                }

                ExpandableRow {
                    id: rowItem
                    width: parent.width
                    headerHeight: Theme.touchTarget
                    expanded: model.expandedState
                    bodyComponent: promptDetail
                    bodyData: model

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
                        compact: true
                        danger: true
                        enabled: !model.systemPrompt
                        tip: model.systemPrompt ? "系统内置提示词不可删除" : "删除提示词"
                        icon.source: "qrc:/icons/res/trash.svg"
                        onClicked: {
                            promptList.currentIndex = index
                            root.pendingDeleteId = model.id
                            root.pendingDeleteName = model.name
                            root.pendingDeleteIndex = index
                            deletePromptDialog.open()
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
            }
        }

        Text {
            anchors.centerIn: parent
            visible: promptList.count === 0
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
