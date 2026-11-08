import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 模型 API 配置页
// 职责：选择配置、编辑表单字段、拉取模型列表。
// 说明：改造前"测试连接"按钮整段被注释（保留功能），本次同样不启用。
PageScaffold {
    id: root

    title: "模型 API 配置"
    contentPadding: Theme.pagePadding
    onLeftClicked: Ui.requestDrawer()

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: Theme.sectionGap

            // ========== 连接配置 ==========
            SectionCard {
                title: "连接配置"
                Layout.fillWidth: true

                LabeledField {
                    label: "当前配置"

                    ComboBox {
                        id: configCombo
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        model: ApiConfigViewModel.configListModel
                        textRole: "name"
                        valueRole: "id"
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily

                        // qt6.8 不支持直接写 currentValue，用 Binding 同步选中项
                        Binding on currentIndex {
                            value: configCombo.indexOfValue(ApiConfigViewModel.currentConfigId)
                            when: configCombo.indexOfValue(ApiConfigViewModel.currentConfigId) !== -1
                        }

                        // 页面切换回来时 ComboBox 是新建的，Binding 的 when 初始可能为 false；
                        // 创建完成后补一次按 currentConfigId 定位，避免回到第 0 项。
                        Component.onCompleted: {
                            const idx = indexOfValue(ApiConfigViewModel.currentConfigId)
                            if (idx >= 0)
                                currentIndex = idx
                        }

                        onActivated: ApiConfigViewModel.selectConfig(currentValue)

                        contentItem: Row {
                            spacing: Theme.spaceSm
                            anchors.verticalCenter: parent.verticalCenter
                            leftPadding: Theme.spaceMd

                            Rectangle {
                                width: Theme.spaceSm
                                height: Theme.spaceSm
                                radius: width / 2
                                anchors.verticalCenter: parent.verticalCenter
                                color: ApiConfigViewModel.currentIsActive ? Theme.success
                                                                          : "transparent"
                                border.width: ApiConfigViewModel.currentIsActive ? 0 : 1
                                border.color: Theme.borderStrong
                            }

                            Text {
                                text: configCombo.displayText
                                font: configCombo.font
                                color: Theme.textPrimary
                                anchors.verticalCenter: parent.verticalCenter
                                elide: Text.ElideRight
                            }
                        }

                        delegate: ItemDelegate {
                            width: configCombo.width
                            highlighted: configCombo.highlightedIndex === index

                            contentItem: Row {
                                spacing: Theme.spaceSm
                                anchors.verticalCenter: parent.verticalCenter

                                Rectangle {
                                    width: Theme.spaceSm
                                    height: Theme.spaceSm
                                    radius: width / 2
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: model.isActive ? Theme.success : "transparent"
                                    border.width: model.isActive ? 0 : 1
                                    border.color: Theme.borderStrong
                                }

                                Text {
                                    text: model.name
                                    font.pixelSize: Theme.fontBody
                                    font.family: Theme.fontFamily
                                    color: Theme.textPrimary
                                    anchors.verticalCenter: parent.verticalCenter
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceSm

                    AppButton {
                        text: "设为当前使用"
                        variant: "secondary"
                        enabled: !ApiConfigViewModel.currentIsActive
                        onClicked: ApiConfigViewModel.applyCurrentConfig()
                    }

                    AppButton {
                        text: "删除"
                        variant: "danger"
                        visible: !ApiConfigViewModel.isCurrentBuiltin
                        onClicked: deleteDialog.open()
                    }

                    Item { Layout.fillWidth: true }

                    AppButton {
                        text: "添加来源"
                        variant: "secondary"
                        iconSource: "qrc:/icons/res/file-plus.svg"
                        onClicked: addModelDialog.open()
                    }
                }
            }

            // ========== 参数表单 ==========
            SectionCard {
                title: "参数"
                Layout.fillWidth: true
                enabled: configCombo.count > 0

                LabeledField {
                    label: "配置名称"
                    visible: !ApiConfigViewModel.isCurrentBuiltin
                    TextField {
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        text: ApiConfigViewModel.configName
                        placeholderText: "给这个 API 配置起个名字"
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily
                        onTextEdited: ApiConfigViewModel.setConfigName(text)
                    }
                }

                LabeledField {
                    label: "API 密钥"

                    TextField {
                        id: keyField
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        text: ApiConfigViewModel.apiKey
                        echoMode: TextInput.Password
                        placeholderText: "sk-*************"
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily
                        rightPadding: Theme.touchTarget
                        onTextEdited: ApiConfigViewModel.setApiKey(text)

                        ToolButton {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            // compact: true
                            icon.source: keyField.echoMode === TextInput.Password
                                         ? "qrc:/icons/res/eye-off.svg"
                                         : "qrc:/icons/res/eye.svg"
                            onClicked: keyField.echoMode = keyField.echoMode === TextInput.Password
                                                         ? TextInput.Normal : TextInput.Password
                        }
                    }
                }

                LabeledField {
                    label: "API 地址"
                    visible: ApiConfigViewModel.showDomainInput
                    TextField {
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        text: ApiConfigViewModel.baseDomain
                        placeholderText: "https://api.openai.com"
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily
                        onTextEdited: ApiConfigViewModel.setBaseDomain(text)
                    }
                }

                LabeledField {
                    label: "接口路径"
                    visible: ApiConfigViewModel.showDomainInput
                    TextField {
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        text: ApiConfigViewModel.apiPath
                        placeholderText: "/chat/completions"
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily
                        onTextEdited: ApiConfigViewModel.setApiPath(text)
                    }
                }

                LabeledField {
                    label: "可用模型"
                    ComboBox {
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        model: ApiConfigViewModel.modelList
                        currentIndex: ApiConfigViewModel.currentModelsIndex
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily
                        onActivated: (index) => ApiConfigViewModel.setCurrentModelsIndex(index)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceSm

                    AppButton {
                        text: "获取可用模型"
                        variant: "secondary"
                        enabled: !ApiConfigViewModel.fetchModelListState
                        onClicked: ApiConfigViewModel.fetchModelList()
                    }

                    BusyIndicator {
                        running: ApiConfigViewModel.fetchModelListState
                        visible: running
                        implicitWidth: Theme.iconLg
                        implicitHeight: Theme.iconLg
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }

    // ========== 添加自定义来源 ==========
    Dialog {
        id: addModelDialog
        title: "添加自定义模型来源"
        modal: true
        anchors.centerIn: Overlay.overlay
        width: Math.min(parent ? parent.width - Theme.spaceXl * 2 : 360, 420)
        padding: Theme.spaceLg
        standardButtons: Dialog.Ok | Dialog.Cancel

        background: Rectangle {
            color: Theme.bgElevated
            radius: Theme.radiusMd
            border.width: 1
            border.color: Theme.borderDefault
        }

        onAccepted: {
            ApiConfigViewModel.addCustomConfig(nameInput.text)
            nameInput.text = ""
        }

        contentItem: ColumnLayout {
            spacing: Theme.spaceSm

            Label {
                Layout.fillWidth: true
                text: "请输入配置名称"
                font.pixelSize: Theme.fontBodySm
                font.family: Theme.fontFamily
                color: Theme.textSecondary
            }

            TextField {
                id: nameInput
                Layout.fillWidth: true
                implicitHeight: Theme.controlMd
                placeholderText: "配置名称"
                focus: true
                font.pixelSize: Theme.fontBody
                font.family: Theme.fontFamily
            }
        }
    }

    ConfirmDialog {
        id: deleteDialog
        title: "确认删除"
        text: "确定删除当前 API 配置吗？"
        acceptText: "删除"
        danger: true
        onConfirmed: ApiConfigViewModel.deleteCurrentConfig()
    }

    Connections {// 兼容旧接口
        target: ApiConfigViewModel
        function onOperationError(message) { Ui.toast(message, true) }
        function onOperationTip(message) { Ui.toast(message, false) }
        function onDeleteSuccess() { Ui.toast("配置已删除", false) }
        function onTestSuccess() { Ui.toast("连接正常", false) }
    }
}
