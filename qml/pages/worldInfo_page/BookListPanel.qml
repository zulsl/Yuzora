import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 世界书文集列表面板
// 职责：文集的增删改开关与选中；具体文件选择由页面负责（面板只发意图）。
// 根节点用 Item 包一层，便于被 AdaptiveSplit 用 LayoutItemProxy 搬运。
Item {
    id: root

    implicitWidth: 280
    implicitHeight: 480

    signal importRequested()
    signal exportRequested()
    signal deleteRequested()
    signal renameRequested()
    // 窄窗口下选中文集后，由页面推入条目页
    signal bookActivated()

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spaceSm

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceXs

            Label {
                Layout.fillWidth: true
                text: "世界书文集"
                font.pixelSize: Theme.fontBody
                font.bold: true
                font.family: Theme.fontFamily
                color: Theme.textPrimary
            }

            IconButton {
                tip: "新建文集"
                compact: true
                icon.source: "qrc:/icons/res/file-plus.svg"
                onClicked: WorldInfoViewModel.createBook()
            }
            IconButton {
                tip: "导入"
                compact: true
                icon.source: "qrc:/icons/res/file-import.svg"
                onClicked: root.importRequested()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceXs

            IconButton {
                tip: "导出"
                compact: true
                icon.source: "qrc:/icons/res/file-export.svg"
                enabled: WorldInfoViewModel.currentBookId.length > 0
                onClicked: root.exportRequested()
            }
            IconButton {
                tip: "重命名"
                compact: true
                icon.source: "qrc:/icons/res/edit.svg"
                enabled: WorldInfoViewModel.currentBookId.length > 0
                onClicked: root.renameRequested()
            }
            IconButton {
                tip: "删除"
                compact: true
                danger: true
                icon.source: "qrc:/icons/res/trash.svg"
                enabled: WorldInfoViewModel.currentBookId.length > 0
                onClicked: root.deleteRequested()
            }
            Item { Layout.fillWidth: true }
        }

        ListView {
            id: bookListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.spaceXs
            model: WorldInfoViewModel.bookModel
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: ItemDelegate {
                id: bookDelegate
                required property int index
                required property var modelData

                width: ListView.view.width
                height: Theme.touchTarget
                highlighted: WorldInfoViewModel.currentBookId === bookDelegate.modelData.id

                onClicked: {
                    bookListView.currentIndex = index
                    WorldInfoViewModel.selectBook(bookDelegate.modelData.id)
                    root.bookActivated()
                }

                contentItem: RowLayout {
                    spacing: Theme.spaceSm

                    CheckBox {
                        checked: bookDelegate.modelData.isEnabled
                        onToggled: WorldInfoViewModel.toggleBookEnabled(bookDelegate.modelData.id)
                    }

                    Label {
                        Layout.fillWidth: true
                        text: bookDelegate.modelData.name || ""
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily
                        font.weight: Font.Medium
                        elide: Text.ElideRight
                        color: bookDelegate.modelData.isEnabled ? Theme.textPrimary
                                                                : Theme.textDisabled
                    }
                }

                background: Rectangle {
                    radius: Theme.radiusSm
                    color: bookDelegate.highlighted ? Theme.rowSelected
                         : bookDelegate.hovered ? Theme.rowHover
                         : "transparent"
                }
            }
        }

        EmptyState {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.controlLg * 3
            visible: bookListView.count === 0
            text: "还没有世界书"
        }
    }
}
