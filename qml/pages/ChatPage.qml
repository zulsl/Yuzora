import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8
import QtQuick.Controls.Material 6.8
import Qt5Compat.GraphicalEffects
import Yuzora 1.0

// import com.qtsillytavern 1.0

//聊天主界面
//绑定 ChatViewModel，支持流式输出、消息列表增量渲染、发送 / 取消生成，完全适配 ListModel 高性能渲染。

Page {
    id: root
    padding: 0//清除边距
    header: ToolBar {
        height: 50
        RowLayout {
            anchors.fill: parent
            // 左侧按钮
            ToolButton {
                id:leftBtn
                icon.source: "qrc:/icons/res/menu.svg"
                onClicked: sideDrawer.open()
            }
            Label {
                text: ChatViewModel.currentCharName || "未选择角色"
                font.pixelSize: 16
                font.bold: true
                Layout.fillWidth: true
            }
            Label {
                text: "标题: " + ChatViewModel.currentSessionTitle
                color: palette.mid
            }
            Label {
                text: "预设: " + ChatViewModel.currentPresetName || "默认"
                color: palette.mid
            }
            ToolButton {
                ToolTip.text: "新对话"
                icon.source:"qrc:/icons/res/message-circle-plus.svg"
                onClicked: ChatViewModel.createNewSession()
            }
            ToolButton {
                id:historyBtn
                icon.source:"qrc:/icons/res/history.svg"
                onClicked: historyDrawer.open()
            }
        }
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 消息列表（使用QAbstractListModel增量渲染）
        ListView {
            id: messageListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10
            topMargin: 12
            bottomMargin: 12

            model: ChatViewModel ? ChatViewModel.messageModel : null

            ScrollBar.vertical: ScrollBar {policy: ScrollBar.AsNeeded} //滚动条策略
            clip: true
            interactive: count > 0
            reuseItems: true // 开启delegate复用，长列表性能提升

            // onModelChanged: {
            //     if (model && model.count > 0) {
            //         // 延迟两帧，等 delegate 创建、数据绑定、尺寸计算全部完成
            //         Qt.callLater(() => {
            //             Qt.callLater(() => {
            //                 forceLayout()
            //                 positionViewAtEnd()
            //             })
            //         })
            //     }
            // }
            // Component.onCompleted: {
            //     forceLayout()//重绘气泡,解决一些bug
            // }

            // 新消息自动滚动到底部
            onCountChanged: {
                // 用户已经处于列表最末端，才滚到底部；生成中也跟随滚动
                if(atYEnd) {
                    Qt.callLater(()=>{
                        positionViewAtEnd()
                    })
                }
            }

            // 上拉加载更多历史消息
            onAtYBeginningChanged: {
                if (atYBeginning && ChatViewModel && !ChatViewModel.isGenerating) {
                    ChatViewModel.loadMoreHistory()
                }
            }



            delegate: Item {
                id: delegateItem
                width: ListView.view.width
                height: Math.max(avatarSize + 8, messageColumn.implicitHeight + 8)

                readonly property int avatarSize: 40// 头像尺寸
                readonly property bool isUser:model.messageType === 0

                RowLayout{
                    anchors.fill: parent
                    // leftPadding:8
                    anchors.leftMargin: 8 // 替代原来无效的 leftPadding，左边留 8px 空隙
                    spacing: 8            // 头像和消息列之间的间距，和原来一致

                    // ----- 头像 -----
                    Item {
                        id: avatarImage
                        width: avatarSize
                        height: avatarSize  // 宽高必须相等，保证正圆
                        Layout.alignment: Qt.AlignTop // 顶部对齐，不再垂直居中

                        // 原始图片（仅作为遮罩输入，不直接显示）
                        Image {
                            id: avatarSource
                            anchors.fill: parent
                            source: isUser ? "qrc:/default/res/user-circle.png"
                                : ChatViewModel.currentCharAvatarPath
                            fillMode: Image.PreserveAspectCrop
                            smooth: true
                            mipmap: false          //显式关闭mipmap，小尺寸无需多级纹理
                            visible: false
                        }

                        // 圆形遮罩形状（仅提供裁剪轮廓，不直接显示）
                        Rectangle {
                            id: circleMask
                            anchors.fill: parent
                            radius: width / 2  // 圆角设为宽度一半，即为正圆
                            color: "white"
                            visible: false
                        }

                        // 应用遮罩，输出最终圆形头像
                        OpacityMask {
                            anchors.fill: parent
                            source: avatarSource
                            maskSource: circleMask
                            smooth: true  // 开启抗锯齿，边缘更平滑
                        }
                    }

                    // ----- 消息内容（包含名称、气泡、时间） -----
                    ColumnLayout {
                        id: messageColumn
                        Layout.fillWidth: true
                        spacing: 2

                        // 发送者名称
                        Label {
                            text: senderName
                            font.pixelSize: 12
                            width: parent.width
                            elide: Text.ElideRight
                        }

                        // ----- 消息气泡 -----
                        Rectangle {
                            id: bubble
                            Layout.maximumWidth: delegateItem.width - delegateItem.avatarSize - 24
                            implicitWidth: messageText.implicitWidth + 16
                            implicitHeight: messageText.implicitHeight + 16

                            // width:delegateItem.width-avatarSize-8
                            // width: Math.min(messageText.implicitWidth+16,delegateItem.width-avatarSize-24)
                            // width: Math.min(messageText.implicitWidth+16,root.width-avatarSize-24)
                            height: messageText.implicitHeight + 16
                            radius: 8
                            color: isUser ? "#95ec69" : "#ffffff"   // 自己绿色，对方白色
                            border.color: isUser ? "transparent" : "#e0e0e0"
                            border.width: isUser ? 0 : 1

                            // 消息文本
                            TextEdit {
                                id:messageText
                                anchors {
                                    fill: parent
                                    margins: 8
                                }
                                text: model.content
                                wrapMode: Text.Wrap//按字符自动换行
                                // font.pixelSize: 15
                                color: isUser ? "#000000" : "#000000"
                                readOnly: true      // 设置为只读，防止用户编辑
                                selectByMouse: true // 允许用鼠标选择文本
                                // selectionColor: "lightblue"
                            }
                        }
                        // 时间戳 + 编辑标记
                        RowLayout {
                            spacing: 8
                            Text {
                                text: model.timeText
                                color: "#909399"
                                font.pixelSize: 10
                            }
                            Text {
                                visible: model.isEdited
                                text: "已编辑"
                                color: isUser ? "rgba(255,255,255,0.7)" : "#909399"
                                font.pixelSize: 10
                            }
                        }
                    }

                    Item{Layout.fillWidth: true}
                }


            }
        }

        // 底部输入区域
        Pane {
            id: inputPane
            Layout.fillWidth: true
            padding: 12
            // background: Rectangle {
            //     color: palette.window
            // }
            RowLayout {
                anchors.fill: parent
                spacing: 10

                TextArea {
                    id: inputArea
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    placeholderText: "输入消息，回车发送，Shift+回车换行"
                    font.pixelSize: 14
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere//换行模式
                    selectByMouse: true
                    enabled: ChatViewModel && !ChatViewModel.isGenerating

                    Keys.onReturnPressed: function(event) {
                        event.accepted = !(event.modifiers & Qt.ShiftModifier)
                        if (event.accepted) sendBtn.clicked()
                    }
                }

                ToolButton {
                    id: sendBtn
                    icon.source: ChatViewModel && ChatViewModel.isGenerating
                        ?"qrc:/icons/res/player-stop.svg":"qrc:/icons/res/arrow-narrow-up.svg"
                    enabled: inputArea.text.length > 0 || ChatViewModel.isGenerating
                    onClicked: {
                        if (!ChatViewModel) return
                        if (ChatViewModel.isGenerating) {
                            ChatViewModel.cancelGeneration()
                        } else {
                            ChatViewModel.sendMessage(inputArea.text)
                            inputArea.clear()
                        }
                    }
                }
            }
        }
    }
    Drawer {
        id: historyDrawer
        width: 240        // 抽屉宽度
        height: parent.height
        edge: Qt.RightEdge  // 从右侧边缘滑出
        modal: true         // 模态：打开时背景半透明遮罩
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        ColumnLayout{
            anchors.fill: parent

            Button{
                text: "删除所有历史会话"
                onClicked: {
                    ChatViewModel.deleteAllSession()
                }
            }

            // 抽屉内部：列表视图
            ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 2
            model: ChatViewModel.sessionModel
            currentIndex: -1
            // currentIndex: ChatViewModel.currentIndex
                delegate: ItemDelegate {
                    width: ListView.view.width
                    height: 60
                    text: model.title
                    highlighted: ListView.isCurrentItem//高亮显示
                    onClicked: {
                        historyDrawer.close() // 点击选项后自动关闭抽屉

                        ListView.view.currentIndex = index//设置索引
                        ChatViewModel.switchSession(model.id)//切换会话

                    }
                }
            }
        }
    }
}