// import QtQuick.Window 2.15
// import QtQuick.Controls 2.15
// import QtQuick.Layouts 2.15

// Window {
//     width: 800
//     height: 600
//     title: "网络模块测试页 - MVVM 测试"
//     visible: true

//     ColumnLayout {
//         anchors.fill: parent
//         anchors.margins: 16
//         spacing: 12

//         // 1. 模型类型选择
//         RowLayout {
//             Layout.fillWidth: true
//             spacing: 8

//             Label { text: "模型类型：" }
//             ComboBox {
//                 id: modelTypeCombo
//                 Layout.fillWidth: true
//                 model: ListModel {
//                     ListElement { name: "DeepSeek"; type: 1 }
//                     ListElement { name: "OpenAI"; type: 0 }
//                 }
//                 textRole: "name"
//                 valueRole: "type"

//                 onCurrentValueChanged: {
//                     Net_test_VM.currentModelType = currentValue;
//                 }
//             }
//             Button {
//                 text: "连接"
//                 onClicked: Net_test_VM.loadModelList()
//                 enabled: !Net_test_VM.isLoading
//             }
//         }

//         // 2. 远端模型下拉选择
//         RowLayout {
//             Layout.fillWidth: true
//             spacing: 8

//             Label { text: "可用模型：" }
//             ComboBox {
//                 id: modelNameCombo
//                 Layout.fillWidth: true
//                 model: Net_test_VM.modelList
//             }
//         }

//         // 3. 输入框
//         TextField {
//             Layout.fillWidth: true
//             height: 80
//             placeholderText: "请输入对话内容..."
//             id: inputEdit
//             wrapMode: Text.Wrap
//         }

//         // 4. 操作按钮
//         RowLayout {
//             Layout.fillWidth: true
//             spacing: 12

//             Button {
//                 text: "发送请求"
//                 Layout.fillWidth: true
//                 enabled: !Net_test_VM.isLoading
//                 onClicked: {
//                     Net_test_VM.sendChatRequest(inputEdit.text, modelNameCombo.currentText)
//                 }
//             }
//             Button {
//                 text: "取消请求"
//                 Layout.fillWidth: true
//                 onClicked: Net_test_VM.cancelAllRequest()
//             }
//         }

//         // 5. 日志输出
//         GroupBox {
//             Layout.fillWidth: true
//             Layout.fillHeight: true
//             title: "运行日志"

//             ScrollView {
//                 anchors.fill: parent
//                 TextEdit {
//                     readOnly: true
//                     text: Net_test_VM.requestLog
//                     wrapMode: Text.Wrap
//                 }
//             }
//         }

//         // 6. 流式返回内容
//         GroupBox {
//             Layout.fillWidth: true
//             Layout.fillHeight: true
//             title: "AI 返回内容"

//             ScrollView {
//                 anchors.fill: parent
//                 TextEdit {
//                     readOnly: true
//                     text: Net_test_VM.receiveContent
//                     wrapMode: Text.Wrap
//                 }
//             }
//         }
//     }
// }
