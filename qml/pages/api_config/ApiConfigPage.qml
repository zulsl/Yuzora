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
                                color: ApiConfigViewModel.draft.isActive ? Theme.success
                                                                          : "transparent"
                                border.width: ApiConfigViewModel.draft.isActive ? 0 : 1
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
                        enabled: !ApiConfigViewModel.draft.isActive
                        onClicked: ApiConfigViewModel.applyCurrentConfig()
                    }

                    AppButton {
                        text: "删除"
                        variant: "danger"
                        visible: !ApiConfigViewModel.draft.isBuiltin
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
                    visible: !ApiConfigViewModel.draft.isBuiltin
                    TextField {
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        id: nameField
                        text: ApiConfigViewModel.draft.name
                        TextContextMenu {}
                        placeholderText: "给这个 API 配置起个名字"
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily
                        onTextEdited: ApiConfigViewModel.draft.name = text
                    }
                }

                LabeledField {
                    label: "API 密钥"

                    TextField {
                        id: keyField
                        TextContextMenu {}
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        text: ApiConfigViewModel.draft.apiKey
                        echoMode: TextInput.Password
                        placeholderText: "sk-*************"
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily
                        rightPadding: Theme.touchTarget
                        onTextEdited: ApiConfigViewModel.draft.apiKey = text

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
                    visible: !ApiConfigViewModel.draft.isBuiltin
                    TextField {
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        id: domainField
                        text: ApiConfigViewModel.draft.baseDomain
                        TextContextMenu {}
                        placeholderText: "https://api.openai.com"
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily
                        onTextEdited: ApiConfigViewModel.draft.baseDomain = text
                    }
                }

                LabeledField {
                    label: "接口路径"
                    visible: !ApiConfigViewModel.draft.isBuiltin
                    TextField {
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        id: pathField
                        text: ApiConfigViewModel.draft.apiPath
                        TextContextMenu {}
                        placeholderText: "/chat/completions"
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily
                        onTextEdited: ApiConfigViewModel.draft.apiPath = text
                    }
                }

                LabeledField {
                    label: "可用模型"
                    ComboBox {
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        model: ApiConfigViewModel.draft.modelList
                        currentIndex: ApiConfigViewModel.draft.modelsIndex
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily
                        onActivated: (index) => ApiConfigViewModel.draft.setCurrentModelsIndex(index)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceSm

                    AppButton {
                        text: "获取可用模型"
                        variant: "secondary"
                        enabled: !ApiConfigViewModel.isFetchingModels
                        onClicked: ApiConfigViewModel.fetchModelList()
                    }

                    BusyIndicator {
                        running: ApiConfigViewModel.isFetchingModels
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

    // 文本控件的 text: draft.xxx 绑定在用户输入后会被控件内部赋值断掉，
    // 只靠绑定会出现"切了配置但输入框还是上一份的内容"。
    // 这里在草稿被外部改动（切换配置、拉取模型列表）时重新对齐文本。
    Connections {
        target: ApiConfigViewModel.draft
        function onNameChanged() {
            if (nameField.text !== ApiConfigViewModel.draft.name && !nameField.activeFocus)
                nameField.text = ApiConfigViewModel.draft.name
        }
        function onApiKeyChanged() {
            if (keyField.text !== ApiConfigViewModel.draft.apiKey && !keyField.activeFocus)
                keyField.text = ApiConfigViewModel.draft.apiKey
        }
        function onBaseDomainChanged() {
            if (domainField.text !== ApiConfigViewModel.draft.baseDomain && !domainField.activeFocus)
                domainField.text = ApiConfigViewModel.draft.baseDomain
        }
        function onApiPathChanged() {
            if (pathField.text !== ApiConfigViewModel.draft.apiPath && !pathField.activeFocus)
                pathField.text = ApiConfigViewModel.draft.apiPath
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

        background: Rectangle {
            color: Theme.bgElevated
            radius: Theme.radiusMd
            border.width: 1
            border.color: Theme.borderDefault
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
                TextContextMenu {}
                Layout.fillWidth: true
                implicitHeight: Theme.controlMd
                placeholderText: "配置名称"
                focus: true
                font.pixelSize: Theme.fontBody
                font.family: Theme.fontFamily
            }
        }

        footer: Item {
            implicitHeight: Theme.touchTarget + Theme.spaceSm
            Row {
                anchors.right: parent.right
                anchors.rightMargin: Theme.spaceLg
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.spaceSm

                AppButton {
                    text: "取消"
                    variant: "ghost"
                    onClicked: addModelDialog.close()
                }
                AppButton {
                    text: "添加"
                    variant: "primary"
                    enabled: nameInput.text.trim().length > 0
                    onClicked: {
                        if (ApiConfigViewModel.addCustomConfig(nameInput.text)) {
                            nameInput.text = ""
                            addModelDialog.close()
                        }
                    }
                }
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
}
