import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 世界书条目列表
// 职责：条目行的开关/名称/触发策略/插入位置/排序，以及展开后的详情编辑。
// 展开行与详情懒加载统一用 ExpandableRow（原先这套"标题行 + Loader 详情"在
// BookEntryListView 与 PromptsListView 里各复制了一遍）。
//
// 为什么不是 ListView：ListView 的 contentHeight 是"已创建委托平均高度 × 条目数"的
// 估算值，不是真实总和。展开行可以高到一两千 px，估算会随委托增删反复修正 ——
// 表现就是滚动条长度抖动（看着像忽粗忽细）、拖动手柄位置错乱、滚动范围失真。
// 这里改成 Flickable + Column + Repeater：所有行真实存在，contentHeight 精确。
// 代价是放弃虚拟化，条目很多时首次加载要创建全部标题行（展开体仍由 Loader 懒加载）。
Flickable {
    id: root

    contentWidth: width
    contentHeight: column.implicitHeight
    clip: true
    boundsBehavior: Flickable.StopAtBounds
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

            // 内容区不内嵌滚动，高度随内容增长（最小约 4 行）。
            // 原因：嵌套 Flickable 会独占滚轮 —— 实测光标落在内层时，
            // 连续 45 格滚轮外层 ListView 一次都没收到（内层到底也不让），
            // 表现就是"在内容区滚动时整个列表滚不动"。
            // 去掉内层滚动后全页只剩列表这一条滚动条，行为就正常了。
            TextArea {
                id: contentArea
                Layout.fillWidth: true
                // TextArea 的 implicitHeight 已经等于 contentHeight + 上下内边距（实测 65 / 407），
                // 直接取它即可，不用手算
                Layout.preferredHeight: Math.max(contentArea.implicitHeight,
                                                 Theme.controlLg * 2)

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
    Column {
        id: column
        width: root.width
        spacing: Theme.spaceXs

        Repeater {
            model: WorldInfoEntryViewModel.entryModel
            delegate: rowComponent
        }
    }

    Component {
        id: rowComponent

        ExpandableRow {
            id: entryRow

            // Repeater 的委托上下文里**没有** model 对象（实测 model / modelData 都取不到），
            // 用到的 role 只能逐个声明成 required property。
            // 注意 entryId 这个 role 名是为此改的（原先是 "id"，与 QML 的对象 id 概念太容易混）。
            required property string entryId
            required property string name
            required property bool isEnabled
            required property bool expandedState
            required property int triggerControl
            required property int insertPosition
            required property int insertDepth
            required property int sortOrder
            required property int scanDepth
            required property string content
            required property var keywords
            required property var secondaryKeywords
            required property bool selective
            required property int selectiveLogic

            // 行宽让出 16px 给滚动条（Material 下 ScrollBar.implicitWidth 实测 = 16，
            // 且它浮在内容之上、不占布局）。不让的话它会压住行尾的删除按钮。
            width: column.width - Theme.spaceLg
            // Column 按 height 排布、不认 implicitHeight，所以这里必须显式接上；
            // 展开动画仍由 ExpandableRow 内部对 implicitHeight 的 Behavior 驱动。
            height: implicitHeight
            headerHeight: Theme.touchTarget
            expanded: expandedState
            bodyComponent: entryDetail
            // 详情区按字段读取，这里给一份快照；任一 required property 变化都会重建。
            // 只放详情真正用到的字段，避免改标题时也白重建一次。
            bodyData: ({
                id: entryRow.entryId,
                triggerControl: entryRow.triggerControl,
                insertPosition: entryRow.insertPosition,
                insertDepth: entryRow.insertDepth,
                sortOrder: entryRow.sortOrder,
                scanDepth: entryRow.scanDepth,
                content: entryRow.content,
                keywords: entryRow.keywords,
                secondaryKeywords: entryRow.secondaryKeywords,
                selective: entryRow.selective,
                selectiveLogic: entryRow.selectiveLogic
            })

            IconButton {
                compact: true
                tip: entryRow.expanded ? "收起" : "展开"
                icon.source: entryRow.expanded
                             ? "qrc:/icons/res/circle-chevron-up.svg"
                             : "qrc:/icons/res/circle-chevron-down.svg"
                // 展开状态存在列表模型里，走 ViewModel 的命令接口写回
                onClicked: WorldInfoEntryViewModel.setEntryExpanded(entryRow.entryId,
                                                                    !entryRow.expanded)
            }

            Switch {
                checked: entryRow.isEnabled
                onToggled: WorldInfoEntryViewModel.setEntryField(entryRow.entryId, "isEnabled", checked)
            }

            TextField {
                Layout.fillWidth: true
                implicitHeight: Theme.controlSm
                text: entryRow.name || ""
                placeholderText: "标题"
                font.pixelSize: Theme.fontBody
                font.family: Theme.fontFamily
                onTextEdited: WorldInfoEntryViewModel.setEntryField(entryRow.entryId, "name", text)
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
                    entryId: entryRow.entryId
                    entryValue: entryRow.triggerControl
                }
            }

            IconButton {
                compact: true
                danger: true
                tip: "删除条目"
                icon.source: "qrc:/icons/res/trash.svg"
                onClicked: WorldInfoEntryViewModel.deleteEntry(entryRow.entryId)
            }
        }
    }
}
