import QtQuick

import Yuzora 1.0

// 全局通知桥
// 职责：把各 ViewModel 的操作结果统一映射为 Ui.toast，避免每个页面各写一份
// Connections。页面只负责布局和命令，不再决定错误/成功提示。
Item {
    id: root
    width: 0
    height: 0
    visible: false

    Connections {
        target: CharacterViewModel
        function onOperationError(message) { Ui.toast(message, true) }
        function onOperationTip(message) { Ui.toast(message, false) }
        function onImportSuccess(count) { Ui.toast("已导入 " + count + " 个角色", false) }
        function onExportSuccess() { Ui.toast("导出成功", false) }
    }

    Connections {
        target: ApiConfigViewModel
        function onOperationError(message) { Ui.toast(message, true) }
        function onOperationTip(message) { Ui.toast(message, false) }
        function onDeleteSuccess() { Ui.toast("配置已删除", false) }
    }

    Connections {
        target: CompletionPresetViewModel
        function onOperationError(message) { Ui.toast(message, true) }
        function onOperationTip(message) { Ui.toast(message, false) }
        function onImportSuccess(count) { Ui.toast("已导入 " + count + " 个预设", false) }
        function onExportSuccess() { Ui.toast("导出成功", false) }
    }

    Connections {
        target: ChatViewModel
        function onOperationError(message) { Ui.toast(message, true) }
        function onOperationTip(message) { Ui.toast(message, false) }
        function onGenerationError(message) { Ui.toast(message, true) }
    }

    Connections {
        target: WorldInfoViewModel
        function onOperationError(message) { Ui.toast(message, true) }
        function onImportSuccess(count) { Ui.toast("已导入 " + count + " 个条目", false) }
        function onExportSuccess() { Ui.toast("导出成功", false) }
    }
}
