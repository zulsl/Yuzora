import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8
import QtQuick.Dialogs 6.8
import QtQuick.Controls.Material 6.8
import Yuzora 1.0

//
ListView {
    id: bookEntryListView
    Layout.fillWidth: true
    // Layout.fillHeight: true
    Layout.preferredHeight: contentHeight
    interactive: false

    clip: true
    cacheBuffer: 400
    spacing: 2
    model: WorldInfoViewModel.entryModel
    currentIndex: -1

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

                //主要关键字
                TextField {
                    width: parent.width
                    text: entry?.name.join(",") ?? ""
                    onTextEdited: if (entry) entry.name = text
                }

                //逻辑(未完成)
                ComboBox{
                    model: ["与任意", "与所有","非所有","非任何"]
                }

                //次要关键字(未完成)
                TextField{}

                //扫描深度
                SpinBox{
                    from:0
                    to:1000
                    stepSize:1
                }

                //内容
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
                        textFormat: TextEdit.PlainText
                        font.pixelSize: 18
                        wrapMode: TextArea.Wrap
                        placeholderText: "内容"
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
        width: bookEntryListView.width
        height: mainColumn.implicitHeight   //高度随内容变化

        readonly property var entry: model

        Column {
            id: mainColumn
            width: parent.width

            // ---------- 标题行 ----------
            RowLayout{
                width: parent.width
                height: 56

                //下拉按钮
                RoundButton{
                    id: rotateBtn
                    width: 64
                    height: 64
                    flat: true
                    icon.source: model.expandedState
                        ?"qrc:/icons/res/circle-chevron-up.svg"
                        :"qrc:/icons/res/circle-chevron-down.svg"
                    onClicked: {
                        model.expandedState = !model.expandedState
                    }
                }

                //开关
                Switch {
                    checked: model.isEnabled
                    onToggled: model.isEnabled = checked
                }

                //名称
                TextField {
                    Layout.fillWidth: true
                    text: model.name
                    onTextEdited:model.name=text
                }

                //触发策略
                ComboBox{
                    model: ["始终", "关键词","向量化"]
                    currentIndex: entry.triggerControl
                    onCurrentIndexChanged: {
                        if (entry.triggerControl===currentIndex) return
                        entry.triggerControl = currentIndex
                    }
                }

                //插入位置
                ComboBox{
                    id:insertPositionComboBox
                    implicitContentWidthPolicy: ComboBox.WidestTextWhenCompleted
                    model: [
                        "角色定义之前",
                        "角色定义之后",
                        "示例消息之前",
                        "示例消息之后",
                        "作者注释之前",
                        "作者注释之后",
                        "@D[系统]在深度",
                        "@D[用户]在深度",
                        "@D[AI]在深度"
                    ]
                    currentIndex: entry.insertPosition
                    onCurrentIndexChanged: {
                        if (entry.insertPosition===currentIndex) return
                        entry.insertPosition=currentIndex
                    }
                }


                //深度
                SpinBox{
                    visible: entry.insertPosition === 4
                    from:0
                    to:1000000
                    stepSize:1
                    value: entry.insertDepth
                    onValueModified:entry.insertDepth=value
                }

                //顺序
                SpinBox{
                    from:0
                    to:1000000
                    stepSize:1
                    value: entry.sortOrder
                    onValueModified:entry.sortOrder=value
                }

                // //触发概率
                // SpinBox{}

                //删除
                MyToolButton {
                    ToolTip.text:"删除"
                    icon.source: "qrc:/icons/res/trash.svg"
                }
            }


            Loader {
                id: detailLoader
                width: parent.width
                sourceComponent: detailComponent
                active: model !== null && model !== undefined && model.expandedState
                asynchronous: false

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
