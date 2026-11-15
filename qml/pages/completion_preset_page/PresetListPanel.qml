import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 预设列表面板
// 职责：预设的增删选与导入导出入口；编辑区由页面决定放哪。
// 根节点用 Item 包一层，便于被 AdaptiveSplit 用 LayoutItemProxy 搬运。
Item {
    id: root

    implicitWidth: 300
    implicitHeight: 480

    signal importRequested()
    signal exportRequested()
    // 窄窗口下选中预设后要求推入编辑页（由 MasterDetailStack 自动接线）
    signal detailRequested()

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spaceSm

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceXs

            Label {
                Layout.fillWidth: true
                text: "预设列表"
                font.pixelSize: Theme.fontBody
                font.bold: true
                font.family: Theme.fontFamily
                color: Theme.textPrimary
            }

            IconButton {
                tip: "导入预设"
                compact: true
                icon.source: "qrc:/icons/res/file-import.svg"
                onClicked: root.importRequested()
            }
            IconButton {
                tip: "新建预设"
                compact: true
                icon.source: "qrc:/icons/res/file-plus.svg"
                onClicked: CompletionPresetViewModel.createPreset()
            }

        }

        ListView {
            id: presetListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.spaceXs
            model: CompletionPresetViewModel.presetModel
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: ItemDelegate {
                id: presetDelegate
                required property int index
                required property var modelData

                width: ListView.view.width
                height: Theme.touchTarget
                highlighted: CompletionPresetViewModel.currentPreset.id === modelData.id

                onClicked: {
                    presetListView.currentIndex = index
                    CompletionPresetViewModel.selectPreset(presetDelegate.modelData.id)
                    root.detailRequested()
                }

                contentItem: RowLayout {
                    spacing: Theme.spaceSm

                    Label {
                        Layout.fillWidth: true
                        text: presetDelegate.modelData.name || ""
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                    }

                    TagChip {
                        visible: presetDelegate.modelData.isActive === true
                        text: "启用"
                        tone: "success"
                    }
                }

                background: Rectangle {
                    radius: Theme.radiusSm
                    color: presetDelegate.highlighted ? Theme.rowSelected
                         : presetDelegate.hovered ? Theme.rowHover
                         : "transparent"
                }
            }
        }

        EmptyState {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.controlLg * 3
            visible: presetListView.count === 0
            text: "还没有预设"
        }
    }
}
