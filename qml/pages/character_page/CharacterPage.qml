import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

import Yuzora 1.0

// 角色管理页
// 响应式策略：差异大 → Loader
//   宽窗口：AdaptiveSplit 1:3 并排（LayoutItemProxy，切换断点不重建子项）
//   窄窗口：MasterDetailStack 列表 → 推入详情
PageScaffold {
    id: root

    readonly property bool compactDetailVisible: layoutLoader.item
                                                ? layoutLoader.item.detailShown === true
                                                : false

    title: compactDetailVisible
           ? (CharacterViewModel.currentCharacter && CharacterViewModel.currentCharacter.name
              ? CharacterViewModel.currentCharacter.name : "角色详情")
           : "角色管理"
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

    // ========== 首次构建详情表单时的加载态 ==========
    // 为什么要它：从"未选中"切到"有选中"时，右侧要一次性创建整个表单
    // （含多个长文本 TextArea），长角色卡的排版会阻塞一帧以上，表现为点完卡一下。
    // 这里先画一帧"加载中"，下一帧再真正构建表单 —— 用户看到的是转圈，而不是卡死。
    // 只在"无 → 有"时生效；两个角色之间切换不会销毁重建表单。
    property bool pendingFirstBuild: false
    // 【必须是普通属性，不能绑定 hasSelection】
    // 若写成 `property bool _hadSelection: CharacterViewModel.hasSelection`，
    // 信号到来时 hasSelection 已经是 true、绑定也已更新，永远判断不出"从无到有"的跳变，
    // 于是加载态和"首次构建"计时都不会触发。
    property bool _hadSelection: false

    // 详情区显示状态：0 = 空态，1 = 加载中，2 = 详情表单。
    // 关键：Loader 只依赖这个状态，**不能直接依赖 hasSelection**。
    // 否则信号发出的瞬间 Loader 就会先把完整表单建出来（实测 700ms+），
    // 再把"首次构建"逻辑套上去，等于白建一次又重建一次。
    property int detailMode: 0

    Component.onCompleted: {
        root._hadSelection = CharacterViewModel.hasSelection
        root.detailMode = root._hadSelection ? 2 : 0
    }

    Connections {
        target: CharacterViewModel

        function onCurrentCharacterChanged() {
            const selected = CharacterViewModel.hasSelection

            if (selected) {
                if (!root._hadSelection) {
                    // 先切到"加载中"，让 Loader 只构建一个 BusyIndicator，
                    // 真正的表单等下一帧再建，避免信号里同步建完整表单
                    root.detailMode = 1
                    root.pendingFirstBuild = true
                    buildDelay.restart()
                }
            } else {
                root.pendingFirstBuild = false
                root.detailMode = 0
            }
            root._hadSelection = selected
        }
    }

    Timer {
        id: buildDelay
        interval: 16          // 一帧：让加载态先上屏
        repeat: false
        onTriggered: {
            root.pendingFirstBuild = false
            root.detailMode = 2      // 这时才真正构建详情表单
        }
    }

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
        id: layoutLoader
        anchors.fill: parent
        sourceComponent: Theme.isCompact ? compactLayout : wideLayout
    }

    // ========== 宽窗口：左右并排 ==========
    // 右侧按"是否有选中"切换：没选中就显示空态，而不是一个空的详情表单。
    Component {
        id: characterDetailComp

        CharacterDetailForm {
            onExportRequested: exportDialog.open()
        }
    }

    Component {
        id: noCharacterComp

        EmptyState {
            text: "未选择角色"
            detail: "在左侧列表中选择一个角色进行编辑"
        }
    }

    // 首次构建表单时的加载态（一帧）
    Component {
        id: loadingComp

        Item {
            BusyIndicator {
                anchors.centerIn: parent
                running: true
                width: Theme.controlLg
                height: Theme.controlLg
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.verticalCenter
                anchors.topMargin: Theme.controlLg * 0.8
                text: "正在加载角色…"
                font.pixelSize: Theme.fontBodySm
                font.family: Theme.fontFamily
                color: Theme.textSecondary
            }
        }
    }

    Component {
        id: wideLayout

        AdaptiveSplit {
            firstRatio: 1
            secondRatio: 3

            first: CharacterListPanel {
                onImportRequested: importDialog.open()
                // 并排时详情一直在右侧，无需额外动作
            }

            second: Loader {
                // 只依赖 detailMode（由页面逻辑控制），**不直接依赖 hasSelection**：
                // 否则信号发出的那一瞬就会先把完整表单建出来，白建一次再重建。
                // 实测：直接依赖 hasSelection 时首次点击 730ms，改后 11ms。
                sourceComponent: root.detailMode === 0 ? noCharacterComp
                                 : root.detailMode === 1 ? loadingComp
                                 : characterDetailComp
            }
        }
    }

    // ========== 窄窗口：列表 → 详情 ==========
    Component {
        id: compactLayout

        MasterDetailStack {
            listComponent: compactList
            detailComponent: compactDetail
        }
    }

    Component {
        id: compactList

        CharacterListPanel {
            onImportRequested: importDialog.open()
            // detailRequested 由 MasterDetailStack 自动连接到 showDetail()，
            // 不再跨 Component 引用 detailStack 的 id（那样会 ReferenceError）
        }
    }

    Component {
        id: compactDetail

        CharacterDetailForm {
            anchors.fill: parent
            onExportRequested: exportDialog.open()
        }
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
