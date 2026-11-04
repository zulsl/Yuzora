import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8
import QtQuick.Dialogs 6.8
import QtQuick.Controls.Material 6.8
import Yuzora 1.0

// ==================== 主界面 ====================
Page {
    property completionPresetEntity editingPreset : CompletionPresetViewModel.currentEditingPreset

    id: root
    padding: 12
    header: MyToolBar{
        title:"对话补全预设"
        onLeftClicked: sideDrawer.open()
    }


    RowLayout{
        anchors.fill: parent
        // ========== 左侧布局 ==========
        ColumnLayout{
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            spacing: 0
            // 操作栏
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                ToolButton {
                    text: "新建预设"
                    onClicked: {
                        CompletionPresetViewModel.createPreset()
                    }
                }

                ToolButton {
                    icon.source: "qrc:/icons/res/file-import.svg"
                    onClicked: importDialog.open()

                    hoverEnabled: true
                    ToolTip.text:"导入预设"
                    ToolTip.visible: hovered
                }

                ToolButton {

                    icon.source: "qrc:/icons/res/file-export.svg"
                    onClicked: exportAllDialog.open()

                    hoverEnabled: true
                    ToolTip.text:"导出预设"
                    ToolTip.visible: hovered
                }
            }

            // ---------- 预设列表 ----------
            ListView {
                id: presetListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: CompletionPresetViewModel ? CompletionPresetViewModel.presetModel : null
                currentIndex: -1

                delegate: ItemDelegate {
                    width: presetListView.width
                    text: model.name
                    highlighted: ListView.isCurrentItem
                    onClicked: {
                        presetListView.currentIndex = index
                        CompletionPresetViewModel.selectPreset(model.id)
                    }

                    rightPadding: 8
                    contentItem: RowLayout {
                        spacing: 8

                        Label {
                            text: model.name
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Label {
                            visible: model.isDefault
                            text: "启用"
                            font.pixelSize: 11
                            color: "#2e7d32"
                            padding: 2
                            background: Rectangle {
                                color: "#e8f5e9"
                                radius: 3
                            }
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
            }

        }

        // ========== 右侧布局 ==========
        Pane{
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 3

            // 未选中状态占位
            Column {
                anchors.centerIn: parent
                visible: !CompletionPresetViewModel || !CompletionPresetViewModel.hasSelection
                spacing: 8

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "请选择一个预设进行编辑"
                    color: palette.mid
                    font.pixelSize: 14
                }
            }

            // 选中后显示表单
            ScrollView {
                id:scrollView
                anchors.fill: parent
                visible: CompletionPresetViewModel && CompletionPresetViewModel.hasSelection
                contentWidth: availableWidth
                spacing: 12
                clip: true
                rightPadding:20
                ColumnLayout {
                    width: scrollView.availableWidth
                    // anchors.fill: parent
                    spacing: 16

                    // 页面操作按钮
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Button {
                            text: "设为启用"
                            icon.name: "emblem-default"
                            enabled: !CompletionPresetViewModel.currentEditingPreset.isDefault
                            onClicked: CompletionPresetViewModel.setActivePreset(CompletionPresetViewModel.currentPreset.id)
                        }

                        // Button {text: "复制预设"}

                        Button {
                            text: "导出预设"
                            icon.source: "qrc:/icons/res/file-export.svg"
                            icon.width: 30
                            icon.height: 30
                            onClicked: {
                                exportSingleDialog.presetId = CompletionPresetViewModel.currentEditingPreset.id
                                exportSingleDialog.open()
                            }
                        }

                        Item { Layout.fillWidth: true }

                        // Button {text: "取消修改";onClicked: onCurrentPresetChanged()}

                        Button {
                            text: "删除"
                            icon.name: "edit-delete"
                            enabled: !CompletionPresetViewModel.currentEditingPreset.isDefault
                            onClicked: {
                                if (confirmDeleteDialog.visible) return
                                confirmDeleteDialog.open()
                            }
                        }

                        Button {
                            text: "保存修改"
                            icon.name: "document-save"
                            highlighted: true
                            onClicked: CompletionPresetViewModel.applyEditingPreset(editingPreset)
                        }
                    }

                    // 基础设置分组
                    GroupBox {
                        title: "基础设置"
                        Layout.fillWidth: true
                        ColumnLayout  {
                            width: parent.width

                            Label { text: "预设名称" }
                            TextField {
                                id: nameField
                                Layout.fillWidth: true
                                text: editingPreset.name
                                onTextEdited: editingPreset.name = text
                            }

                            Label { text: "最大上下文长度(Token)" }
                            SpinBox {
                                id: contextTokenField
                                Layout.fillWidth: true
                                from: 512//最小值
                                to: 2000000//最大值
                                editable:true//可编辑
                                stepSize: 512//步长

                                value: editingPreset.maxContextToken
                                onValueModified: editingPreset.maxContextToken = value
                            }

                            Label { text: "最大回复 Token" }
                            SpinBox {
                                id: responseTokenField
                                Layout.fillWidth: true
                                from: 64
                                to: 128000
                                editable:true
                                stepSize: 64

                                value: editingPreset.maxResponseToken
                                onValueModified: editingPreset.maxResponseToken = value
                            }

                            Label { text: "流式输出" }
                            Switch {
                                id: streamSwitch
                                checked: editingPreset.streamEnabled
                                onToggled: editingPreset.streamEnabled = checked
                            }
                        }
                    }

                    // 采样参数分组
                    GroupBox {
                        title: "采样参数"
                        Layout.fillWidth: true
                        ColumnLayout  {
                            anchors.fill: parent

                            SliderFloatInput {
                                labelText: "温度(创造力,写作推荐1.0)"
                                from:0.0
                                to:2.0

                                value:editingPreset.temperature
                                onEdited:editingPreset.temperature = value
                            }
                            SliderFloatInput {
                                labelText: "频率惩罚"
                                from:-2.0
                                to:2.0

                                value:editingPreset.frequencyPenalty
                                onEdited: editingPreset.frequencyPenalty=value
                            }
                            SliderFloatInput {
                                labelText: "存在惩罚"
                                from:-2.0
                                to:2.0

                                value:editingPreset.presencePenalty
                                onEdited: editingPreset.presencePenalty=value
                            }
                            SliderFloatInput {
                                id: topPField
                                labelText: "Top-P(词汇多样性,写作推荐0.95)"
                                from:0.0
                                to:1.0

                                value:editingPreset.topP
                                onEdited:editingPreset.topP=value
                            }
                            SliderFloatInput {
                                id: topKField
                                labelText:"Top-K(候选词数量,写作推荐60)"
                                from: 0
                                to: 200

                                value: editingPreset.topK
                                onEdited: editingPreset.topK=value
                            }
                        }
                    }


                    //提示词列表
                    Label { text: "提示词" }
                    PromptsListView{}

                }
            }
        }
    }

    // ========== 对话框 ==========
    FileDialog {
        id: importDialog
        title: "导入预设文件"
        nameFilters: ["JSON 文件 (*.json)", "所有文件 (*)"]
        fileMode: FileDialog.OpenFile
        onAccepted: CompletionPresetViewModel.importFromFile(selectedFile)
    }

    FileDialog {
        id: exportSingleDialog
        property string presetId: ""
        title: "导出预设"
        nameFilters: ["JSON 文件 (*.json)"]
        fileMode: FileDialog.SaveFile
        onAccepted: CompletionPresetViewModel.exportCurrent(
            presetId, fileUrl.toString().replace("file://", ""))
    }

    MessageDialog {
        id: confirmDeleteDialog
        title: "确认删除"
        text: "确定要删除该预设吗？此操作不可撤销。"
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        onAccepted: CompletionPresetViewModel.deletePreset(CompletionPresetViewModel.currentPreset.id)
    }

    Connections {
        target: CompletionPresetViewModel

        function onOperationError(errorMsg) {
            errorToast.show(errorMsg)
            console.warn("操作错误:", errorMsg)
        }

        function onOperationTip(tipMsg) {
            tipToast.show(tipMsg)
            console.log(tipMsg)
        }
    }
}