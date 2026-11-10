import QtQuick
import QtQuick.Dialogs

import Yuzora 1.0

// 对话补全预设页
// 响应式策略：差异大 → Loader
//   宽窗口：AdaptiveSplit 1:3（预设列表 | 编辑表单）
//   窄窗口：MasterDetailStack（列表 → 编辑页）
PageScaffold {
    id: root

    readonly property bool compactDetailVisible: layoutLoader.item
                                                ? layoutLoader.item.detailShown === true
                                                : false

    title: compactDetailVisible ? "预设编辑" : "对话预设"
    leftIconSource: compactDetailVisible
                    ? "qrc:/icons/res/arrow-narrow-left.svg"
                    : "qrc:/icons/res/menu.svg"
    contentPadding: Theme.spaceSm
    onLeftClicked: {
        if (compactDetailVisible && layoutLoader.item)
            layoutLoader.item.hideDetail()
        else
            Ui.requestDrawer()
    }

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

            first: PresetListPanel {
                onImportRequested: importDialog.open()// 连接信号
                onExportRequested: exportDialog.open()
            }

            // 没选中预设时显示空态，而不是一个空的编辑表单
            second: Loader {
                sourceComponent: CompletionPresetViewModel.hasSelection
                                 ? presetEditComp : noPresetComp
            }
        }
    }

    Component {// 详情界面
        id: presetEditComp

        PresetEditForm {
            onExportRequested: exportDialog.open()
        }
    }

    Component {// 空态占位
        id: noPresetComp

        EmptyState {
            text: "未选择预设"
            detail: "在左侧列表中选择一个预设进行编辑"
        }
    }

    // ========== 窄窗口 ==========
    Component {
        id: compactLayout

        MasterDetailStack {
            listComponent: compactList
            detailComponent: compactDetail
        }
    }

    Component {
        id: compactList

        PresetListPanel {
            onImportRequested: importDialog.open()
            onExportRequested: exportDialog.open()
            // detailRequested 由 MasterDetailStack 自动连接到 showDetail()
        }
    }

    Component {
        id: compactDetail

        PresetEditForm {
            anchors.fill: parent
            onExportRequested: exportDialog.open()
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
        id: exportDialog
        title: "导出预设"
        nameFilters: ["JSON 文件 (*.json)"]
        fileMode: FileDialog.SaveFile
        onAccepted: CompletionPresetViewModel.exportCurrent(
                        CompletionPresetViewModel.currentEditingPreset.id,selectedFile)
    }

    Connections {// 兼容旧接口
        target: CompletionPresetViewModel
        function onOperationError(message) { Ui.toast(message, true) }
        function onOperationTip(message) { Ui.toast(message, false) }
        function onImportSuccess(count) { Ui.toast("已导入 " + count + " 个预设", false) }
        function onExportSuccess() { Ui.toast("导出成功", false) }
    }
}
