import QtQuick 6.8
import QtQuick.Dialogs 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8
import QtQuick.Controls.Material 6.8
import Yuzora 1.0

/**
 * 角色详情表单 - 可复用
 * 包含：所有角色信息编辑字段 + 操作按钮
 */
ScrollView{
    id:charaterScrollView
    anchors.fill: parent
    contentWidth: availableWidth
    clip: true

    padding:Theme.page_padding

    rightPadding:30

    property characterEntity editingCharacter : CharacterViewModel.currentCharacter

    ColumnLayout {
        id: root
        width: parent.width
        spacing:Theme.layout_spacing

        // CharacterViewModel.hasSelection ?

        Text {
            text: "角色详情"
            font.pixelSize: Theme.baseFontSize+2
            font.weight: Font.Bold
            Layout.bottomMargin: 4
        }

        // 操作栏
        RowLayout {
            Layout.alignment: Qt.AlignRight
            Layout.topMargin: 10
            spacing: 8

            MyToolButton{
                ToolTip.text:"角色世界书"
                icon.source: "qrc:/icons/res/main_page/book-2.svg"
                onClicked:characterBookDialog.open()
            }
            MyToolButton {
                ToolTip.text: "删除"
                icon.source: "qrc:/icons/res/trash.svg"
                enabled: CharacterViewModel.hasSelection
                onClicked: characterDeleteDialog.open()
            }
            MyToolButton {
                ToolTip.text: "保存"
                icon.source: "qrc:/icons/res/save.svg"
                onClicked: CharacterViewModel.saveCurrentCharacter(editingCharacter)
            }
            MyToolButton {
                ToolTip.text: "导出"
                icon.source: "qrc:/icons/res/file-export.svg"
                onClicked: exportDialog.open()
            }
        }



        TextField {
            id: nameField
            Layout.fillWidth: true
            placeholderText: "角色名称 *"
            text: editingCharacter.name || ""
        }


        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 300
            TextArea {
                id: descField
                placeholderText: "角色描述(系统提示词)"
                text: editingCharacter.description || ""
                wrapMode: Text.WordWrap
                textFormat: Qt.PlainText
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 300
            clip: true
            TextArea {
                id: firstMsgField
                placeholderText: "初始开场白"
                text:editingCharacter.firstMessage || ""
                wrapMode: Text.WordWrap
                textFormat: Qt.PlainText
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 300
            TextArea {
                id: altGreetField
                placeholderText: "备选开场白（每行一个）"
                text: (editingCharacter.alternateGreetings || []).join("\n")
                wrapMode: Text.WordWrap
                textFormat: Qt.PlainText
            }
        }

        //作者
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            TextField {
                id: creatorField
                Layout.fillWidth: true
                placeholderText: "作者"
                text: editingCharacter.creator || ""
            }
            TextField {
                id: versionField
                Layout.preferredWidth: 120
                placeholderText: "版本号"
                text: editingCharacter.characterVersion || "1.0.0"
            }
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            TextArea {
                id: creatorNotesField
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                placeholderText: "作者备注"
                text: editingCharacter.creatorNotes || ""
                wrapMode: Text.WordWrap
            }
        }


        // 触发按钮
        Button {
            id: dropdownBtn
            Layout.fillWidth: true
            flat: true
            text: extraForm.expanded ? "收起 ▲" : "更多高级定义 ▼"
            onClicked: extraForm.expanded = !extraForm.expanded
        }
        // 下拉表单
        ColumnLayout {
            id: extraForm
            Layout.fillWidth: true
            Layout.preferredHeight: expanded ? implicitHeight : 0  // 0 → 内容高
            Layout.bottomMargin: expanded ? 0 : -8                 // 收起时去掉多余间距
            spacing: 8
            clip: true                                             // 动画中途防止溢出
            opacity: expanded ? 1 : 0

            property bool expanded: false

            // 平滑动画
            Behavior on Layout.preferredHeight {
                NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
            }
            Behavior on opacity {
                NumberAnimation { duration: 180 }
            }

            TextArea {
                id: personalityField
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                placeholderText: "性格特征"
                text: editingCharacter.personality || ""
                wrapMode: Text.WordWrap
            }
            TextArea {
                id: scenarioField
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                placeholderText: "对话场景设定"
                text: editingCharacter.scenario || ""
                wrapMode: Text.WordWrap
            }
            TextArea {
                id: exampleField
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                placeholderText: "示例对话"
                text: editingCharacter.exampleDialogue || ""
                wrapMode: Text.WordWrap
            }
            TextArea {
                id: systemPromptField
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                placeholderText: "角色专属系统提示词"
                text: editingCharacter.systemPrompt || ""
                wrapMode: Text.WordWrap
            }
            TextField {
                id: tagsField
                Layout.fillWidth: true
                placeholderText: "标签（英文逗号分隔）"
                text: (editingCharacter.tags || []).join(", ")
            }
        }

    }


    MessageDialog {
        id: characterDeleteDialog
        title: "确认删除"
        text: "确定要删除该角色吗？"
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        onAccepted: CharacterViewModel.deleteCurrentCharacter()
    }

    Dialog {
        id: characterBookDialog
        title: "角色世界书"
        modal: true
        anchors.centerIn: Overlay.overlay   // 全局窗口居中

        //按钮居中
        footer: DialogButtonBox {
            standardButtons: Dialog.Ok | Dialog.Cancel
            alignment: Qt.AlignHCenter  //居中
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 16

            Label {
                text: "角色链接的世界书"
                font.pixelSize: 18
                font.bold: true
                Layout.fillWidth: true
            }

            Label {
                text: "使用该角色聊天时会自动启用该世界书"
                wrapMode: Label.WordWrap
                Layout.fillWidth: true
            }

            Item {
                Layout.fillWidth: true
                implicitHeight: 48

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: palette.base
                    border.color: palette.mid
                    border.width: 1

                    Label {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 12
                        text: CharacterViewModel.characterBookName
                        font.pixelSize: 15
                        color: palette.text
                    }
                }
            }

            // 底部占位撑开一点，防止内容贴按钮
            Item {
                Layout.fillHeight: true
            }
        }
    }


}
