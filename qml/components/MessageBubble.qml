import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 聊天消息行
// 职责：一条消息的完整呈现（头像 + 发送者 + 气泡 + 时间/已编辑标记）。
// 从 ChatPage 的内联 delegate 抽出，使气泡配色、圆角、最大宽度在全项目唯一。
// 可配置属性：isUser、senderName、content、timeText、isEdited、avatarSource。
Item {
    id: root

    property bool isUser: false
    property string senderName: ""
    property string content: ""
    property string timeText: ""
    property bool isEdited: false
    property url avatarSource

    // 开场白切换：只有承载开场白的那条消息会打开它（greetingText 形如 "1/3"）
    property bool isGreeting: false
    property string greetingText: ""

    signal greetingPrev()
    signal greetingNext()

    implicitHeight: Math.max(Theme.avatarMd + Theme.spaceSm, column.implicitHeight + Theme.spaceSm)

    RowLayout {
        id: row
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: Theme.spaceSm
        anchors.rightMargin: Theme.spaceSm
        spacing: Theme.spaceSm

        // 用户消息头像靠右，与常见 IM 习惯一致
        CircleAvatar {
            visible: !root.isUser
            Layout.alignment: Qt.AlignTop
            size: Theme.avatarMd
            source: root.avatarSource
            fallbackText: root.senderName.length > 0 ? root.senderName.charAt(0) : ""
        }

        ColumnLayout {
            id: column
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: Theme.spaceXs

            Label {
                Layout.fillWidth: true
                text: root.senderName
                visible: root.senderName.length > 0
                font.pixelSize: Theme.fontCaption
                font.family: Theme.fontFamily
                color: Theme.textSecondary
                elide: Text.ElideRight
                horizontalAlignment: root.isUser ? Text.AlignRight : Text.AlignLeft
            }

            Rectangle {
                id: bubble
                Layout.alignment: root.isUser ? Qt.AlignRight : Qt.AlignLeft
                Layout.maximumWidth: row.width - Theme.avatarMd - Theme.spaceSm * 2
                implicitWidth: Math.min(text.implicitWidth + Theme.spaceMd * 2, Layout.maximumWidth)
                implicitHeight: text.implicitHeight + Theme.spaceSm * 2
                radius: Theme.radiusMd
                color: root.isUser ? Theme.bubbleUser : Theme.bubbleAssistant
                border.width: root.isUser ? 0 : 1
                border.color: Theme.bubbleAssistantBorder

                TextEdit {
                    id: text
                    anchors.fill: parent
                    anchors.margins: Theme.spaceSm
                    text: root.content
                    readOnly: true
                    selectByMouse: true
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    textFormat: TextEdit.PlainText
                    font.pixelSize: Theme.fontBody
                    font.family: Theme.fontFamily
                    color: root.isUser ? Theme.bubbleUserText : Theme.textPrimary
                    selectionColor: Theme.accentSubtle
                }
            }

            RowLayout {
                Layout.alignment: root.isUser ? Qt.AlignRight : Qt.AlignLeft
                spacing: Theme.spaceSm
                Label {
                    text: root.timeText
                    visible: root.timeText.length > 0
                    font.pixelSize: Theme.fontCaption
                    font.family: Theme.fontFamily
                    color: Theme.textDisabled
                }
                Label {
                    text: "已编辑"
                    visible: root.isEdited
                    font.pixelSize: Theme.fontCaption
                    font.family: Theme.fontFamily
                    color: Theme.textDisabled
                }
            }

            // ---------- 开场白切换（仅承载开场白的那条消息显示）----------
            // 用一个朝上的细箭头图标旋转 ±90°，避免为左右箭头再引一套资产。
            // 按钮本身是圆角方块，旋转 90° 外观不变，只有图标方向变。
            RowLayout {
                Layout.alignment: root.isUser ? Qt.AlignRight : Qt.AlignLeft
                spacing: Theme.spaceXs
                visible: root.isGreeting && root.greetingText.length > 0

                IconButton {
                    compact: true
                    tip: "上一条开场白"
                    icon.source: "qrc:/icons/res/arrow-narrow-up.svg"
                    rotation: -90
                    onClicked: root.greetingPrev()
                }

                Label {
                    text: root.greetingText
                    font.pixelSize: Theme.fontCaption
                    font.family: Theme.fontFamily
                    color: Theme.textSecondary
                }

                IconButton {
                    compact: true
                    tip: "下一条开场白"
                    icon.source: "qrc:/icons/res/arrow-narrow-up.svg"
                    rotation: 90
                    onClicked: root.greetingNext()
                }
            }
        }

        CircleAvatar {
            visible: root.isUser
            Layout.alignment: Qt.AlignTop
            size: Theme.avatarMd
            source: root.avatarSource
            fallbackText: root.senderName.length > 0 ? root.senderName.charAt(0) : ""
        }
    }
}
