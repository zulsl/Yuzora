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

    implicitWidth: 320
    implicitHeight: 480

    signal importRequested()
    signal detailRequested()

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spaceSm

        // 顶部：搜索 + 操作
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm

            TextField {
                id: searchField
                Layout.fillWidth: true
                implicitHeight: Theme.controlMd
                placeholderText: "搜索角色名称…"
                font.pixelSize: Theme.fontBody
                font.family: Theme.fontFamily
                onTextChanged: CharacterViewModel.searchKeyword = text
                onAccepted: CharacterViewModel.searchCharacters()
            }

            IconButton {
                tip: "新建角色"
                icon.source: "qrc:/icons/res/file-plus.svg"
                onClicked: CharacterViewModel.createNewCharacter()
            }

            IconButton {
                tip: "导入角色卡"
                icon.source: "qrc:/icons/res/file-import.svg"
                onClicked: root.importRequested()
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

                // 单击只做选中（详情在右侧或推入新页），进入对话用右侧省略号菜单里的显式动作
                onClicked: {
                    // 性能探针：测"点击 → 读库 + 信号派发"这一段（不含后续构建）
                    Perf.begin("选中角色（读库+信号）")
                    CharacterViewModel.setCurrentIndex(index)
                    CharacterViewModel.selectCharacter(rowDelegate.modelData.id)
                    Perf.end("选中角色（读库+信号）")
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

                        Label {
                            Layout.fillWidth: true
                            text: rowDelegate.modelData.lastUsedText || ""
                            visible: text.length > 0
                            font.pixelSize: Theme.fontCaption
                            font.family: Theme.fontFamily
                            color: Theme.textSecondary
                            elide: Text.ElideRight
                        }
                    }

                    IconButton {
                        tip: "角色详情"
                        compact: true
                        icon.source: "qrc:/icons/res/dots.svg"
                        onClicked: {
                            CharacterViewModel.setCurrentIndex(rowDelegate.index)
                            CharacterViewModel.selectCharacter(rowDelegate.modelData.id)
                            root.detailRequested()
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
}
