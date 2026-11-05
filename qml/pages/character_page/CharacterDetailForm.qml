import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 角色详情表单
// 职责：编辑当前角色的各个字段，并把结果整体交回 ViewModel。
// 关键修复：改造前所有输入框都是单向绑定（text: editingCharacter.x），
//          没有任何回写，点"保存"提交的是未改动的副本 —— 角色编辑实际不生效。
//          现在用 onTextChanged + activeFocus 守卫回写到编辑副本。
// 根节点用 Item 包一层，便于被 AdaptiveSplit 用 LayoutItemProxy 搬运。
Item {
    id: root

    implicitWidth: 480
    implicitHeight: 600

    // Q_GADGET 值类型只能用 var 承接（QML 不接受 QML_VALUE_TYPE 注册名做类型注解）
    property var editingCharacter: CharacterViewModel.currentCharacter

    signal exportRequested()
    signal bookRequested()

    // 统一回写：只在用户正在编辑该字段时写回，避免模型刷新把编辑内容冲掉
    function commit(field, value) {
        if (!editingCharacter) return
        if (editingCharacter[field] === value) return
        editingCharacter[field] = value
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: Theme.spaceMd

            // ========== 操作栏 ==========
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                Label {
                    Layout.fillWidth: true
                    text: "角色详情"
                    font.pixelSize: Theme.fontTitle
                    font.bold: true
                    font.family: Theme.fontFamily
                    color: Theme.textPrimary
                }

                IconButton {
                    tip: "角色世界书"
                    icon.source: "qrc:/icons/res/main_page/book-2.svg"
                    onClicked: characterBookDialog.open()
                }
                IconButton {
                    tip: "删除角色"
                    danger: true
                    icon.source: "qrc:/icons/res/trash.svg"
                    enabled: CharacterViewModel.hasSelection
                    onClicked: characterDeleteDialog.open()
                }
                IconButton {
                    tip: "导出角色卡"
                    icon.source: "qrc:/icons/res/file-export.svg"
                    enabled: CharacterViewModel.hasSelection
                    onClicked: root.exportRequested()
                }
                IconButton {
                    tip: "保存"
                    icon.source: "qrc:/icons/res/save.svg"
                    enabled: CharacterViewModel.hasSelection
                    onClicked: {
                        CharacterViewModel.saveCurrentCharacter(root.editingCharacter)
                        Ui.toast("角色已保存", false)
                    }
                }
            }

            // ========== 基础字段 ==========
            LabeledField {
                label: "角色名称"
                TextField {
                    Layout.fillWidth: true
                    implicitHeight: Theme.controlMd
                    placeholderText: "角色名称 *"
                    text: root.editingCharacter ? (root.editingCharacter.name || "") : ""
                    onTextChanged: if (activeFocus) root.commit("name", text)
                }
            }

            LabeledField {
                label: "作者"
                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        placeholderText: "作者"
                        text: root.editingCharacter ? (root.editingCharacter.creator || "") : ""
                        onTextChanged: if (activeFocus) root.commit("creator", text)
                    }
                    TextField {
                        Layout.preferredWidth: Theme.controlMd * 3
                        implicitHeight: Theme.controlMd
                        placeholderText: "版本号"
                        text: root.editingCharacter ? (root.editingCharacter.characterVersion || "") : ""
                        onTextChanged: if (activeFocus) root.commit("characterVersion", text)
                    }
                }
            }

            // 长文本区（描述 / 开场白 / 备选开场白 / 作者备注）
            Repeater {
                model: [
                    { field: "description", label: "角色描述（系统提示词）", hint: "", height: Theme.controlLg * 7 },
                    { field: "firstMessage", label: "初始开场白", hint: "", height: Theme.controlLg * 7 },
                    { field: "creatorNotes", label: "作者备注", hint: "", height: Theme.controlLg * 3 }
                ]

                ColumnLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: Theme.spaceXs

                    Label {
                        text: modelData.label
                        font.pixelSize: Theme.fontBodySm
                        font.family: Theme.fontFamily
                        color: Theme.textSecondary
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: modelData.height
                        clip: true

                        TextArea {
                            placeholderText: modelData.hint
                            wrapMode: Text.WordWrap
                            textFormat: TextEdit.PlainText
                            font.pixelSize: Theme.fontBody
                            font.family: Theme.fontFamily
                            color: Theme.textPrimary
                            text: root.editingCharacter
                                  ? (root.editingCharacter[modelData.field] || "") : ""
                            onTextChanged: {
                                if (activeFocus)
                                    root.commit(modelData.field, text)
                            }
                        }
                    }
                }
            }

            // 备选开场白：多行文本 ↔ 字符串数组
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceXs

                Label {
                    text: "备选开场白（每行一个）"
                    font.pixelSize: Theme.fontBodySm
                    font.family: Theme.fontFamily
                    color: Theme.textSecondary
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Theme.controlLg * 5
                    clip: true

                    TextArea {
                        placeholderText: "每行一个备选开场白"
                        wrapMode: Text.WordWrap
                        textFormat: TextEdit.PlainText
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily
                        color: Theme.textPrimary
                        text: root.editingCharacter
                              ? (root.editingCharacter.alternateGreetings || []).join("\n") : ""
                        onTextChanged: {
                            if (!activeFocus) return
                            root.commit("alternateGreetings",
                                        text.split("\n").filter(line => line.trim().length > 0))
                        }
                    }
                }
            }

            // ========== 高级定义 ==========
            AppButton {
                Layout.fillWidth: true
                variant: "ghost"
                text: extraForm.expanded ? "收起高级定义 ▲" : "更多高级定义 ▼"
                onClicked: extraForm.expanded = !extraForm.expanded
            }

            // 高级定义：折叠时**不创建**内部控件。
            // 这 4 个 TextArea 如果常驻，每次切换角色都要把它们的整段文本重新排版，
            // 是"选中角色卡一下"的主要来源（无文本的角色因此不会卡）。
            ColumnLayout {
                id: extraForm
                Layout.fillWidth: true
                layoutDirection: Qt.LeftToRight
                spacing: 0
                clip: true
                property bool expanded: false

                Layout.preferredHeight: expanded ? advancedLoader.implicitHeight : 0
                Behavior on Layout.preferredHeight {
                    NumberAnimation {
                        duration: Theme.durationBase
                        easing.type: Theme.easingOut
                    }
                }

                Loader {
                    id: advancedLoader
                    Layout.fillWidth: true
                    active: extraForm.expanded
                    sourceComponent: advancedFields
                    opacity: extraForm.expanded ? 1 : 0
                    Behavior on opacity {
                        NumberAnimation { duration: Theme.durationBase }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }

    // ========== 高级定义的内容（懒加载：折叠时完全不存在）==========
    Component {
        id: advancedFields

        ColumnLayout {
            spacing: Theme.spaceMd

            Repeater {
                model: [
                    { field: "personality", label: "性格特征", height: Theme.controlLg * 2.5 },
                    { field: "scenario", label: "对话场景设定", height: Theme.controlLg * 2.5 },
                    { field: "exampleDialogue", label: "示例对话", height: Theme.controlLg * 4 },
                    { field: "systemPrompt", label: "角色专属系统提示词", height: Theme.controlLg * 3 }
                ]

                ColumnLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: Theme.spaceXs

                    Label {
                        text: modelData.label
                        font.pixelSize: Theme.fontBodySm
                        font.family: Theme.fontFamily
                        color: Theme.textSecondary
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: modelData.height
                        clip: true

                        TextArea {
                            wrapMode: Text.WordWrap
                            textFormat: TextEdit.PlainText
                            font.pixelSize: Theme.fontBody
                            font.family: Theme.fontFamily
                            color: Theme.textPrimary
                            text: root.editingCharacter
                                  ? (root.editingCharacter[modelData.field] || "") : ""
                            onTextChanged: {
                                if (activeFocus)
                                    root.commit(modelData.field, text)
                            }
                        }
                    }
                }
            }

            LabeledField {
                label: "标签"
                TextField {
                    Layout.fillWidth: true
                    implicitHeight: Theme.controlMd
                    placeholderText: "英文逗号分隔"
                    text: root.editingCharacter
                          ? (root.editingCharacter.tags || []).join(", ") : ""
                    onTextChanged: {
                        if (!activeFocus) return
                        root.commit("tags",
                                    text.split(",").map(t => t.trim())
                                        .filter(t => t.length > 0))
                    }
                }
            }
        }
    }

    // ========== 对话框 ==========
    ConfirmDialog {
        id: characterDeleteDialog
        title: "确认删除"
        text: "确定要删除该角色吗？关联的会话数据不会被删除。"
        acceptText: "删除"
        danger: true
        onConfirmed: CharacterViewModel.deleteCurrentCharacter()
    }

    Dialog {
        id: characterBookDialog
        title: "角色世界书"
        modal: true
        anchors.centerIn: Overlay.overlay
        width: Math.min(parent ? parent.width - Theme.spaceXl * 2 : 360, 420)
        padding: Theme.spaceLg

        background: Rectangle {
            color: Theme.bgElevated
            radius: Theme.radiusMd
            border.width: 1
            border.color: Theme.borderDefault
        }

        contentItem: ColumnLayout {
            spacing: Theme.spaceSm

            Label {
                Layout.fillWidth: true
                text: "使用该角色聊天时会自动启用以下世界书"
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontBody
                font.family: Theme.fontFamily
                color: Theme.textPrimary
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: Theme.controlLg
                radius: Theme.radiusSm
                color: Theme.bgSurfaceVariant
                border.width: 1
                border.color: Theme.borderDefault

                Label {
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.spaceMd
                    anchors.verticalCenter: parent.verticalCenter
                    text: CharacterViewModel.characterBookName
                    font.pixelSize: Theme.fontBody
                    font.family: Theme.fontFamily
                    color: Theme.textPrimary
                    elide: Text.ElideRight
                }
            }
        }

        footer: Item {
            implicitHeight: Theme.touchTarget + Theme.spaceSm
            AppButton {
                anchors.right: parent.right
                anchors.rightMargin: Theme.spaceLg
                anchors.verticalCenter: parent.verticalCenter
                text: "关闭"
                onClicked: characterBookDialog.close()
            }
        }
    }
}
