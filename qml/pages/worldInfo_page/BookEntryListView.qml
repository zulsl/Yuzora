import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 世界书条目列表
// 职责：条目行的开关/名称/触发策略/插入位置/排序，以及展开后的详情编辑。
// 展开行与详情懒加载统一用 ExpandableRow（原先这套"标题行 + Loader 详情"在
// BookEntryListView 与 PromptsListView 里各复制了一遍）。
// 注意：原先用 Layout.preferredHeight: contentHeight + interactive: false 嵌在外层滚动里，
//      两层滚动互相打架；这里改为自身 fillHeight 独立滚动。
ListView {
    id: root

    clip: true
    spacing: Theme.spaceXs
    model: WorldInfoEntryViewModel.entryModel
    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

    // ---------- 展开后的详情 ----------
    Component {
        id: entryDetail

        ColumnLayout {
            // 实例根节点必须有自己的 id：组件内部要用它取 bodyData。
            // 注意不能写 entryRoot.bodyData —— entryDetail 是 Component 对象，没有这个属性，
            // 那样取到的永远是 undefined（文本会全部显示为空）。
            id: entryRoot

            // ExpandableRow 会把这个属性设为当前 delegate 的 model
            property var bodyData

            spacing: Theme.spaceSm

            Label {
                text: "内容"
                font.pixelSize: Theme.fontBodySm
                font.family: Theme.fontFamily
                color: Theme.textSecondary
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.controlLg * 5
                clip: true

                TextArea {
                    id: contentArea
                    textFormat: TextEdit.PlainText
                    wrapMode: TextArea.Wrap
                    placeholderText: "条目内容"
                    font.pixelSize: Theme.fontBody
                    font.family: Theme.fontFamily
                    color: Theme.textPrimary
                    text: entryRoot.bodyData ? (entryRoot.bodyData.content || "") : ""
                    onTextChanged: {
                        if (activeFocus && entryRoot.bodyData)
                            WorldInfoEntryViewModel.setEntryField(entryRoot.bodyData.id,
                                                                  "content", text)
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                Label {
                    text: "扫描深度"
                    font.pixelSize: Theme.fontBodySm
                    font.family: Theme.fontFamily
                    color: Theme.textSecondary
                }

                SpinBox {
                    from: -1
                    to: 1000
                    stepSize: 1
                    editable: true
                    value: entryRoot.bodyData ? entryRoot.bodyData.scanDepth : -1
                    onValueModified: {
                        if (entryRoot.bodyData)
                            WorldInfoEntryViewModel.setEntryField(entryRoot.bodyData.id,
                                                                  "scanDepth", value)
                    }
                }

                Item { Layout.fillWidth: true }
            }
        }
    }

    // ---------- 条目行 ----------
    delegate: ExpandableRow {
        id: entryRow
        width: ListView.view.width
        headerHeight: Theme.touchTarget
        expanded: model.expandedState
        bodyComponent: entryDetail
        bodyData: model

        // 数据行对象单独存一份。
        // 原因：ComboBox 自己也有 model 属性，子项里一旦写 `model: [...]`，
        // 就会把委托上下文里的 `model`（数据行）遮蔽掉，导致 `model.triggerControl`
        // 变成 undefined → "Unable to assign [undefined] to int"。
        readonly property var row: model

        IconButton {
            compact: true
            tip: model.expandedState ? "收起" : "展开"
            icon.source: model.expandedState
                         ? "qrc:/icons/res/circle-chevron-up.svg"
                         : "qrc:/icons/res/circle-chevron-down.svg"
            onClicked: model.expandedState = !model.expandedState
        }

        Switch {
            checked: model.isEnabled
            onToggled: WorldInfoEntryViewModel.setEntryField(model.id, "isEnabled", checked)
        }

        TextField {
            Layout.fillWidth: true
            implicitHeight: Theme.controlSm
            text: model.name || ""
            placeholderText: "关键字（逗号分隔）"
            font.pixelSize: Theme.fontBody
            font.family: Theme.fontFamily
            onTextEdited: WorldInfoEntryViewModel.setEntryField(model.id, "name", text)
        }

        ComboBox {
            Layout.preferredWidth: Theme.controlLg * 2
            implicitHeight: Theme.controlSm
            model: ["始终", "关键词", "向量化"]
            currentIndex: entryRow.row.triggerControl
            onActivated: (index) => WorldInfoEntryViewModel.setEntryField(model.id, "triggerControl", index)
        }

        // 窄窗口下这两个下拉/数字太长，隐藏后仍可在详情里查看
        ComboBox {
            visible: !Theme.isCompact
            Layout.preferredWidth: Theme.controlLg * 3.5
            implicitHeight: Theme.controlSm
            model: [
                "角色定义之前", "角色定义之后", "示例消息之前", "示例消息之后",
                "作者注释之前", "作者注释之后", "@D[系统]在深度", "@D[用户]在深度", "@D[AI]在深度"
            ]
            currentIndex: entryRow.row.insertPosition
            onActivated: (index) => WorldInfoEntryViewModel.setEntryField(model.id, "insertPosition", index)
        }

        SpinBox {
            visible: !Theme.isCompact && entryRow.expanded === false
                    && entryRow.row.insertPosition >= 6
                    && entryRow.row.insertPosition <= 8
            implicitHeight: Theme.controlSm
            from: 0
            to: 1000000
            editable: true
            value: entryRow.row.insertDepth
            onValueModified: WorldInfoEntryViewModel.setEntryField(model.id, "insertDepth", value)
        }

        SpinBox {
            visible: !Theme.isCompact
            implicitHeight: Theme.controlSm
            from: 0
            to: 1000000
            editable: true
            value: model.sortOrder
            onValueModified: WorldInfoEntryViewModel.setEntryField(model.id, "sortOrder", value)
        }

        IconButton {
            compact: true
            danger: true
            tip: "删除条目"
            icon.source: "qrc:/icons/res/trash.svg"
            onClicked: WorldInfoEntryViewModel.deleteEntry(model.id)
        }
    }
}
