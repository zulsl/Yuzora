import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 角色列表面板
// 职责：搜索、新建/导入入口、角色列表；选中即通知 ViewModel，详情由页面决定放哪。
// 根节点用 Item 包一层：面板要能被 AdaptiveSplit 用 LayoutItemProxy 搬运，
// 而被代理的目标若是 Layout 自身，Layout.* 附加属性会失效。
Item {
    id: root

    implicitWidth: 300
    implicitHeight: 480

    signal importRequested()
    signal detailRequested()

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spaceSm

        // 顶部：搜索 + 操作
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceXs

            Label {
                Layout.fillWidth: true
                text: "角色列表"
                font.pixelSize: Theme.fontBody
                font.bold: true
                font.family: Theme.fontFamily
                color: Theme.textPrimary
            }

            IconButton {
                tip: "导入角色卡"
                compact: true
                icon.source: "qrc:/icons/res/file-import.svg"
                onClicked: root.importRequested()
            }

            IconButton {
                tip: "新建角色"
                compact: true
                icon.source: "qrc:/icons/res/file-plus.svg"
                onClicked: CharacterViewModel.createNewCharacter()
            }
        }

        // 列表
        ListView {
            id: characterListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.spaceXs
            model: CharacterViewModel.characterListModel
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: ItemDelegate {
                id: rowDelegate
                required property int index
                required property var modelData

                width: ListView.view.width
                height: Theme.avatarLg + Theme.spaceMd
                highlighted: CharacterViewModel.currentIndex === index

                // 单击只做选中（详情在右侧或推入新页）
                onClicked: {
                    if (CharacterViewModel.requestSelectCharacter(rowDelegate.modelData.id))
                        root.detailRequested()
                }

                contentItem: RowLayout {
                    spacing: Theme.spaceMd

                    CircleAvatar {
                        size: Theme.avatarLg
                        source: rowDelegate.modelData.avatarPath || ""
                        fallbackText: (rowDelegate.modelData.name || "?").charAt(0)
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Label {
                            Layout.fillWidth: true
                            text: rowDelegate.modelData.name || ""
                            font.pixelSize: Theme.fontBody
                            font.weight: Font.Medium
                            font.family: Theme.fontFamily
                            color: Theme.textPrimary
                            elide: Text.ElideRight
                        }
                    }

                    IconButton {
                        tip: "角色切换"
                        compact: true
                        icon.source: "qrc:/icons/res/switch-3.svg"
                        onClicked: {
                            if (CharacterViewModel.requestSelectCharacter(rowDelegate.modelData.id))
                                CharacterViewModel.openSelectedCharChat()
                        }
                    }
                }

                background: Rectangle {
                    radius: Theme.radiusSm
                    color: rowDelegate.highlighted ? Theme.rowSelected
                         : rowDelegate.hovered ? Theme.rowHover
                         : "transparent"
                }
            }
        }
    }

    // 空态
    EmptyState {
        anchors.centerIn: parent
        visible: characterListView.count === 0
        text: "还没有角色"
        detail: "点击右上角的 + 新建，或导入角色卡"
    }

    Connections {
        target: CharacterViewModel
        function onUnsavedChanges() { unsavedDialog.open() }
    }

    ConfirmDialog {
        id: unsavedDialog
        title: "未保存的修改"
        text: "当前角色有未保存的修改，切换后将丢失。"
        acceptText: "丢弃并切换"
        danger: true
        onConfirmed: CharacterViewModel.confirmPendingCharacterSelection()
        onDeclined: CharacterViewModel.cancelPendingCharacterSelection()
    }
}
