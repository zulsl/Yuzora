import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 预设的提示词条目列表
// 职责：条目的名称/角色/位置/启用开关，展开后编辑正文。
// 展开行复用 ExpandableRow（与世界书条目列表同一套实现）。
ListView {
    id: root

    clip: true
    spacing: Theme.spaceXs
    model: CompletionPresetViewModel.presetPromptModel
    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

    // ---------- 展开后的详情 ----------
    Component {
        id: promptDetail

        ColumnLayout {
            property var bodyData

            spacing: Theme.spaceSm

            Timer {
                id: debounceTimer
                interval: 300
                repeat: false
                onTriggered: {
                    if (promptDetail.bodyData)
                        promptDetail.bodyData.content = contentArea.text
                }
            }

            LabeledField {
                label: "角色"
                ComboBox {
                    Layout.fillWidth: true
                    implicitHeight: Theme.controlMd
                    model: ["系统", "用户", "AI 助手"]
                    currentIndex: promptDetail.bodyData ? promptDetail.bodyData.role : 0
                    onActivated: (index) => {
                        if (promptDetail.bodyData)
                            promptDetail.bodyData.role = index
                    }
                }
            }

            LabeledField {
                label: "位置"
                ComboBox {
                    Layout.fillWidth: true
                    implicitHeight: Theme.controlMd
                    model: ["相对", "聊天中"]
                    currentIndex: promptDetail.bodyData ? promptDetail.bodyData.position : 0
                    onActivated: (index) => {
                        if (promptDetail.bodyData)
                            promptDetail.bodyData.position = index
                    }
                }
            }

            Label {
                text: "提示词内容"
                font.pixelSize: Theme.fontBodySm
                font.family: Theme.fontFamily
                color: Theme.textSecondary
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.controlLg * 5
                clip: true

                TextArea {
                    id: contentArea
                    textFormat: TextEdit.PlainText
                    wrapMode: TextArea.Wrap
                    placeholderText: "提示词"
                    font.pixelSize: Theme.fontBody
                    font.family: Theme.fontFamily
                    color: Theme.textPrimary
                    text: promptDetail.bodyData ? (promptDetail.bodyData.content || "") : ""
                    onTextChanged: {
                        if (activeFocus)
                            debounceTimer.restart()
                    }
                }
            }
        }
    }

    // ---------- 条目行 ----------
    delegate: ExpandableRow {
        width: ListView.view.width
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
            onClicked: model.expandedState = !model.expandedState
        }

        TextField {
            Layout.fillWidth: true
            implicitHeight: Theme.controlSm
            text: model.name || ""
            placeholderText: "提示词名称"
            font.pixelSize: Theme.fontBody
            font.family: Theme.fontFamily
            onTextEdited: model.name = text
        }

        Switch {
            checked: model.isActive
            onToggled: model.isActive = checked
        }
    }
}
