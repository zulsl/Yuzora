import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import Yuzora 1.0

// 系统设置页
// 职责：只做组合与绑定 —— 不写样式、不写业务分支。
// 原实现的"通用设置""网络设置"两组控件在改动前就是整段注释掉的（未启用），
// 本次重构不擅自启用，保持与改造前一致的功能边界。
PageScaffold {
    id: root

    title: "设置"
    onLeftClicked: Ui.requestDrawer()

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: Theme.sectionGap

            // ========== 外观设置 ==========
            SectionCard {
                title: "外观设置"
                Layout.fillWidth: true

                LabeledField {
                    label: "主题模式"

                    ComboBox {
                        id: themeCombo
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        model: ["浅色模式", "深色模式", "跟随系统"]
                        currentIndex: SettingsViewModel.themeMode
                        onActivated: (index) => SettingsViewModel.themeMode = index
                    }
                }

                LabeledField {
                    label: "字体大小"

                    SpinBox {
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        from: 12
                        to: 24
                        stepSize: 1
                        value: SettingsViewModel.baseFontSize
                        onValueModified: SettingsViewModel.baseFontSize = value
                    }
                }

                LabeledField {
                    label: "主题颜色"

                    AppButton {
                        Layout.fillWidth: true
                        text: "调整主色"
                        variant: "secondary"
                        onClicked: colorDialog.open()
                    }
                }
            }

            // ========== 存储设置 ==========
            SectionCard {
                title: "存储设置"
                Layout.fillWidth: true

                LabeledField {
                    label: "数据目录"

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            Layout.fillWidth: true
                            text: SettingsViewModel.dataPath
                            elide: Text.ElideMiddle
                            font.pixelSize: Theme.fontBodySm
                            font.family: Theme.fontFamily
                            color: Theme.textSecondary
                        }
                        AppButton {
                            text: "更改"
                            variant: "secondary"
                            onClicked: folderDialog.open()
                        }
                    }
                }

                LabeledField {
                    label: "清除数据"

                    AppButton {
                        Layout.fillWidth: true
                        text: "清除所有应用数据"
                        variant: "danger"
                        onClicked: clearDialog.open()
                    }
                }
            }

            AppButton {
                Layout.fillWidth: true
                text: "恢复默认设置"
                variant: "secondary"
                onClicked: {
                    SettingsViewModel.resetAllSettings()
                    Ui.toast("已恢复默认设置", false)
                }
            }

            Item { Layout.fillHeight: true }
        }
    }

    ConfirmDialog {
        id: clearDialog
        title: "确认清除"
        text: "该操作将重置所有应用数据，确定要继续吗？"
        detail: "应用会随后重启。"
        acceptText: "清除并重启"
        danger: true
        onConfirmed: SettingsViewModel.clearAllData()
    }

    FolderDialog {
        id: folderDialog
        title: "选择数据存储目录"
        onAccepted: SettingsViewModel.customAppPath(selectedFolder)
    }

    ColorDialog {
        id: colorDialog
        title: "选择主题主色"
        selectedColor: Theme.primaryColor
        onAccepted: SettingsViewModel.themeColor = selectedColor
    }
}
