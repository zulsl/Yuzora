import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

import Yuzora 1.0

// 世界书页
// 响应式策略：差异大 → Loader
//   宽窗口：AdaptiveSplit 1:3（文集 | 条目）
//   窄窗口：MasterDetailStack（文集 → 条目）
MasterDetailPage {
    id: root

    readonly property bool compactDetailVisible:
        layoutLoader.item ? layoutLoader.item.detailShown === true : false

    listTitle: "世界书 / 知识书"
    detailTitle: WorldInfoViewModel.currentBookName.length > 0
                 ? WorldInfoViewModel.currentBookName : "条目列表"
    detailVisible: compactDetailVisible
    onBackRequested: {
        if (layoutLoader.item)
            layoutLoader.item.hideDetail()
    }
    contentPadding: Theme.spaceSm

    Loader {
        id: layoutLoader
        anchors.fill: parent
        sourceComponent: Theme.isCompact ? compactLayout : wideLayout
    }

    // ========== 宽窗口 ==========
    Component {
        id: wideLayout

        AdaptiveSplit {
            firstRatio: 1
            secondRatio: 3

            first: BookListPanel {
                onImportRequested: importDialog.open()
                onExportRequested: exportDialog.open()
                onDeleteRequested: deleteDialog.open()
                onRenameRequested: renameDialog.open()
            }

            second: EntryListPanel {}
        }
    }

    // ========== 窄窗口 ==========
    Component {
        id: compactLayout

        MasterDetailStack {
            listComponent: compactBooks
            detailComponent: compactEntries
        }
    }

    Component {
        id: compactBooks

        BookListPanel {
            onImportRequested: importDialog.open()
            onExportRequested: exportDialog.open()
            onDeleteRequested: deleteDialog.open()
            onRenameRequested: renameDialog.open()
            // detailRequested 由 MasterDetailStack 自动连接到 showDetail()
        }
    }

    Component {
        id: compactEntries

        EntryListPanel {
            anchors.fill: parent
        }
    }

    // ========== 对话框 ==========
    FileDialog {
        id: importDialog
        title: "导入世界书"
        nameFilters: ["JSON 文件 (*.json)", "所有文件 (*)"]
        fileMode: FileDialog.OpenFile
        onAccepted: WorldInfoViewModel.importBook(selectedFile)
    }

    FileDialog {
        id: exportDialog
        title: "导出世界书"
        nameFilters: ["JSON 文件 (*.json)"]
        fileMode: FileDialog.SaveFile
        onAccepted: WorldInfoViewModel.exportCurrentBook(selectedFile)
    }

    ConfirmDialog {
        id: deleteDialog
        title: "确认删除"
        text: "确定删除该世界书吗？其中的条目会一并删除。"
        acceptText: "删除"
        danger: true
        onConfirmed: WorldInfoViewModel.deleteBook()
    }

    Dialog {
        id: renameDialog
        title: "重命名文集"
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

        onAccepted: WorldInfoViewModel.currentBookName = nameInput.text

        contentItem: TextField {
            id: nameInput
            text: WorldInfoViewModel.currentBookName
            placeholderText: "文集名称"
            focus: true
            font.pixelSize: Theme.fontBody
            font.family: Theme.fontFamily
        }
    }
}
