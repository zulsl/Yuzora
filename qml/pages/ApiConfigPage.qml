import QtQuick 6.8
import QtQuick.Dialogs 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8
import QtQuick.Controls.Material 6.8


//api配置界面

Page {
    id: root
    padding: Theme.page_padding
    header: ToolBar {
        // padding: 0
        height: 50
        RowLayout {
            anchors.fill: parent
            spacing: 8

            ToolButton {
                id:menuBtn
                icon.source: "qrc:/icons/res/menu.svg"
                onClicked: sideDrawer.open()
            }

            Label {
                Layout.fillWidth: true
                text: "模型API配置"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            //占位弹簧
            Item {Layout.preferredWidth: menuBtn.width}
        }
    }


    ScrollView{
        id:scrollView
        anchors.fill: parent
        contentWidth: availableWidth
        Layout.maximumWidth: Screen.width
        clip: true
        ColumnLayout {
            anchors.fill: parent

            // ========== 顶部：下拉选择 + 添加按钮 ==========
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 5

                Label {
                    text: "API 连接配置:"
                    font.bold: true
                }
                ComboBox {
                    id: configCombo
                    Layout.fillWidth: true
                    model: ApiConfigViewModel.configListModel
                    textRole: "name"
                    valueRole: "id"//用id作为"值"

                    //qt6.8不支持直接写currentValue
                    // currentValue: ApiConfigViewModel.currentConfigId

                    Binding on currentIndex {
                        value: configCombo.indexOfValue(ApiConfigViewModel.currentConfigId)
                        when: configCombo.indexOfValue(ApiConfigViewModel.currentConfigId) !== -1 // 找到有效值才更新
                    }

                    // 自定义收起后的显示框
                    contentItem: Row {
                        spacing: 10
                        anchors.verticalCenter: parent.verticalCenter
                        leftPadding: 12
                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: ApiConfigViewModel.currentIsActive ? "#27ae60" : "transparent"
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: configCombo.displayText
                            font: configCombo.font
                            color: configCombo.palette.text
                            anchors.verticalCenter: parent.verticalCenter
                            elide: Text.ElideRight
                        }
                    }

                    // 下拉列表项自定义委托：左侧带激活状态绿点
                    delegate: ItemDelegate {
                        width: configCombo.width
                        highlighted: configCombo.highlightedIndex === index

                        contentItem: Row {
                            spacing: 10
                            anchors.verticalCenter: parent.verticalCenter

                            // 激活状态绿点
                            Rectangle {
                                width: 8
                                height: 8
                                radius: 4
                                color: model.isActive ? "#27ae60" : "transparent"
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Text {
                                text: model.name
                                font: configCombo.font
                                color: parent.highlighted ? configCombo.palette.highlightedText : configCombo.palette.text
                                anchors.verticalCenter: parent.verticalCenter
                                elide: Text.ElideRight//超出宽度显示省略号
                            }
                        }
                    }

                    //仅用户主动点选/键盘确认
                    onActivated: ApiConfigViewModel.selectConfig(currentValue)
                }

                // 操作按钮区
                GridLayout{
                    columns: 2

                    Button {
                        text: "设为当前使用"
                        icon.name: "emblem-default"
                        enabled: !ApiConfigViewModel.currentIsActive
                        onClicked: ApiConfigViewModel.applyCurrentConfig()
                    }

                    Button {
                        text: "删除"
                        visible: !ApiConfigViewModel.isCurrentBuiltin
                        icon.name: "edit-delete"
                        onClicked: ApiConfigViewModel.deleteCurrentConfig()
                    }

                    Button {
                        text: "添加自定义模型来源"
                        icon.name: "list-add"
                        onClicked: addModelDialog.open()
                    }


                }
            }

            // ========== 下方表单区域 ==========
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8
                enabled: configCombo.count > 0

                // 配置名称（仅自定义显示）
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: !ApiConfigViewModel.isCurrentBuiltin
                    Label { text: "配置名称:" }
                    TextField {
                        Layout.fillWidth: true
                        text: ApiConfigViewModel.configName
                        onTextEdited: ApiConfigViewModel.setConfigName(text)
                        placeholderText: "给这个API配置起个名字"
                    }
                }

                // API 密钥
                ColumnLayout {
                    Layout.fillWidth: true
                    Label { text: "API 密钥:" }
                    TextField {
                        Layout.fillWidth: true
                        text: ApiConfigViewModel.apiKey
                        echoMode: TextInput.Password
                        onTextEdited: ApiConfigViewModel.setApiKey(text)
                        placeholderText: "sk-*************"

                        rightPadding: 32// 预留按钮空间，防止文字盖住眼睛

                        // 显示/隐藏密码按钮
                        ToolButton {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            icon.source: {
                                var src = parent.echoMode === TextInput.Password
                                    ? "qrc:/icons/res/eye-off.svg" : "qrc:/icons/res/eye.svg";
                                return src;
                            }
                            icon.color: "black"
                            onClicked: parent.echoMode = parent.echoMode === TextInput.Password ? TextInput.Normal : TextInput.Password
                        }
                    }
                }

                // API 域名（仅自定义显示）
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: ApiConfigViewModel.showDomainInput
                    Label { text: "API 地址:"}
                    TextField {
                        Layout.fillWidth: true
                        text: ApiConfigViewModel.baseDomain
                        onTextEdited: ApiConfigViewModel.setBaseDomain(text)
                        placeholderText: "https://api.openai.com"
                    }
                }

                // 接口路径（仅自定义显示）
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: ApiConfigViewModel.showDomainInput
                    Label { text: "接口路径:" }
                    TextField {
                        Layout.fillWidth: true
                        text: ApiConfigViewModel.apiPath
                        onTextEdited: ApiConfigViewModel.setApiPath(text)
                        placeholderText: "/chat/completions"
                    }
                }

                // 子模型选择
                ColumnLayout {
                    Layout.fillWidth: true
                    Label { text: "可用模型:"}
                    ComboBox {
                        Layout.fillWidth: true
                        model: ApiConfigViewModel.modelList
                        currentIndex:ApiConfigViewModel.currentModelsIndex
                        onCurrentIndexChanged: {
                            ApiConfigViewModel.setCurrentModelsIndex(currentIndex)
                        }
                    }
                }


                Button {
                    text: "获取可用模型"
                    enabled: !ApiConfigViewModel.fetchModelListState
                    onClicked: ApiConfigViewModel.fetchModelList()
                    BusyIndicator {
                        anchors.centerIn: parent
                        running: ApiConfigViewModel.fetchModelListState
                        visible: running
                    }
                }

                //保留功能
                // Button {
                //     text: "测试连接"
                //     enabled: !ApiConfigViewModel.testingConnection
                //     onClicked: ApiConfigViewModel.testCurrentConnection()
                //     BusyIndicator {
                //         anchors.centerIn: parent
                //         running: ApiConfigViewModel.testingConnection
                //         visible: running
                //     }
                // }

            }
        }
    }


    // 添加模型对话框
    Dialog {
        id: addModelDialog
        //自定义标题
        // header: Rectangle {
        //     height: titleLabel.implicitHeight + 10
        //     Label {text: "弹窗居中标题";anchors.centerIn: parent}
        // }
        title: "添加自定义模型来源"
        modal: true

        //按钮居中
        footer: DialogButtonBox {
            standardButtons: Dialog.Ok | Dialog.Cancel
            alignment: Qt.AlignHCenter //居中
        }

        // 全局窗口居中
        anchors.centerIn: Overlay.overlay

        onAccepted: {
            ApiConfigViewModel.addCustomConfig(nameInput.text)
            nameInput.text = ""
        }
        onRejected: {
            nameInput.text = ""
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Label {
                text: "请输入配置名称"
                font.pointSize: 10
                Layout.alignment: Qt.AlignHCenter
            }

            TextField {
                id: nameInput
                Layout.fillWidth: true
                placeholderText: "配置名称"
                focus: true
            }
        }
    }

    //提示信息
    Connections {
        target: ApiConfigViewModel

        function onOperationError(errorMsg) {
            errorToast.show(errorMsg)
            console.warn("API配置错误:", errorMsg)
        }

        function onOperationTip(tipMsg) {
            tipToast.show(tipMsg)
            console.log(tipMsg)
        }
    }
}