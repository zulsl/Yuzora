import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 聊天消息行
// 职责：一条消息的完整呈现（头像 + 发送者 + 气泡 + 时间/已编辑标记）。
// 从 ChatPage 的内联 delegate 抽出，使气泡配色、圆角、最大宽度在全项目唯一。
// 可配置属性：isUser、senderName、content、timeText、isEdited、avatarSource、status。
//
// status 用来表达"生成中 / 失败 / 已取消"：
//   - 生成中且还没有文本 -> 气泡里循环三点（延迟 250ms 出现，避免快速回复闪一下）；
//   - 空内容结束 -> 气泡里显示"生成失败 / 已取消 /（空回复）"（纯显示，不写库）；
//   - 有正文但被中断 -> 时间行补"生成已中断 / 已取消"。
Item {
    id: root

    property bool isUser: false
    property string senderName: ""
    property string content: ""
    property string timeText: ""
    property bool isEdited: false
    property url avatarSource
    // GlobalConstants::MessageGenerationStatus，默认按"已完成"处理
    property int status: GlobalConstants.Normal
    // 是否是会话最后一条（只有最后一条助手回复可以重新生成）
    property bool isLastMessage: false
    // 是否是当前开场白消息（开场白可以删除，但不能重新生成）
    property bool isGreeting: false
    // 当前会话是否正在生成：生成中消息操作统一置灰（只保留文本操作）
    property bool generationActive: false

    signal deleteRequested()
    signal regenerateRequested()

    readonly property bool isGenerating: status === GlobalConstants.Generating
    readonly property bool hasText: content.trim().length > 0
    // 等待态：助手消息、正在生成、但还没有任何非空白文本
    readonly property bool waiting: !isUser && isGenerating && !hasText

    // "是否有资格"和"当前能不能点"分开：生成中菜单仍然打开，但动作置灰
    readonly property bool canRegenerate: !isUser && isLastMessage && !isGreeting
    readonly property bool canDelete: true

    // 250ms 之后才真正画点：100ms 内就开始流式回复的情况下不闪
    property bool waitingVisible: false

    Timer {
        id: waitingDelay
        interval: 250
        repeat: false
        onTriggered: root.waitingVisible = true
    }

    // 不用 running: waiting 绑定：Timer 触发后会把 running 置回 false，
    // 下一次进入等待态时绑定可能已经失效。这里显式 restart 更可靠。
    onWaitingChanged: {
        if (waiting) {
            waitingVisible = false
            waitingDelay.restart()
        } else {
            waitingDelay.stop()
            waitingVisible = false
        }
    }

    Component.onCompleted: {
        if (waiting)
            waitingDelay.restart()
    }

    // 空内容结束时的占位文案
    readonly property string statusText: {
        if (isUser || hasText || isGenerating)
            return ""
        if (status === GlobalConstants.Failed)
            return qsTr("生成失败")
        if (status === GlobalConstants.Canceled)
            return qsTr("已取消")
        if (status === GlobalConstants.Normal)
            return qsTr("（空回复）")
        return ""
    }

    // 有正文但被中断时的标记，放在时间那一行
    readonly property string interruptedNote: {
        if (isUser || !hasText)
            return ""
        if (status === GlobalConstants.Failed)
            return qsTr("生成已中断")
        if (status === GlobalConstants.Canceled)
            return qsTr("已取消")
        return ""
    }

    // 等待态气泡按"一行字"的高度画；用 TextMetrics 取行高，
    // 这样从三个点切到第一行字时不会有一像素跳动。
    TextMetrics {
        id: lineMetrics
        font.pixelSize: Theme.fontBody
        font.family: Theme.fontFamily
        text: "M"
    }

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

            Label {// 发生者名称
                Layout.fillWidth: true
                text: root.isUser ? SettingsViewModel.userName : root.senderName
                visible: root.senderName.length > 0
                font.pixelSize: Theme.fontCaption
                font.family: Theme.fontFamily
                color: Theme.textSecondary
                elide: Text.ElideRight
                horizontalAlignment: root.isUser ? Text.AlignRight : Text.AlignLeft
            }

            Rectangle {// 气泡
                id: bubble
                Layout.alignment: root.isUser ? Qt.AlignRight : Qt.AlignLeft
                Layout.maximumWidth: row.width - Theme.avatarMd - Theme.spaceSm * 2
                implicitWidth: {
                    if (root.waiting)
                        return typingDots.implicitWidth + Theme.spaceMd * 2
                    if (root.statusText.length > 0)
                        return statusLabel.implicitWidth + Theme.spaceMd * 2
                    return Math.min(text.implicitWidth + Theme.spaceMd * 2, Layout.maximumWidth)
                }
                implicitHeight: {
                    const contentHeight = (root.waiting || root.statusText.length > 0)
                        ? lineMetrics.height
                        : Math.max(text.implicitHeight, lineMetrics.height)
                    return contentHeight + Theme.spaceSm * 2
                }
                radius: Theme.radiusMd
                color: root.isUser ? Theme.bubbleUser : Theme.bubbleAssistant
                border.width: root.isUser ? 0 : 1
                border.color: Theme.bubbleAssistantBorder

                TextEdit {
                    id: text
                    anchors.fill: parent
                    anchors.margins: Theme.spaceSm
                    text: root.content
                    // 等待态 / 结束占位时不排版正文：既能省一次全文排版，
                    // 也避免空气泡里露出一个看不见的光标区域。
                    visible: !root.waiting && root.statusText.length === 0
                    opacity: root.waiting ? 0 : 1
                    readOnly: true
                    selectByMouse: true
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    textFormat: TextEdit.PlainText
                    font.pixelSize: Theme.fontBody
                    font.family: Theme.fontFamily
                    color: root.isUser ? Theme.bubbleUserText : Theme.textPrimary

                    // 首字到达时淡入，和下面的三点淡出节奏一致
                    Behavior on opacity {
                        NumberAnimation { duration: 120; easing.type: Theme.easingOut }
                    }
                }

                TypingDots {
                    id: typingDots
                    anchors.centerIn: parent
                    visible: root.waiting && root.waitingVisible
                    running: root.waiting && root.waitingVisible
                    opacity: visible ? 1 : 0
                    color: Theme.textSecondary

                    Behavior on opacity {
                        NumberAnimation { duration: 120; easing.type: Theme.easingOut }
                    }
                }

                Label {
                    id: statusLabel
                    anchors.centerIn: parent
                    visible: root.statusText.length > 0
                    text: root.statusText
                    font.pixelSize: Theme.fontBody
                    font.family: Theme.fontFamily
                    color: Theme.textDisabled
                }

                // 气泡级菜单：正文/空状态都能右键；桌面右键、触屏长按
                MessageContextMenu {
                    textTarget: text
                    hasText: root.hasText
                    canRegenerate: root.canRegenerate
                    canDelete: root.canDelete
                    actionsEnabled: !root.generationActive
                    onRegenerateRequested: root.regenerateRequested()
                    onDeleteRequested: root.deleteRequested()
                }
            }

            RowLayout {// 时间
                Layout.alignment: root.isUser ? Qt.AlignRight : Qt.AlignLeft
                spacing: Theme.spaceSm

                Label {
                    text: root.interruptedNote
                    visible: root.interruptedNote.length > 0
                    font.pixelSize: Theme.fontCaption
                    font.family: Theme.fontFamily
                    color: Theme.textDisabled
                }

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
