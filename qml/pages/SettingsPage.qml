import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8
import QtQuick.Dialogs 6.8
import QtQuick.Controls.Material 6.8
import Yuzora 1.0

Page {
    id: root
    padding: Theme.page_padding
    header: MyToolBar{
        title:"设置"
        onLeftClicked: sideDrawer.open()
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ColumnLayout {
            width: parent.width
            spacing: Theme.layout_spacing

            // ========== 外观设置 ==========
            GroupBox {
                title: "外观设置"
                Layout.fillWidth: true
                ColumnLayout {
                    width: parent.width
                    spacing: Theme.widget_spacing

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.widget_spacing
                        Label { text: "主题模式"; width: 100 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["浅色模式", "深色模式", "跟随系统"]
                            currentIndex: Theme.materialTheme
                            onCurrentIndexChanged: {
                                if (SettingsViewModel)
                                    SettingsViewModel.themeMode = currentIndex
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.widget_spacing
                        Label { text: "字体大小"; width: 100 }
                        SpinBox {
                            from: 12; to: 24; stepSize: 1
                            value: Theme.baseFontSize
                            onValueChanged: SettingsViewModel.baseFontSize = value
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.widget_spacing
                        Label { text: "主题颜色"; width: 100 }
                        Button{
                            Layout.fillWidth: true
                            text: "调色盘"
                            onClicked: colorDialog.open()
                        }
                    }
                }
            }

            // ========== 通用设置 ==========
            GroupBox {
                title: "通用设置"
                Layout.fillWidth: true
                // ColumnLayout {
                //     width: parent.width
                //     spacing: 12

                //     RowLayout {
                //         Layout.fillWidth: true
                //         Label { text: "回车发送消息"; Layout.fillWidth: true }
                //         Switch {
                //             checked: SettingsViewModel ? SettingsViewModel.sendWithEnter : true
                //             onCheckedChanged: {
                //                 if (SettingsViewModel)
                //                     SettingsViewModel.sendWithEnter = checked
                //             }
                //         }
                //     }

                //     RowLayout {
                //         Layout.fillWidth: true
                //         Label { text: "显示消息时间"; Layout.fillWidth: true }
                //         Switch {
                //             checked: SettingsViewModel ? SettingsViewModel.showTimeStamp : true
                //             onCheckedChanged: {
                //                 if (SettingsViewModel)
                //                     SettingsViewModel.showTimeStamp = checked
                //             }
                //         }
                //     }

                //     RowLayout {
                //         Layout.fillWidth: true
                //         Label { text: "新消息自动滚动"; Layout.fillWidth: true }
                //         Switch {
                //             checked: SettingsViewModel ? SettingsViewModel.autoScroll : true
                //             onCheckedChanged: {
                //                 if (SettingsViewModel)
                //                     SettingsViewModel.autoScroll = checked
                //             }
                //         }
                //     }

                //     RowLayout {
                //         Layout.fillWidth: true
                //         Label { text: "自动保存间隔(秒)"; width: 140 }
                //         SpinBox {
                //             from: 5; to: 300; stepSize: 5
                //             value: SettingsViewModel ? SettingsViewModel.autoSaveInterval : 30
                //             onValueChanged: {
                //                 if (SettingsViewModel)
                //                     SettingsViewModel.autoSaveInterval = value
                //             }
                //         }
                //     }
                // }
            }

            // ========== 网络设置 ==========
            GroupBox {
                title: "网络设置"
                Layout.fillWidth: true
                // ColumnLayout {
                //     width: parent.width
                //     spacing: 12

                //     RowLayout {
                //         Layout.fillWidth: true
                //         Label { text: "启用代理"; Layout.fillWidth: true }
                //         Switch {
                //             id: proxySwitch
                //             checked: SettingsViewModel ? SettingsViewModel.enableProxy : false
                //             onCheckedChanged: {
                //                 if (SettingsViewModel)
                //                     SettingsViewModel.enableProxy = checked
                //             }
                //         }
                //     }

                //     RowLayout {
                //         Layout.fillWidth: true
                //         enabled: proxySwitch.checked
                //         Label { text: "代理类型"; width: 100 }
                //         ComboBox {
                //             Layout.fillWidth: true
                //             model: ["HTTP", "SOCKS5"]
                //             currentIndex: SettingsViewModel?.proxyType === "socks5" ? 1 : 0
                //             onCurrentIndexChanged: {
                //                 if (!SettingsViewModel) return
                //                 SettingsViewModel.proxyType =
                //                     currentIndex === 0 ? "http" : "socks5"
                //             }
                //         }
                //     }

                //     RowLayout {
                //         Layout.fillWidth: true
                //         enabled: proxySwitch.checked
                //         Label { text: "代理地址"; width: 100 }
                //         TextField {
                //             Layout.fillWidth: true
                //             text: SettingsViewModel ? SettingsViewModel.proxyHost : ""
                //             placeholderText: "127.0.0.1"
                //             onEditingFinished: {
                //                 if (SettingsViewModel)
                //                     SettingsViewModel.proxyHost = text
                //             }
                //         }
                //     }

                //     RowLayout {
                //         Layout.fillWidth: true
                //         enabled: proxySwitch.checked
                //         Label { text: "代理端口"; width: 100 }
                //         SpinBox {
                //             from: 1; to: 65535
                //             value: SettingsViewModel ? SettingsViewModel.proxyPort : 7890
                //             onValueChanged: {
                //                 if (SettingsViewModel)
                //                     SettingsViewModel.proxyPort = value
                //             }
                //         }
                //     }

                //     RowLayout {
                //         Layout.fillWidth: true
                //         Label { text: "请求超时(秒)"; width: 100 }
                //         SpinBox {
                //             from: 5; to: 300; stepSize: 5
                //             value: SettingsViewModel ? SettingsViewModel.requestTimeout : 30
                //             onValueChanged: {
                //                 if (SettingsViewModel)
                //                     SettingsViewModel.requestTimeout = value
                //             }
                //         }
                //     }
                // }
            }

            // ========== 存储设置 ==========
            GroupBox {
                title: "存储设置"
                Layout.fillWidth: true
                ColumnLayout {
                    width: parent.width
                    spacing: 12

                    RowLayout {
                        spacing: 8
                        Layout.fillWidth: true
                        Label { text: "数据存储路径"; width: 100 }
                        Label {
                            Layout.fillWidth: true
                            text: SettingsViewModel ? SettingsViewModel.dataPath : "默认路径"
                            elide: Text.ElideMiddle
                            color: palette.mid
                        }
                        Button {
                            text: "更改"
                            onClicked: folderDialog.open()
                        }
                    }
                    Button {
                        Layout.fillWidth: true
                        text: "清除所有数据"
                        onClicked: clearMsgDlg.open()
                    }
                }
            }


            Button {
                Layout.fillWidth: true
                text: "恢复默认设置"
                onClicked: SettingsViewModel.resetAllSettings()
            }
        }
    }

    //目录选择对话框
    FolderDialog {
        id: folderDialog
        title: "选择数据存储目录"
        acceptLabel: "确定"
        rejectLabel: "取消"

        // 用户点击确定
        onAccepted: {
            SettingsViewModel.customAppPath(selectedFolder);
        }
        // 用户取消
        onRejected: {
            console.log("用户取消选择")
        }
    }

    MessageDialog {
        id: clearMsgDlg
        title: "提示"
        text: "该操作将重置所有应用数据,确定要清除吗？"
        informativeText: "该操作将重启应用"
        buttons: MessageDialog.Yes | MessageDialog.No | MessageDialog.Cancel

        onAccepted: SettingsViewModel.clearAllData()
    }

    //颜色对话框
    ColorDialog {
        id: colorDialog
        title: "选择主题主色"
        selectedColor: Theme.primaryColor // 设置初始颜色
        onAccepted: {
            SettingsViewModel.themeColor = selectedColor
            // 用户点击“确定”后，selectedColor 即为新颜色
        }
    }
}