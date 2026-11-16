import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 条列表面板
// 职责：条目列表的标题栏与新建入口，列表本身交给 BookEntryListView。
// 根节点用 Item 包一层，便于被 AdaptiveSplit 用 LayoutItemProxy 搬运。
Item {
    id: root

    implicitWidth: 420
    implicitHeight: 480

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spaceSm

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm

            Label {
                Layout.fillWidth: true
                text: "条目列表"
                font.pixelSize: Theme.fontBody
                font.bold: true
                font.family: Theme.fontFamily
                color: Theme.textPrimary
            }

            IconButton {
                tip: "新建条目"
                compact: true
                icon.source: "qrc:/icons/res/file-plus.svg"
                enabled: WorldInfoViewModel.currentBookId.length > 0
                onClicked: WorldInfoEntryViewModel.createEntry()
            }
        }

        // 列表区包一层 Item：空态占位要和列表互为兄弟才能 anchors.centerIn，
        // 直接放进 Flickable 里会按"内容坐标"居中（没有条目时内容高 0，直接不见）。
        Item {
            id: listArea
            Layout.fillWidth: true
            Layout.fillHeight: true

            BookEntryListView {
                anchors.fill: parent
            }

            EmptyState {
                anchors.centerIn: parent
                visible: WorldInfoViewModel.currentBookId.length > 0
                         && WorldInfoEntryViewModel.entryCount === 0
                text: "当前无条目"
                detail: "点右上角的 + 新建条目"
            }
        }
    }

    EmptyState {
        anchors.centerIn: parent
        visible: WorldInfoViewModel.currentBookId.length === 0
        text: "请选择一个世界书文集"
    }
}
