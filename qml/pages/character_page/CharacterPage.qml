import QtQuick
import QtQuick.Dialogs

import Yuzora 1.0

// 角色管理页
// 响应式策略：差异大 → Loader
//   宽窗口：AdaptiveSplit 1:3 并排（LayoutItemProxy，切换断点不重建子项）
//   窄窗口：MasterDetailStack 列表 → 推入详情
PageScaffold {
    id: root

    title: "角色管理"
    contentPadding: Theme.spaceSm
    onLeftClicked: Ui.requestDrawer()

    // QML 的 url 没有 toLocalFile()，这里做一次统一的 file:// → 本地路径转换
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

    // ========== 宽窗口：左右并排 ==========
    Component {
        id: wideLayout

        AdaptiveSplit {
            firstRatio: 1
            secondRatio: 3

            first: CharacterListPanel {
                onImportRequested: importDialog.open()
                // 并排时详情一直在右侧，无需额外动作
            }

            second: CharacterDetailForm {
                onExportRequested: exportDialog.open()
            }
        }
    }

    // ========== 窄窗口：列表 → 详情 ==========
    Component {
        id: compactLayout

        MasterDetailStack {
            id: detailStack
            listComponent: compactList
            detailComponent: compactDetail
        }
    }

    Component {
        id: compactList

        CharacterListPanel {
            onImportRequested: importDialog.open()
            onDetailRequested: detailStack.showDetail()
        }
    }

    Component {
        id: compactDetail

        // 详情页以整页形式呈现：自带返回按钮
        PageScaffold {
            title: root.detailStackTitle()
            leftIconSource: "qrc:/icons/res/arrow-narrow-left.svg"
            onLeftClicked: detailStack.hideDetail()

            CharacterDetailForm {
                anchors.fill: parent
                onExportRequested: exportDialog.open()
            }
        }
    }

    function detailStackTitle() {
        return CharacterViewModel.currentCharacter
               && CharacterViewModel.currentCharacter.name
               ? CharacterViewModel.currentCharacter.name : "角色详情"
    }

    // ========== 对话框 ==========
    FileDialog {
        id: importDialog
        title: "导入角色卡"
        nameFilters: ["角色卡文件 (*.png *.json)", "所有文件 (*.*)"]
        fileMode: FileDialog.OpenFile
        onAccepted: CharacterViewModel.importCharacter(selectedFile)
    }

    FileDialog {
        id: exportDialog
        title: "导出角色卡"
        nameFilters: ["PNG 角色卡 (*.png)", "JSON 角色卡 (*.json)"]
        fileMode: FileDialog.SaveFile
        defaultSuffix: "png"
        onAccepted: CharacterViewModel.exportCurrentCharacter(
                        root.localPath(selectedFile))
    }

    // 页面级错误统一走全局吐司
    Connections {
        target: CharacterViewModel
        function onOperationError(message) { Ui.toast(message, true) }
        function onOperationTip(message) { Ui.toast(message, false) }
        function onImportSuccess(count) { Ui.toast("已导入 " + count + " 个角色", false) }
        function onExportSuccess() { Ui.toast("导出成功", false) }
    }
}
