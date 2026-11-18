pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 世界书条目 · 关键字列表（主要 / 次要关键字共用）
// 轻量设计：不维护本地副本 —— 芯片直接绑 values（模型 role），
// 增删都通过 setEntryField 交回 ViewModel，再由 dataChanged 回灌，
// 避免出现「本地列表和模型各存一份、谁覆盖谁说不清」的问题。
ColumnLayout {
    id: root

    property string label: ""
    property string entryId: ""
    property string field: ""          // "keywords" / "secondaryKeywords"
    property var values: []
    property string placeholder: "输入后回车添加，可用逗号一次输入多个"

    readonly property var keys: values || []

    spacing: Theme.spaceXs

    // 写回一次新数组；返回是否真的新增了关键字（决定要不要清空输入框）
    function commit(rawText) {
        const text = (rawText || "").trim()
        if (text.length === 0 || root.entryId.length === 0)
            return false

        const next = root.keys.slice()
        const seen = ({})
        for (const k of next)
            seen[String(k).toLowerCase()] = true

        let added = false
        for (const part of text.split(/[,，]/)) {
            const key = part.trim()
            if (key.length === 0)
                continue
            const lower = key.toLowerCase()
            if (seen[lower])        // 不区分大小写去重：匹配器本身也按这个口径比较
                continue
            seen[lower] = true
            next.push(key)
            added = true
        }

        if (!added)
            return false

        WorldInfoEntryViewModel.setEntryField(root.entryId, root.field, next)
        return true
    }

    function removeAt(index) {
        const list = root.keys.slice()
        if (index < 0 || index >= list.length)
            return

        list.splice(index, 1)
        WorldInfoEntryViewModel.setEntryField(root.entryId, root.field, list)
    }

    Label {
        Layout.fillWidth: true
        text: root.label
        font.pixelSize: Theme.fontBodySm
        font.family: Theme.fontFamily
        color: Theme.textSecondary
    }

    Flow {
        Layout.fillWidth: true
        spacing: Theme.spaceXs
        visible: root.keys.length > 0

        Repeater {
            model: root.keys

            delegate: Rectangle {
                id: chip
                required property string modelData
                required property int index

                radius: Theme.radiusSm
                color: Theme.bgSurfaceVariant
                border.width: 1
                border.color: Theme.borderDefault
                implicitWidth: chipRow.implicitWidth + Theme.spaceSm * 2
                implicitHeight: chipRow.implicitHeight + Theme.spaceXs

                RowLayout {
                    id: chipRow
                    anchors.centerIn: parent
                    spacing: Theme.spaceXs

                    Label {
                        text: chip.modelData
                        font.pixelSize: Theme.fontBodySm
                        font.family: Theme.fontFamily
                        color: Theme.textPrimary
                    }

                    Label {
                        text: "×"
                        font.pixelSize: Theme.fontBodySm
                        font.family: Theme.fontFamily
                        color: Theme.textSecondary

                        MouseArea {
                            // 负边距把点击区放大到约 20dp，点小叉子不用瞄
                            anchors.fill: parent
                            anchors.margins: -Theme.spaceXs
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.removeAt(chip.index)
                        }
                    }
                }
            }
        }
    }

    TextField {
        Layout.fillWidth: true
        implicitHeight: Theme.controlSm
        placeholderText: root.placeholder
        TextContextMenu {}
        font.pixelSize: Theme.fontBodySm
        font.family: Theme.fontFamily
        // 回车与失焦都提交（onEditingFinished 覆盖两者）；
        // 只有真的新增了才清空，重复关键字不会把输入吃掉。
        onEditingFinished: {
            if (root.commit(text))
                text = ""
        }
    }
}
