import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 聊天页
// 职责：组合 MessageBubble 呈现消息、底部输入区发送/取消，右侧抽屉切换历史会话。
// 键盘避让由 PageScaffold 统一处理（内容区底部让出键盘高度），页面内不再自己算 insets。
PageScaffold {
    id: root

    title: ChatViewModel.currentCharName.length > 0 ? ChatViewModel.currentCharName : "未选择角色"
    subtitle: {
        const parts = []
        if (ChatViewModel.currentSessionTitle.length > 0)
            parts.push(ChatViewModel.currentSessionTitle)
        if (ChatViewModel.currentPresetName.length > 0)
            parts.push(ChatViewModel.currentPresetName)
        return parts.join(" · ")
    }
    contentPadding: 0
    onLeftClicked: Ui.requestDrawer()

    rightItems: [
        IconButton {
            tip: "新对话"
            icon.source: "qrc:/icons/res/message-circle-plus.svg"
            onClicked: ChatViewModel.createNewSession()
        },
        IconButton {
            tip: "历史会话"
            icon.source: "qrc:/icons/res/history.svg"
            onClicked: historyDrawer.open()
        }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ========== 消息列表 ==========
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: messageListView
                anchors.fill: parent
                anchors.topMargin: Theme.spaceSm
                anchors.bottomMargin: Theme.spaceSm
                clip: true
                spacing: Theme.spaceSm
                reuseItems: true
                model: ChatViewModel.messageModel
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                // 只在用户本来就在底部时跟随滚动
                onCountChanged: {
                    if (atYEnd) {
                        Qt.callLater(() => messageListView.positionViewAtEnd())
                    }
                }

                // 上拉到顶时加载更多历史
                onAtYBeginningChanged: {
                    if (atYBeginning && !ChatViewModel.isGenerating)
                        ChatViewModel.loadMoreHistory()
                }

                delegate: MessageBubble {
                    width: ListView.view.width
                    // 修正：原实现用 messageType === 0 判断"是我"，而 0 其实是 System，
                    // 导致用户消息与 AI 消息的气泡样式整体对调。
                    isUser: model.messageType === GlobalConstants.User
                    senderName: model.senderName
                    content: model.content
                    timeText: SettingsViewModel.showTimeStamp ? model.timeText : ""
                    isEdited: model.isEdited
                    avatarSource: model.messageType === GlobalConstants.User
                                  ? "qrc:/default/res/user-circle.png"
                                  : ChatViewModel.currentCharAvatarPath
                }
            }

            EmptyState {
                anchors.centerIn: parent
                // 用 ListView.count 判断：QAbstractListModel 的 rowCount() 不可从 QML 直接调用
                visible: messageListView.count === 0
                text: "还没有消息"
                detail: ChatViewModel.currentCharName.length > 0
                        ? "和 " + ChatViewModel.currentCharName + " 打个招呼吧"
                        : "先在「角色管理」里选择一个角色"
            }
        }

        // ========== 输入区 ==========
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: inputRow.implicitHeight + Theme.spaceMd * 2
            color: Theme.bgSurface

            Rectangle {
                anchors { left: parent.left; right: parent.right; top: parent.top }
                height: 1
                color: Theme.borderDefault
            }

            RowLayout {
                id: inputRow
                anchors.fill: parent
                anchors.margins: Theme.spaceMd
                spacing: Theme.spaceSm

                ScrollView {
                    Layout.fillWidth: true
                    Layout.maximumHeight: Theme.controlLg * 2.4
                    clip: true

                    TextArea {
                        id: inputArea
                        placeholderText: "输入消息，回车发送，Shift+回车换行"
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        selectByMouse: true
                        font.pixelSize: Theme.fontBody
                        font.family: Theme.fontFamily
                        color: Theme.textPrimary
                        enabled: !ChatViewModel.isGenerating

                        // 回车发送、Shift+回车换行（沿用改造前的行为；
                        // SettingsViewModel.sendWithEnter 目前仍是未接线状态，见 P5 说明）
                        Keys.onReturnPressed: (event) => {
                            event.accepted = !(event.modifiers & Qt.ShiftModifier)
                            if (event.accepted)
                                sendButton.clicked()
                        }
                    }
                }

                IconButton {
                    id: sendButton
                    tip: ChatViewModel.isGenerating ? "停止生成" : "发送"
                    enabled: inputArea.text.length > 0 || ChatViewModel.isGenerating
                    icon.source: ChatViewModel.isGenerating
                                 ? "qrc:/icons/res/player-stop.svg"
                                 : "qrc:/icons/res/arrow-narrow-up.svg"
                    onClicked: {
                        if (ChatViewModel.isGenerating) {
                            ChatViewModel.cancelGeneration()
                        } else {
                            const text = inputArea.text.trim()
                            if (text.length === 0)
                                return
                            ChatViewModel.sendMessage(text)
                            inputArea.clear()
                        }
                    }
                }
            }
        }
    }

    // ========== 历史会话抽屉 ==========
    Drawer {
        id: historyDrawer
        width: Math.min(320, Math.round(root.width * 0.84))
        height: root.height
        edge: Qt.RightEdge
        modal: true
        interactive: !Theme.isTouchPrimary
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: Theme.spaceMd

        background: Rectangle { color: Theme.bgSurface }

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.spaceSm

            Label {
                text: "历史会话"
                font.pixelSize: Theme.fontBody
                font.bold: true
                font.family: Theme.fontFamily
                color: Theme.textPrimary
                Layout.fillWidth: true
            }

            ListView {
                id: sessionListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: Theme.spaceXs
                model: ChatViewModel.sessionModel

                delegate: ItemDelegate {
                    width: ListView.view.width
                    height: Theme.touchTarget
                    text: model.title
                    font.pixelSize: Theme.fontBody
                    font.family: Theme.fontFamily
                    highlighted: ListView.isCurrentItem

                    onClicked: {
                        sessionListView.currentIndex = index
                        ChatViewModel.switchSession(model.id)
                        historyDrawer.close()
                    }
                }
            }

            AppButton {
                Layout.fillWidth: true
                text: "删除所有历史会话"
                variant: "danger"
                onClicked: ChatViewModel.deleteAllSession()
            }
        }
    }

    // 聊天相关错误统一走全局吐司
    Connections {
        target: ChatViewModel
        function onOperationError(message) { Ui.toast(message, true) }
        function onOperationTip(message) { Ui.toast(message, false) }
        function onGenerationError(message) { Ui.toast(message, true) }
    }
}
