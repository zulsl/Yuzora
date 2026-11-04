import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8

Rectangle {
    id: detailPanel
    required property string entryContent
    width: parent.width
    color: "#fafafa"
    clip: true                     // 收起时裁掉内容
    visible: height > 0            // 减少无谓渲染

    implicitHeight: form.implicitHeight + 24


    ColumnLayout {
        id: form
        width: parent.width - 32   // Column 内不能用 anchors
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 10
        //主要关键字
        Label { text: "主关键词（命中即触发）" }
        TextArea {
            Layout.fillWidth: true
            // text: model.keywords
            placeholderText: "每行一个关键词"
            implicitHeight: 80
            onEditingFinished: {
                //
            }
        }
        //逻辑
        //可选过滤器
        Label { text: "二级关键词（需同时满足）" }
        TextArea {
            Layout.fillWidth: true
            // text: model.secondaryKeywords
            placeholderText: "每行一个二级关键词"
            implicitHeight: 60
        }
        //内容
        ScrollView {
            width: parent.width
            height: 300
            clip: true
            TextArea {
                width: parent.width
                textFormat: TextEdit.PlainText  // 纯文本模式,关闭富文本
                font.pixelSize: 18
                wrapMode: TextArea.Wrap // 自动换行
                placeholderText: "内容"
                text: entryContent
                onActiveFocusChanged: {
                    model.content=text
                }
            }
        }
    }
}
