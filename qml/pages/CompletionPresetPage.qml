import QtQuick
import QtQuick.Dialogs

import Yuzora 1.0

// 对话补全预设页
// 响应式策略：差异大 → Loader
//   宽窗口：AdaptiveSplit 1:3（预设列表 | 编辑表单）
//   窄窗口：MasterDetailStack（列表 → 编辑页）
PageScaffold {
    id: root

    title: "对话补全预设"
    contentPadding: Theme.spaceSm
    onLeftClicked: Ui.requestDrawer()

    function localPath(url) {
        const s = String(url)
        if (s.indexOf("file:///") === 0)
            return decodeURIComponent(s.substring(8))
        if (s.indexOf("file://") === 0)
            return decodeURIComponent(s.substring(7))
        return s
    }

    Loader {
        anchors.fill: parent
        sourceComponent: Theme.isCompact ? compactLayout : wideLayout
    }

    // ========== 宽窗口 ==========
    Component {
        id: wideLayout

        AdaptiveSplit {
            firstRatio: 1
            secondRatio: 3

            first: PresetListPanel {
                onImportRequested: importDialog.open()
                onExportAllRequested: exportAllDialog.open()
            }

            second: PresetEditForm {
                onExportRequested: exportSingleDialog.open()
            }
        }
    }

    // ========== 窄窗口 ==========
    Component {
        id: compactLayout

        MasterDetailStack {
            id: presetStack
            listComponent: compactList
            detailComponent: compactDetail
        }
    }

    Component {
        id: compactList

        PresetListPanel {
            onImportRequested: importDialog.open()
            onExportAllRequested: exportAllDialog.open()
            onPresetActivated: presetStack.showDetail()
        }
    }

    Component {
        id: compactDetail

        PageScaffold {
            title: "预设编辑"
            leftIconSource: "qrc:/icons/res/arrow-narrow-left.svg"
            onLeftClicked: presetStack.hideDetail()

            PresetEditForm {
                anchors.fill: parent
                onExportRequested: exportSingleDialog.open()
            }
        }
    }

    // ========== 对话框 ==========
    FileDialog {
        id: importDialog
        title: "导入预设"
        nameFilters: ["JSON 文件 (*.json)"]
        fileMode: FileDialog.OpenFile
        onAccepted: CompletionPresetViewModel.importFromFile(selectedFile)
    }

    FileDialog {
        id: exportAllDialog
        title: "导出全部预设"
        nameFilters: ["JSON 文件 (*.json)"]
        fileMode: FileDialog.SaveFile
        onAccepted: CompletionPresetViewModel.exportAll(root.localPath(selectedFile))
    }

    FileDialog {
        id: exportSingleDialog
        title: "导出预设"
        nameFilters: ["JSON 文件 (*.json)"]
        fileMode: FileDialog.SaveFile
        onAccepted: CompletionPresetViewModel.exportCurrent(
                        CompletionPresetViewModel.currentEditingPreset.id,
                        root.localPath(selectedFile))
    }

    Connections {
        target: CompletionPresetViewModel
        function onOperationError(message) { Ui.toast(message, true) }
        function onOperationTip(message) { Ui.toast(message, false) }
        function onImportSuccess(count) { Ui.toast("已导入 " + count + " 个预设", false) }
        function onExportSuccess() { Ui.toast("导出成功", false) }
    }
}
