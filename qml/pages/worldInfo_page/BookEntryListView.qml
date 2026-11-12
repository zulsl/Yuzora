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

            // ---------- 逐条设置 ----------
            // 触发策略 / 插入位置在宽态位于标题行，窄态才落到这里；
            // 插入深度 / 插入顺序两态都在这里。
            // 字段本体各自只定义一次（同目录 TriggerStrategyComboBox /
            // InsertPositionComboBox / EntryNumberField），这里只负责摆放与可见性。
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceXs

                LabeledField {
                    label: "触发策略"
                    visible: Theme.isCompact
                    TriggerStrategyComboBox {
                        Layout.fillWidth: true
                        entryId: entryRoot.bodyData ? entryRoot.bodyData.id : ""
                        entryValue: entryRoot.bodyData ? entryRoot.bodyData.triggerControl : 0
                    }
                }

                LabeledField {
                    label: "插入位置"
                    InsertPositionComboBox {
                        Layout.fillWidth: true
                        entryId: entryRoot.bodyData ? entryRoot.bodyData.id : ""
                        entryValue: entryRoot.bodyData ? entryRoot.bodyData.insertPosition : 0
                    }
                }

                LabeledField {
                    label: "插入深度"
                    // 只有插入位置是 "@D[...] 在深度"（索引 6~8）时才谈得上深度；
                    // !! 是为了在 bodyData 还没赋值的首帧拿到确定的 false，
                    // 否则表达式求值成 null 赋给 visible，会留下类型告警
                    visible: !!entryRoot.bodyData
                             && entryRoot.bodyData.insertPosition >= 6
                             && entryRoot.bodyData.insertPosition <= 8
                    EntryNumberField {
                        Layout.fillWidth: true
                        entryId: entryRoot.bodyData ? entryRoot.bodyData.id : ""
                        field: "insertDepth"
                        entryValue: entryRoot.bodyData ? entryRoot.bodyData.insertDepth : 0
                    }
                }

                LabeledField {
                    label: "插入顺序"
                    EntryNumberField {
                        Layout.fillWidth: true
                        entryId: entryRoot.bodyData ? entryRoot.bodyData.id : ""
                        field: "sortOrder"
                        entryValue: entryRoot.bodyData ? entryRoot.bodyData.sortOrder : 0
                    }
                }
            }

            // ---------- 关键字：主 → 次关键字逻辑 → 次 ----------
            EntryKeywordList {
                Layout.fillWidth: true
                label: "主要关键字"
                entryId: entryRoot.bodyData ? entryRoot.bodyData.id : ""
                field: "keywords"
                values: entryRoot.bodyData ? (entryRoot.bodyData.keywords || []) : []
            }

            // 触发策略是「关键词」却没有主关键字时，该条目永远不会命中
            Label {
                Layout.fillWidth: true
                visible: !!entryRoot.bodyData
                         && entryRoot.bodyData.triggerControl === 1
                         && (entryRoot.bodyData.keywords || []).length === 0
                text: "主要关键字为空：触发策略为「关键词」，该条目不会被触发"
                font.pixelSize: Theme.fontBodySm
                font.family: Theme.fontFamily
                color: Theme.warning
                wrapMode: Text.WordWrap
            }

            EntrySelectiveLogicComboBox {
                Layout.fillWidth: true
                entryId: entryRoot.bodyData ? entryRoot.bodyData.id : ""
                selective: entryRoot.bodyData ? entryRoot.bodyData.selective : true
                logic: entryRoot.bodyData ? entryRoot.bodyData.selectiveLogic : 0
            }

            EntryKeywordList {
                Layout.fillWidth: true
                label: "次要关键字"
                entryId: entryRoot.bodyData ? entryRoot.bodyData.id : ""
                field: "secondaryKeywords"
                values: entryRoot.bodyData ? (entryRoot.bodyData.secondaryKeywords || []) : []
            }

            Label {
                Layout.fillWidth: true
                visible: !!entryRoot.bodyData
                         && entryRoot.bodyData.selective
                         && (entryRoot.bodyData.secondaryKeywords || []).length > 0
                text: "次要关键字在主要关键字命中之后才判断，组合方式由上面的逻辑决定"
                font.pixelSize: Theme.fontCaption
                font.family: Theme.fontFamily
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.controlLg * 7
                clip: true

                TextArea {
                    id: contentArea
                    textFormat: TextEdit.PlainText
                    wrapMode: TextArea.Wrap
                    placeholderText: "内容"
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

            LabeledField {
                label: "扫描深度"
                EntryNumberField {
                    Layout.fillWidth: true
                    from: -1
                    to: 1000
                    entryId: entryRoot.bodyData ? entryRoot.bodyData.id : ""
                    field: "scanDepth"
                    entryValue: entryRoot.bodyData ? entryRoot.bodyData.scanDepth : -1
                }
            }
        }
    }

    // ---------- 条目行 ----------
    delegate: ExpandableRow {
        id: entryRow
        // 行宽让出 16px 给列表滚动条（Material 下 ScrollBar.implicitWidth 实测 = 16，
        // 且它浮在内容之上、不占布局）。不让的话它会压住行尾的删除按钮，
        // 展开后还会和内容框自己的滚动条叠在同一条右边缘上。
        width: ListView.view.width - Theme.spaceLg
        headerHeight: Theme.touchTarget
        expanded: model.expandedState
        bodyComponent: entryDetail
        bodyData: model

        // 数据行对象单独存一份。
        // 原因：下面那些字段的根节点是 ComboBox，而 ComboBox 自己就有 model 属性。
        // 在 ComboBox 上写 `entryId: model.id` 时，这个 model 会解析到 ComboBox
        // 自己的 model（选项数组）而不是委托上下文的数据行，取到 undefined，
        // 于是刷出 "Unable to assign [undefined] to QString / int"。
        // 走 row.xxx 才能拿到真正的条目数据。
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
            placeholderText: "标题"
            font.pixelSize: Theme.fontBody
            font.family: Theme.fontFamily
            onTextEdited: WorldInfoEntryViewModel.setEntryField(model.id, "name", text)
        }

        RowLayout {
            visible: !Theme.isCompact
            Item { Layout.preferredWidth: 5 }
            Label{
                text: "触发策略"
                font.pixelSize: Theme.fontBody
                font.family: Theme.fontFamily
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
                Layout.preferredWidth: 60
            }
            TriggerStrategyComboBox {// 触发策略（窄态搬进详情）
                // 内容区 = 宽度 - leftPadding(16) - 箭头区(40)。
                // 「向量化」的自然需求是 16+图标20+间距4+文字42+箭头40 ≈ 122，
                // 132 留一点余量，避免三字选项被省略号截断。
                Layout.preferredWidth: Theme.controlLg * 2.75
                entryId: entryRow.row ? entryRow.row.id : ""
                entryValue: entryRow.row ? entryRow.row.triggerControl : 0
            }
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
