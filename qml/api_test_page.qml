// import QtQuick 6.8
// import QtQuick.Controls 6.8
// import QtQuick.Layouts 6.8
// import QtQuick.Dialogs 6.8


// Page {
//     id: page

//     ScrollView {
//         id: scrollView
//         anchors.fill: parent
//         contentWidth: availableWidth

//         Column {
//             anchors.fill: parent
//             anchors.bottomMargin: -9

//             Text {
//                 id: text1
//                 text: "你好"
//                 font.pixelSize: 20
//             }

//             Text {
//                 id: text2
//                 text: "你好2"
//                 font.pixelSize: 12
//             }

//             Button {
//                 id: button
//                 text: "按钮1"
//                 onClicked: importDialog.open()
//             }

//             FileDialog {
//                 id: importDialog
//                 title: "导入角色卡"
//                 nameFilters: ["角色卡文件 (*.png *.json)", "所有文件 (*.*)"]
//                 fileMode: FileDialog.OpenFile
//                 onAccepted: {
//                     const path = file.localPath
//                     // const path = file.toString().replace(/^file:\/{3}/, "")
//                     // const path = fileUrl.toString().replace(/^file:\/\//, "")
//                     characterViewModel.importCharacter(path)
//                 }
//             }

//             Repeater {
//                 model: 20
//                 delegate: Text {
//                     text: "Scroll me !"
//                 }
//             }

//         }
//     }

// }
