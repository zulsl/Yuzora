import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 预设编辑表单
// 职责：编辑当前选中的预设（基础参数 / 提示词条目两个 Tab）。
//       基础参数先落在本地副本，点"保存参数"提交；提示词条目沿用即时保存。
// 根节点用 Item 包一层，便于被 AdaptiveSplit 用 LayoutItemProxy 搬运。
Item {
    id: root

    implicitWidth: 480
    implicitHeight: 600

    property completionPresetEntity editingPreset: CompletionPresetViewModel.currentPreset

    signal exportRequested()

    Component.onCompleted: {
        tabBar.currentIndex = CompletionPresetViewModel.presetDetailTab
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spaceSm

        // ========== 固定操作栏 ==========
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm

            AppButton {
                text: "设为启用"
                variant: "secondary"
                enabled: root.editingPreset && !root.editingPreset.isActive
                onClicked: {
                    CompletionPresetViewModel.setActivePreset(root.editingPreset.id)
                    Ui.toast("已设为启用预设", false)
                }
            }

            AppButton {
                text: "导出预设"
                variant: "secondary"
                // iconSource: "qrc:/icons/res/file-export.svg"
                onClicked: root.exportRequested()
            }

            Item { Layout.fillWidth: true }

            AppButton {
                text: "删除预设"
                variant: "danger"
                enabled: root.editingPreset && !root.editingPreset.isActive
                onClicked: deleteDialog.open()
            }
        }

        // ========== Tab 切换 ==========
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.controlLg
            padding: 0
            spacing: 0

            background: Rectangle {
                color: "transparent"
            }

            TabButton {
                id: basicTabButton
                height: tabBar.height
                leftPadding: Theme.spaceMd
                rightPadding: Theme.spaceMd
                text: "基础参数"

                background: Item {
                    Rectangle {
                        anchors.fill: parent
                        anchors.topMargin: Theme.spaceXs
                        anchors.bottomMargin: Theme.spaceXs
                        radius: Theme.radiusSm
                        color: basicTabButton.hovered && tabBar.currentIndex !== 0
                               ? Theme.rowHover : "transparent"
                    }
                }

                contentItem: Text {
                    text: basicTabButton.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: Theme.fontBody
                    font.family: Theme.fontFamily
                    color: tabBar.currentIndex === 0 ? Theme.primaryColor : Theme.textSecondary
                }
            }

            TabButton {
                id: promptTabButton
                height: tabBar.height
                leftPadding: Theme.spaceMd
                rightPadding: Theme.spaceMd
                text: "提示词条目"

                background: Item {
                    Rectangle {
                        anchors.fill: parent
                        anchors.topMargin: Theme.spaceXs
                        anchors.bottomMargin: Theme.spaceXs
                        radius: Theme.radiusSm
                        color: promptTabButton.hovered && tabBar.currentIndex !== 1
                               ? Theme.rowHover : "transparent"
                    }
                }

                contentItem: Text {
                    text: promptTabButton.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: Theme.fontBody
                    font.family: Theme.fontFamily
                    color: tabBar.currentIndex === 1 ? Theme.primaryColor : Theme.textSecondary
                }
            }

            onCurrentIndexChanged: {
                CompletionPresetViewModel.presetDetailTab = currentIndex
                tabBar.forceActiveFocus()
            }
        }

        Connections {
            target: CompletionPresetViewModel
            function onPresetDetailTabChanged() {
                tabBar.currentIndex = CompletionPresetViewModel.presetDetailTab
            }
        }

        StackLayout {
            id: contentStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // ========== 基础参数 ==========
            Item {
                id: basicTab

                ScrollView {
                    id: basicScroll
                    anchors.fill: parent
                    contentWidth: availableWidth
                    clip: true

                    ColumnLayout {
                        // 给右侧滚动条留出空间，避免滚动条压住输入框
                        width: basicScroll.availableWidth - Theme.spaceMd
                        spacing: Theme.sectionGap

                        SectionCard {
                            title: "基础设置"
                            Layout.fillWidth: true

                            LabeledField {
                                label: "预设名称"
                                TextField {
                                    Layout.fillWidth: true
                                    implicitHeight: Theme.controlMd
                                    id: nameField
                                    text: root.editingPreset.name
                                    TextContextMenu {}
                                    font.pixelSize: Theme.fontBody
                                    font.family: Theme.fontFamily
                                    onTextEdited: CompletionPresetViewModel.renamePreset(root.editingPreset.id,text)
                                }
                            }

                            LabeledField {
                                label: "最大上下文(按Token计数)"
                                SpinBox {
                                    Layout.fillWidth: true
                                    implicitHeight: Theme.controlMd
                                    from: 512
                                    to: 2000000
                                    stepSize: 512
                                    editable: true
                                    id: maxContextSpin
                                    value: root.editingPreset.maxContextToken
                                    onValueModified: CompletionPresetViewModel.setMaxContextToken(value)
                                }
                            }

                            LabeledField {
                                label: "最大回复(按Token计数)"
                                SpinBox {
                                    Layout.fillWidth: true
                                    implicitHeight: Theme.controlMd
                                    from: 64
                                    to: 128000
                                    stepSize: 64
                                    editable: true
                                    id: maxResponseSpin
                                    value: root.editingPreset.maxResponseToken || 1024
                                    onValueModified: CompletionPresetViewModel.setMaxResponseToken(value)
                                }
                            }

                            LabeledField {
                                label: "流式输出"
                                Switch {
                                    Layout.alignment: Qt.AlignLeft
                                    id: streamSwitch
                                    checked: root.editingPreset.streamEnabled === true
                                    onToggled: CompletionPresetViewModel.setStreamEnabled(checked)
                                }
                            }
                        }

                        SectionCard {
                            title: "采样参数"
                            Layout.fillWidth: true

                            SliderFloatInput {
                                Layout.fillWidth: true
                                labelText: "温度（创造力，写作推荐 1.0）"
                                from: 0.0
                                to: 2.0
                                value: root.editingPreset.temperature
                                onEdited: CompletionPresetViewModel.setTemperature(value)
                            }

                            //不再支持
                            // SliderFloatInput {
                            //     Layout.fillWidth: true
                            //     labelText: "频率惩罚"
                            //     from: -2.0
                            //     to: 2.0
                            //     value: //
                            //     onEdited: //
                            // }
                            //
                            // SliderFloatInput {
                            //     Layout.fillWidth: true
                            //     labelText: "存在惩罚"
                            //     from: -2.0
                            //     to: 2.0
                            //     value:
                            //     onEdited:
                            // }

                            SliderFloatInput {
                                Layout.fillWidth: true
                                labelText: "Top-P（词汇多样性，写作推荐 0.95）"
                                from: 0.0
                                to: 1.0
                                value: root.editingPreset.topP
                                onEdited: CompletionPresetViewModel.setTopP(value)
                            }

                            SliderFloatInput {
                                Layout.fillWidth: true
                                labelText: "Top-K（候选词数量，写作推荐 60）"
                                from: 0
                                to: 200
                                value: root.editingPreset.topK
                                onEdited: CompletionPresetViewModel.setTopK(value)
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }

            // ========== 提示词条目 ==========
            Item {
                id: promptTab

                PromptsListView {
                    anchors.fill: parent
                }
            }
        }
    }

    ConfirmDialog {
        id: deleteDialog
        title: "确认删除"
        text: "确定要删除该预设吗？此操作不可撤销。"
        acceptText: "删除"
        danger: true
        onConfirmed: CompletionPresetViewModel.deletePreset(root.editingPreset.id)
    }
}
