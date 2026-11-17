import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 聊天页
// 职责：组合 MessageBubble 呈现消息、底部输入区发送/取消，右侧抽屉切换历史会话。
// 键盘避让由 PageScaffold 统一处理（内容区底部让出键盘高度），页面内不再自己算 insets。
PageScaffold {
    id: root

    // 历史会话抽屉里"当前要操作哪一条"：重命名/删除都先记下目标再弹窗
    property string renameSessionId: ""
    property string deleteSessionId: ""
    property string deleteSessionTitle: ""
    // 错误卡片里的"技术细节"是否展开
    property bool errorDetailExpanded: false

    title: ChatViewModel.currentCharName.length > 0 ? ChatViewModel.currentCharName : "未选择角色"
    subtitle: {// 次级标题
        const parts = []
        if (ChatViewModel.currentSessionTitle.length > 0)
            parts.push(ChatViewModel.currentSessionTitle)
        if (CompletionPresetViewModel.activePreset.name.length > 0)
            parts.push(CompletionPresetViewModel.activePreset.name)
        return parts.join(" · ")
    }
    contentPadding: 0
    onLeftClicked: Ui.requestDrawer()

    // 会话里只有一条消息时它就是开场白：定位到顶部，让开场白从头显示；
    // 否则（已经在聊了）定位到最新消息。开场白可能很长，一条就撑满视口，
    // 这种情况下"定位到末尾"会把人直接甩到开场白底部。
    function positionForSessionStart() {
        Qt.callLater(function() {
            if (messageListView.count === 0)
                return
            if (messageListView.count <= 1)
                messageListView.positionViewAtBeginning()
            else
                messageListView.positionViewAtEnd()
        })
    }

    // 收到新的错误时收起细节，避免看到上一次的展开状态
    Connections {
        target: ChatViewModel
        function onLastGenerationErrorChanged() {
            root.errorDetailExpanded = false
        }
    }

    // PageHost 生命周期：切走时关闭抽屉，切回时按上面规则定位。
    function pageShown() {
        positionForSessionStart()
    }

    function pageHidden() {
        if (historyDrawer.opened)
            historyDrawer.close()
    }

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

        // ========== 开场白切换条 ==========
        // 放在 ListView 外面：不随消息滚动（长开场白看到一半也能切），
        // 也不占用气泡的排版空间。
        // 只在"会话里只剩开场白这一条"时出现——发过消息就定下来了。
        Item {
            id: greetingBar
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.controlMd
            visible: ChatViewModel.greetingMessageId.length > 0
                     && ChatViewModel.greetingCount > 1
                     && ChatViewModel.messageCount === 1

            RowLayout {
                anchors.centerIn: parent
                spacing: Theme.spaceXs

                // 朝上的细箭头旋转 ±90° 当作左右箭头，不为两个方向再引一套资产
                IconButton {
                    compact: true
                    tip: "上一条开场白"
                    icon.source: "qrc:/icons/res/arrow-narrow-up.svg"
                    rotation: -90
                    onClicked: {
                        ChatViewModel.switchGreeting(-1)
                        messageListView.positionViewAtBeginning()
                    }
                }

                Label {
                    text: (ChatViewModel.greetingIndex + 1) + "/"
                          + ChatViewModel.greetingCount
                    font.pixelSize: Theme.fontCaption
                    font.family: Theme.fontFamily
                    color: Theme.textSecondary
                }

                IconButton {
                    compact: true
                    tip: "下一条开场白"
                    icon.source: "qrc:/icons/res/arrow-narrow-up.svg"
                    rotation: 90
                    onClicked: {
                        ChatViewModel.switchGreeting(1)
                        messageListView.positionViewAtBeginning()
                    }
                }
            }
        }

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

                // 消息数变化时跟随：只有开场白一条就停在顶部，否则沿用"在底部才跟随"
                onCountChanged: {
                    if (messageListView.count <= 1) {
                        root.positionForSessionStart()
                    } else if (atYEnd) {
                        Qt.callLater(() => messageListView.positionViewAtEnd())
                    }
                }

                // 上拉到顶时加载更多历史
                onAtYBeginningChanged: {
                    if (atYBeginning && !ChatViewModel.isGenerating
                            && ChatViewModel.hasMoreHistory)
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
                                  ? SettingsViewModel.userAvatar
                                  : ChatViewModel.currentCharAvatarPath
                }
            }

            EmptyState {
                anchors.centerIn: parent
                // 用 ViewModel 的行数：QAbstractListModel 的 rowCount() 不能从 QML 直接调用
                visible: ChatViewModel.messageCount === 0
                text: "还没有消息"
                detail: ChatViewModel.currentCharName.length > 0
                        ? "和 " + ChatViewModel.currentCharName + " 打个招呼吧"
                        : "先在「角色管理」里选择一个角色"
            }
        }

        // ========== 请求失败提示 ==========
        // 以前网络错误只弹 3 秒 toast：长一点的错误看不完、错过就没了，
        // 也分不清是网络、Key 还是模型名的问题。这里把「结论 + 建议」留在界面上，
        // 技术细节按需展开、可复制。新的请求开始或切换会话时自动清空。
        Rectangle {
            id: errorCard
            Layout.fillWidth: true
            implicitHeight: errorContent.implicitHeight + Theme.spaceSm * 2
            Layout.preferredHeight: visible ? implicitHeight : 0
            visible: ChatViewModel.lastGenerationError.length > 0
            color: Theme.dangerSubtle

            ColumnLayout {
                id: errorContent
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: Theme.spaceSm
                spacing: Theme.spaceXs

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceSm

                    Label {
                        text: "请求失败"
                        font.pixelSize: Theme.fontBodySm
                        font.bold: true
                        font.family: Theme.fontFamily
                        color: Theme.danger
                    }

                    Item { Layout.fillWidth: true }

                    Label {
                        visible: ChatViewModel.lastGenerationErrorDetail.length > 0
                        text: root.errorDetailExpanded ? "收起详情" : "查看详情"
                        font.pixelSize: Theme.fontCaption
                        font.family: Theme.fontFamily
                        color: Theme.textSecondary
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.errorDetailExpanded = !root.errorDetailExpanded
                        }
                    }

                    Label {
                        text: "复制"
                        font.pixelSize: Theme.fontCaption
                        font.family: Theme.fontFamily
                        color: Theme.textSecondary
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: ChatViewModel.copyGenerationError()
                        }
                    }

                    Label {
                        text: "关闭"
                        font.pixelSize: Theme.fontCaption
                        font.family: Theme.fontFamily
                        color: Theme.textSecondary
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: ChatViewModel.dismissGenerationError()
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: ChatViewModel.lastGenerationError
                    wrapMode: Text.WordWrap
                    font.pixelSize: Theme.fontBodySm
                    font.family: Theme.fontFamily
                    color: Theme.textPrimary
                }

                ScrollView {
                    id: errorDetailScroll
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    visible: root.errorDetailExpanded
                             && ChatViewModel.lastGenerationErrorDetail.length > 0
                    contentWidth: availableWidth
                    clip: true
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded

                    TextArea {
                        id: errorDetailArea
                        // 与 LongTextField 相同的写法：显式宽度才能正确换行
                        width: Math.max(0, errorDetailScroll.availableWidth - Theme.spaceMd)
                        text: ChatViewModel.lastGenerationErrorDetail
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextArea.WrapAtWordBoundaryOrAnywhere
                        textFormat: TextArea.PlainText
                        font.pixelSize: Theme.fontCaption
                        font.family: Theme.fontFamily
                        color: Theme.textSecondary
                        topPadding: Theme.spaceXs
                        bottomPadding: Theme.spaceXs
                        leftPadding: Theme.spaceSm
                        rightPadding: Theme.spaceSm
                        background: Rectangle {
                            radius: Theme.radiusSm
                            color: Theme.bgSurface
                            border.width: 1
                            border.color: Theme.borderDefault
                        }
                    }
                }
            }
        }

        // ========== 输入区 ==========
        // 键盘避让交给系统：adjustResize 会让窗口自身收缩，输入框自然贴在键盘上沿。
        // 这里不再手动加键盘高度，否则会"窗口已变矮 + 再抬一次"，输入框被顶到顶部。
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
                        TextContextMenu {}
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
                            const send = ChatViewModel.sendWithEnter
                                         ? !(event.modifiers & Qt.ShiftModifier)
                                         : (event.modifiers & Qt.ControlModifier)
                            event.accepted = send
                            if (send)
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
                    // 上下内边距归零：48 的内容区才装得下 40 的图标按钮
                    topPadding: 0
                    bottomPadding: 0
                    highlighted: ChatViewModel.currentSessionId === model.id

                    onClicked: {
                        ChatViewModel.switchSession(model.id)
                        historyDrawer.close()
                    }

                    contentItem: RowLayout {
                        spacing: Theme.spaceXs

                        Label {
                            Layout.fillWidth: true
                            text: model.title
                            elide: Text.ElideRight
                            font.pixelSize: Theme.fontBody
                            font.family: Theme.fontFamily
                            color: Theme.textPrimary
                        }

                        IconButton {
                            compact: true
                            tip: "重命名会话"
                            icon.source: "qrc:/icons/res/edit.svg"
                            onClicked: {
                                root.renameSessionId = model.id
                                renameSessionDialog.text = model.title || ""
                                renameSessionDialog.open()
                            }
                        }

                        IconButton {
                            compact: true
                            danger: true
                            tip: "删除会话"
                            icon.source: "qrc:/icons/res/trash.svg"
                            onClicked: {
                                root.deleteSessionId = model.id
                                root.deleteSessionTitle = model.title || ""
                                deleteSessionDialog.open()
                            }
                        }
                    }
                }
            }

            AppButton {
                Layout.fillWidth: true
                text: "删除所有历史会话"
                variant: "danger"
                onClicked: deleteAllDialog.open()
            }
        }
    }

    ConfirmDialog {
        id: deleteAllDialog
        title: "确认删除"
        text: "确定删除当前角色的所有历史会话吗？此操作不可撤销。"
        acceptText: "删除"
        danger: true
        onConfirmed: ChatViewModel.deleteAllSession()
    }

    TextInputDialog {
        id: renameSessionDialog
        title: "重命名会话"
        label: "会话标题"
        acceptText: "保存"
        onSubmitted: (value) => ChatViewModel.renameSession(root.renameSessionId, value)
    }

    ConfirmDialog {
        id: deleteSessionDialog
        title: "删除会话"
        text: "确定删除「" + root.deleteSessionTitle + "」吗？"
        detail: "会话里的消息会一并删除，且不可撤销。"
        acceptText: "删除"
        danger: true
        onConfirmed: ChatViewModel.deleteSession(root.deleteSessionId)
    }
}
