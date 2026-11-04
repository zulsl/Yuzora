import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8
import QtQuick.Dialogs 6.8
import QtQuick.Controls.Material 6.8
import Yuzora 1.0

//---------提示词条目列表-----------
ListView {
    id: promptsListView
    Layout.fillWidth: true
    // Layout.fillHeight: true
    Layout.preferredHeight: contentHeight
    interactive: false

    clip: true
    cacheBuffer: 400
    spacing: 2
    model: CompletionPresetViewModel.presetPromptModel
    currentIndex: -1

    //(已弃用)
    // property string expandedId: ""// 当前展开的项，-1 表示全部收起//使用int导致莫名其妙的bug

    // ---------- 详情表单：Loader 懒加载 ----------
    Component {
        id: detailComponent
        Item {

            id: detailPanel
            width: parent.width
            // color: "#fafafa"
            clip: true                     // 收起时裁掉内容
            visible: height > 0            // 减少无谓渲染

            implicitHeight: form.implicitHeight + 24


            Column {
                id: form
                width: parent.width - 32
                spacing: 10

                //提示词名称(可编辑)
                Label{
                    font.pixelSize: 16
                    font.bold: true
                    text: "名称:"
                }
                TextField {
                    width: parent.width
                    text: entry?.name ?? ""
                    onTextEdited: if (entry) entry.name = text
                }

                //消息类型/角色
                RowLayout{
                    width: parent.width
                    Label{
                        font.pixelSize: 16
                        font.bold: true
                        text: "角色(此消息应归于谁):"
                    }
                    ComboBox{
                        Layout.fillWidth: true
                        model: ["系统", "用户", "AI助手"]
                        currentIndex: entry?.role ?? 0
                        onActivated: (index) => { if (entry) entry.role = index }
                    }
                }

                //位置
                RowLayout{
                    width: parent.width
                    Label{
                        font.pixelSize: 16
                        font.bold: true
                        text: "位置:"
                    }
                    ComboBox{
                        Layout.fillWidth: true
                        model: ["相对","聊天中"]
                        currentIndex: entry?.position ?? 0
                        onActivated: (index) => { if (entry) entry.position = index }
                    }
                }

                Timer {
                    id: debounceTimer
                    interval: 300
                    repeat: false
                    onTriggered: if (entry) entry.content = innerText.text
                }

                //正文内容
                ScrollView {
                    width: parent.width
                    height: 400
                    clip: true
                    TextArea {
                        id: innerText
                        // width: parent.width
                        // height: parent.width
                        textFormat: TextEdit.PlainText  // 纯文本模式,关闭富文本
                        font.pixelSize: 18
                        wrapMode: TextArea.Wrap // 自动换行
                        placeholderText: "提示词"
                        text: entry?.content ?? ""
                        onTextChanged: {
                            debounceTimer.restart()
                        }
                    }
                }

            }
        }
    }


    delegate: Item {
        id: delegateRoot
        width: promptsListView.width
        height: mainColumn.implicitHeight   //高度随内容变化

        Column {
            id: mainColumn
            width: parent.width

            // ---------- 标题行 ----------
            RowLayout{
                width: parent.width
                height: 56
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    RowLayout{
                        anchors.fill: parent
                        Text {
                            text: "▼"
                            rotation: model.expandedState ? 180 : 0
                            Behavior on rotation { NumberAnimation { duration: 200 } }//旋转动画
                        }
                        //名称
                        Text {
                            Layout.fillWidth: true
                            text: model.name
                            font.pixelSize: 16
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            //再次点击则收起
                            model.expandedState = !model.expandedState
                        }
                    }

                }

                Switch {
                    checked: model.isActive
                    onToggled: model.isActive = checked
                }
            }


            Loader {
                id: detailLoader
                width: parent.width
                sourceComponent: detailComponent
                active: model !== null && model !== undefined && model.expandedState // 收起时不创建，不绑定
                asynchronous: false                  // 异步加载,展开时可以打开异步避免卡一帧

                property var entry: model

                // 展开 = 内容高度，收起 = 0
                height: model.expandedState ? implicitHeight : 0
                Behavior on height {//动画
                    NumberAnimation { duration: 220; easing.type: Easing.InOutQuad }
                }
                clip: true
            }
        }
    }
}
