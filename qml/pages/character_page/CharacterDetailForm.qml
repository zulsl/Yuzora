import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

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

    signal exportRequested()

    function fieldValue(field) {
        const draft = CharacterViewModel.characterDraft
        return draft ? draft.fieldValue(field) : ""
    }

    ScrollView {
        id: formScroll
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            // 给右侧滚动条留出空间，避免滚动条压在输入框/文字上
            width: formScroll.availableWidth - Theme.spaceLg
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
                    tip: "角色世界书绑定"
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
                        CharacterViewModel.saveCharacterDraft()
                    }
                }
            }

            // ========== 角色档案 ==========
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: profileLayout.implicitHeight + Theme.spaceMd * 2
                radius: Theme.radiusMd
                color: Theme.bgSurface
                border.width: 1
                border.color: Theme.borderDefault

                RowLayout {
                    id: profileLayout
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: Theme.spaceMd
                    height: implicitHeight
                    spacing: Theme.spaceLg

                    Item {
                        Layout.preferredWidth: Theme.avatarLg
                        Layout.preferredHeight: Theme.avatarLg
                        Layout.alignment: Qt.AlignTop

                        CircleAvatar {
                            anchors.fill: parent
                            size: Theme.avatarLg
                            source: CharacterViewModel.currentAvatarUrl
                            fallbackText: root.fieldValue("name").length > 0
                                          ? root.fieldValue("name").charAt(0) : ""
                        }

                        CameraMask{
                            onMaskClicked : avatarDialog.open()
                        }
                    }


                    TextField {
                        Layout.fillWidth: true
                        implicitHeight: Theme.controlMd
                        placeholderText: "角色名称"
                        TextContextMenu {}
                        // 强类型草稿：QML 直接读写 typed property，setter 负责发通知
                        text: CharacterViewModel.characterDraft
                              ? CharacterViewModel.characterDraft.name : ""
                        onTextChanged: if (activeFocus && CharacterViewModel.characterDraft)
                            CharacterViewModel.characterDraft.name = text
                    }
                }
            }

            // 长文本区（描述 / 开场白）
            Repeater {
                model: [
                    { field: "description", label: "角色描述（系统提示词）", hint: "", maxLines: 8 },
                    { field: "firstMessage", label: "首选开场白", hint: "", maxLines: 8 },
                ]

                // 超过阈值的文本会退化为"只读预览 + 点开编辑"，
                // 避免几万字在每次切换角色时被整段排版（实测 37k 字 ≈ 330 ms）
                LongTextField {
                    required property var modelData
                    Layout.fillWidth: true
                    recordId: root.fieldValue("id")
                    label: modelData.label
                    placeholder: modelData.hint
                    maxLines: modelData.maxLines
                    content: root.fieldValue(modelData.field)
                    onEdited: (value) => CharacterViewModel.setCharacterDraftField(modelData.field, value)
                }
            }

            // 备选开场白：一条一个 ListView 项，支持折叠和确认删除
            AlternateGreetingsList {
                Layout.fillWidth: true
                recordId: root.fieldValue("id")
                greetings: CharacterViewModel.characterDraft
                           ? (CharacterViewModel.characterDraft.alternateGreetings || []) : []
                onGreetingsEdited: (values) => CharacterViewModel.setCharacterDraftField("alternateGreetings", values)
            }

            // ========== 其他字段 ==========
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    Layout.fillWidth: true
                    implicitHeight: Theme.controlMd
                    placeholderText: "作者"
                    TextContextMenu {}
                    text: root.fieldValue("creator")
                    onTextChanged: if (activeFocus) CharacterViewModel.setCharacterDraftField("creator", text)
                }
                TextField {
                    Layout.preferredWidth: Theme.controlMd * 3
                    implicitHeight: Theme.controlMd
                    placeholderText: "版本号"
                    TextContextMenu {}
                    text: root.fieldValue("characterVersion")
                    onTextChanged: if (activeFocus) CharacterViewModel.setCharacterDraftField("characterVersion", text)
                }
            }
            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                TextArea {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    placeholderText: "作者备注(不进提示词)"
                    TextContextMenu {}
                    text: root.fieldValue("creatorNotes")
                    onTextChanged: if (activeFocus) CharacterViewModel.setCharacterDraftField("creatorNotes", text)
                    wrapMode: Text.WordWrap
                }
            }

            // ========== 高级定义 ==========
            AppButton {
                Layout.fillWidth: true
                variant: "ghost"
                text: extraForm.expanded ? "收起高级定义 ▲" : "更多高级定义 ▼"
                onClicked: {
                    extraForm.animateExtraHeight = true
                    extraForm.expanded = !extraForm.expanded
                    extraHeightTimer.restart()
                }
            }

            // 高级定义：折叠时**不创建**内部控件。
            // 这 4 个 TextArea 如果常驻，每次切换角色都要把它们的整段文本重新排版，
            // 是"选中角色卡一下"的主要来源（无文本的角色因此不会卡）。
            Label {
                Layout.fillWidth: true
                text: "以下为测试功能"
            }
            ColumnLayout {
                id: extraForm
                Layout.fillWidth: true
                layoutDirection: Qt.LeftToRight
                spacing: 0
                clip: true
                property bool expanded: false
                // 只在展开/收起时允许高度动画；输入导致的内容高度变化直接生效
                property bool animateExtraHeight: false

                Layout.preferredHeight: expanded ? advancedLoader.implicitHeight : 0
                Behavior on Layout.preferredHeight {
                    enabled: extraForm.animateExtraHeight
                    NumberAnimation {
                        duration: Theme.durationBase
                        easing.type: Theme.easingOut
                    }
                }

                Timer {
                    id: extraHeightTimer
                    interval: Theme.durationBase + 32
                    repeat: false
                    onTriggered: extraForm.animateExtraHeight = false
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
                    { field: "personality", label: "性格特征", maxLines: 4 },
                    { field: "scenario", label: "对话场景设定", maxLines: 4 },
                    { field: "exampleDialogue", label: "示例对话", maxLines: 6 },
                    { field: "systemPrompt", label: "角色专属系统提示词(该提示词会覆盖预设的主提示词,谨慎输入)", maxLines: 4 }
                ]

                LongTextField {
                    required property var modelData
                    Layout.fillWidth: true
                    recordId: root.fieldValue("id")
                    label: modelData.label
                    maxLines: modelData.maxLines
                    content: root.fieldValue(modelData.field)
                    onEdited: (value) => CharacterViewModel.setCharacterDraftField(modelData.field, value)
                }
            }

            LabeledField {
                label: "标签"
                TextField {
                    Layout.fillWidth: true
                    implicitHeight: Theme.controlMd
                    placeholderText: "英文逗号分隔"
                    TextContextMenu {}
                    text: (root.fieldValue("tags") || []).join(", ")
                    onTextChanged: {
                        if (!activeFocus) return
                        CharacterViewModel.setCharacterDraftField("tags",
                                    text.split(",").map(t => t.trim())
                                        .filter(t => t.length > 0))
                    }
                }
            }
        }
    }

    // ========== 对话框 ==========
    FileDialog {
        id: avatarDialog
        title: "选择角色头像"
        nameFilters: ["图片文件 (*.png *.jpg *.jpeg *.webp *.bmp)"]
        fileMode: FileDialog.OpenFile
        onAccepted: CharacterViewModel.updateAvatar(selectedFile)
    }

    ConfirmDialog {
        id: characterDeleteDialog
        title: "确认删除"
        text: "确定要删除该角色吗？关联的会话数据不会被删除。"
        acceptText: "删除"
        danger: true
        onConfirmed: CharacterViewModel.deleteCurrentCharacter()
    }

    // 世界书绑定管理：解绑 / 无绑定时挑一本。
    // 逻辑整体抽到独立文件（表单已接近 300 行上限），绑定写草稿、随「保存」提交。
    CharacterBookDialog {
        id: characterBookDialog
    }
}
