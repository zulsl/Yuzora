import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 预设编辑表单
// 职责：编辑当前选中的预设（基础参数 + 采样参数 + 提示词列表），
//       修改先落在本地副本，点"保存修改"才整体提交。
// 根节点用 Item 包一层，便于被 AdaptiveSplit 用 LayoutItemProxy 搬运。
Item {
    id: root

    implicitWidth: 480
    implicitHeight: 600

    // Q_GADGET 值类型只能用 var 承接
    property var editingPreset: CompletionPresetViewModel.currentEditingPreset

    signal exportRequested()

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: Theme.sectionGap

            // ========== 操作栏 ==========
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                AppButton {
                    text: "设为启用"
                    variant: "secondary"
                    enabled: root.editingPreset && !root.editingPreset.isDefault
                    onClicked: {
                        CompletionPresetViewModel.setActivePreset(root.editingPreset.id)
                        Ui.toast("已设为启用预设", false)
                    }
                }

                AppButton {
                    text: "导出"
                    variant: "secondary"
                    iconSource: "qrc:/icons/res/file-export.svg"
                    onClicked: root.exportRequested()
                }

                Item { Layout.fillWidth: true }

                AppButton {
                    text: "删除"
                    variant: "danger"
                    enabled: root.editingPreset && !root.editingPreset.isDefault
                    onClicked: deleteDialog.open()
                }

                AppButton {
                    text: "保存修改"
                    variant: "primary"
                    onClicked: {
                        CompletionPresetViewModel.applyEditingPreset(root.editingPreset)
                        Ui.toast("预设已保存", false)
                    }
                }
            }

            // ========== 基础设置 ==========
            SectionCard {
                title: "基础设置"
                Layout.fillWidth: true

                LabeledField {
                    label: "预设名称"
                    TextField {
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        text: root.editingPreset ? (root.editingPreset.name || "") : ""
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily
                        onTextEdited: root.editingPreset.name = text
                    }
                }

                LabeledField {
                    label: "最大上下文"
                    SpinBox {
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        from: 512
                        to: 2000000
                        stepSize: 512
                        editable: true
                        value: root.editingPreset ? root.editingPreset.maxContextToken : 4096
                        onValueModified: root.editingPreset.maxContextToken = value
                    }
                }

                LabeledField {
                    label: "最大回复 Token"
                    SpinBox {
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        from: 64
                        to: 128000
                        stepSize: 64
                        editable: true
                        value: root.editingPreset ? root.editingPreset.maxResponseToken : 1024
                        onValueModified: root.editingPreset.maxResponseToken = value
                    }
                }

                LabeledField {
                    label: "流式输出"
                    Switch {
                        Layout.alignment: Qt.AlignLeft
                        checked: root.editingPreset ? root.editingPreset.streamEnabled : true
                        onToggled: root.editingPreset.streamEnabled = checked
                    }
                }
            }

            // ========== 采样参数 ==========
            SectionCard {
                title: "采样参数"
                Layout.fillWidth: true

                SliderFloatInput {
                    Layout.fillWidth: true
                    labelText: "温度（创造力，写作推荐 1.0）"
                    from: 0.0
                    to: 2.0
                    value: root.editingPreset ? root.editingPreset.temperature : 0.7
                    onEdited: root.editingPreset.temperature = value
                }

                SliderFloatInput {
                    Layout.fillWidth: true
                    labelText: "频率惩罚"
                    from: -2.0
                    to: 2.0
                    value: root.editingPreset ? root.editingPreset.frequencyPenalty : 0.0
                    onEdited: root.editingPreset.frequencyPenalty = value
                }

                SliderFloatInput {
                    Layout.fillWidth: true
                    labelText: "存在惩罚"
                    from: -2.0
                    to: 2.0
                    value: root.editingPreset ? root.editingPreset.presencePenalty : 0.0
                    onEdited: root.editingPreset.presencePenalty = value
                }

                SliderFloatInput {
                    Layout.fillWidth: true
                    labelText: "Top-P（词汇多样性，写作推荐 0.95）"
                    from: 0.0
                    to: 1.0
                    value: root.editingPreset ? root.editingPreset.topP : 0.9
                    onEdited: root.editingPreset.topP = value
                }

                SliderFloatInput {
                    Layout.fillWidth: true
                    labelText: "Top-K（候选词数量，写作推荐 60）"
                    from: 0
                    to: 200
                    value: root.editingPreset ? root.editingPreset.topK : 40
                    onEdited: root.editingPreset.topK = value
                }
            }

            // ========== 提示词 ==========
            SectionCard {
                title: "提示词条目"
                Layout.fillWidth: true

                PromptsListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Theme.controlLg * 8
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
