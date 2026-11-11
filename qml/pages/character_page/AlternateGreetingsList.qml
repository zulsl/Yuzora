import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 备选开场白列表
// 职责：以 ListView 管理多条备选开场白；每条可独立展开/收起，支持末尾添加与确认删除。
// 数据边界：只编辑 local draft，通过 greetingsEdited(values) 交回父级，由父级决定何时落库。
ColumnLayout {
    id: root

    // 父级传入的当前草稿，仅用于切换角色时重建列表
    property var greetings: []
    property string recordId: ""
    property int maxLines: 4

    signal greetingsEdited(var values)

    property int pendingDeleteIndex: -1
    property string pendingDeletePreview: ""

    spacing: Theme.spaceXs

    ListModel {
        id: greetingModel
    }

    function rebuildFromGreetings() {
        greetingModel.clear()
        const source = greetings || []
        for (let i = 0; i < source.length; ++i) {
            greetingModel.append({
                text: source[i] || "",
                expanded: false
            })
        }
        list.currentIndex = -1
    }

    function emitGreetings() {
        const values = []
        for (let i = 0; i < greetingModel.count; ++i) {
            const value = greetingModel.get(i).text
            if (value.trim().length > 0)
                values.push(value)
        }

        greetingsEdited(values)
    }

    function addGreeting() {
        const index = greetingModel.count
        greetingModel.append({ text: "", expanded: true })
        list.currentIndex = index
        list.positionViewAtEnd()
        emitGreetings()

        Qt.callLater(function() {
            const item = list.itemAtIndex(index)
            if (item && item.focusEditor)
                item.focusEditor()
        })
    }

    function requestDelete(index) {
        if (index < 0 || index >= greetingModel.count)
            return

        pendingDeleteIndex = index
        const text = greetingModel.get(index).text
        pendingDeletePreview = text.length > 24
                              ? text.substring(0, 24) + "…" : text
        deleteDialog.open()
    }

    function confirmDelete() {
        if (pendingDeleteIndex < 0 || pendingDeleteIndex >= greetingModel.count)
            return

        const removedIndex = pendingDeleteIndex
        greetingModel.remove(removedIndex)
        pendingDeleteIndex = -1
        pendingDeletePreview = ""
        list.currentIndex = Math.min(removedIndex, greetingModel.count - 1)
        emitGreetings()
    }

    onRecordIdChanged: Qt.callLater(rebuildFromGreetings)
    Component.onCompleted: Qt.callLater(rebuildFromGreetings)

    Label {
        Layout.fillWidth: true
        text: "备选开场白"
        font.pixelSize: Theme.fontBodySm
        font.family: Theme.fontFamily
        color: Theme.textSecondary
    }

    ListView {
        id: list
        Layout.fillWidth: true
        Layout.preferredHeight: contentHeight
        interactive: false
        clip: false
        spacing: Theme.spaceXs
        model: greetingModel

        delegate: Item {
            id: delegateRoot
            required property int index
            required property string text
            required property bool expanded

            width: ListView.view.width
            height: contentColumn.implicitHeight + Theme.spaceSm * 2

            // 折叠态只排版截断后的一行。直接把整段文本交给 Text 再 elide，
            // Qt 仍然会先完成整段文本的排版，10 条长开场白会白付一次成本。
            readonly property string previewText: text.length > 160
                                                  ? text.substring(0, 160) : text
            readonly property real minHeight: Theme.controlMd
            readonly property real maxHeight: Math.max(minHeight,
                itemFontMetrics.height * root.maxLines + Theme.spaceSm * 2)

            // addGreeting() 可能在编辑器 Loader 完成前请求聚焦，先记下来
            property bool focusPending: false
            onExpandedChanged: if (!expanded) focusPending = false

            function focusEditor() {
                if (editorLoader.item)
                    editorLoader.item.textArea.forceActiveFocus()
                else
                    focusPending = true
            }

            Rectangle {
                anchors.fill: parent
                radius: Theme.radiusSm
                color: Theme.bgSurface
                border.width: 1
                border.color: Theme.borderDefault
            }

            ColumnLayout {
                id: contentColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: Theme.spaceSm
                spacing: Theme.spaceXs

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceXs

                    IconButton {
                        compact: true
                        tip: delegateRoot.expanded ? "收起" : "展开"
                        icon.source: delegateRoot.expanded
                                     ? "qrc:/icons/res/circle-chevron-up.svg"
                                     : "qrc:/icons/res/circle-chevron-down.svg"
                        onClicked: greetingModel.setProperty(delegateRoot.index,
                                                            "expanded",
                                                            !delegateRoot.expanded)
                    }

                    Label {
                        text: "开场白 " + (delegateRoot.index + 1)
                        font.pixelSize: Theme.fontBodySm
                        font.family: Theme.fontFamily
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: !delegateRoot.expanded
                        text: delegateRoot.previewText.length > 0
                              ? delegateRoot.previewText : "空开场白"
                        font.pixelSize: Theme.fontBodySm
                        font.family: Theme.fontFamily
                        color: delegateRoot.text.length > 0
                               ? Theme.textSecondary : Theme.textDisabled
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }

                    // 折叠时由预览文本占满剩余宽度；展开后预览隐藏，
                    // 这里必须补一个弹性占位，否则删除按钮会向左滑到标题旁边。
                    Item {
                        Layout.fillWidth: true
                        visible: delegateRoot.expanded
                    }

                    IconButton {
                        compact: true
                        danger: true
                        tip: "删除开场白"
                        icon.source: "qrc:/icons/res/trash.svg"
                        onClicked: root.requestDelete(delegateRoot.index)
                    }
                }

                Loader {
                    id: editorLoader
                    // 折叠时不创建 TextArea。隐藏的 TextArea 仍会同步排版全文，
                    // 之前第三个角色的 10 条长开场白就是在这里被全部排版的。
                    active: delegateRoot.expanded
                    visible: active
                    Layout.fillWidth: true
                    Layout.preferredHeight: {
                        if (!active || !item)
                            return 0

                        return Math.min(delegateRoot.maxHeight,
                            Math.max(delegateRoot.minHeight,
                                     item.naturalHeight))
                    }
                    sourceComponent: editorComponent

                    onLoaded: {
                        item.textArea.text = delegateRoot.text
                        // Component 是独立作用域，不能直接引用 delegateRoot；
                        // 文本改动通过信号回到这里，再写回当前行对应的 model。
                        item.textEdited.connect(function(value) {
                            greetingModel.setProperty(delegateRoot.index, "text", value)
                            root.emitGreetings()
                        })
                        if (delegateRoot.focusPending) {
                            item.textArea.forceActiveFocus()
                            delegateRoot.focusPending = false
                        }
                    }
                }
            }

            FontMetrics {
                id: itemFontMetrics
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
            }
        }
    }

    // 单条开场白的编辑器：只有展开时才由 Loader 实例化
    Component {
        id: editorComponent

        ScrollView {
            id: editorScroll
            width: parent ? parent.width : 0
            contentWidth: availableWidth
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            property alias textArea: area
            readonly property real naturalHeight:
                area.contentHeight + area.topPadding + area.bottomPadding

            signal textEdited(string value)

            TextArea {
                id: area
                width: Math.max(0, editorScroll.availableWidth - Theme.spaceMd)
                placeholderText: "输入备选开场白"
                wrapMode: Text.WordWrap
                textFormat: TextEdit.PlainText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
                color: Theme.textPrimary
                onTextChanged: {
                    if (activeFocus)
                        editorScroll.textEdited(text)
                }
            }
        }
    }

    Text {
        Layout.fillWidth: true
        visible: greetingModel.count === 0
        text: "暂无备选开场白"
        font.pixelSize: Theme.fontBodySm
        font.family: Theme.fontFamily
        color: Theme.textSecondary
    }

    AppButton {
        Layout.fillWidth: true
        text: "添加开场白"
        iconSource: "qrc:/icons/res/plus.svg"
        onClicked: root.addGreeting()
    }

    ConfirmDialog {
        id: deleteDialog
        title: "确认删除"
        text: root.pendingDeletePreview.length > 0
              ? "确定要删除开场白 " + (root.pendingDeleteIndex + 1)
                + "：“" + root.pendingDeletePreview + "”吗？"
              : "确定要删除开场白 " + (root.pendingDeleteIndex + 1) + " 吗？"
        acceptText: "删除"
        danger: true
        onConfirmed: root.confirmDelete()
    }
}
