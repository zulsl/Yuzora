// import QtQuick 2.15
// import QtQuick.Controls 2.15
// import QtQuick.Layouts 1.15
// import QtQuick.Dialogs
// import Qt.labs.platform 1.1 as Platform

// // 测试 CharacterManager 类的完整 QML 界面
// // 依赖 main.cpp 中已将 CharacterManager 单例注册为 "CharacterManager"
// ApplicationWindow {
//     visible: true
//     width: 1024
//     height: 768
//     title: "角色管理器测试 - " + CharacterManager.objectName

//     // 角色列表模型
//     ListModel {
//         id: characterListModel
//     }

//     // 当前选中的角色 ID
//     property string currentCharacterId: ""

//     // 刷新角色列表（调用 CharacterManager 并更新 Model）
//     function refreshCharacterList() {
//         // 注意：getAllCharacters() 在 C++ 端应返回 QVariantList<QVariantMap>
//         // 若返回类型不符，可在 C++ 端增加 getAllCharactersVariant() 方法，
//         // 或使用 CharacterManager 注册的角色列表属性。
//         var characters = CharacterManager.getAllCharacters()
//         characterListModel.clear()
//         for (var i = 0; i < characters.length; ++i) {
//             var c = characters[i]
//             characterListModel.append({
//                 id: c.id,
//                 name: c.name,
//                 description: c.description,
//                 avatarPath: c.avatarPath,
//                 version: c.version,
//                 createdAt: c.createdAt
//             })
//         }
//         if (characterListModel.count > 0 && currentCharacterId === "") {
//             currentCharacterId = characterListModel.get(0).id
//         }
//     }

//     // 监听 CharacterManager 的信号，实时刷新列表
//     Connections {
//         target: CharacterManager
//         function onCharacterListChanged() { refreshCharacterList() }
//         function onCharacterUpdated(charId) { refreshCharacterList() }
//         function onCharacterDeleted(charId) { refreshCharacterList() }
//     }

//     // 初始化加载角色
//     Component.onCompleted: {
//         refreshCharacterList()
//         infoText.text = "就绪，共 " + characterListModel.count + " 个角色"
//     }

//     // 主布局
//     RowLayout {
//         anchors.fill: parent
//         anchors.margins: 10
//         spacing: 10

//         // 左侧：角色列表
//         Rectangle {
//             Layout.preferredWidth: 300
//             Layout.fillHeight: true
//             border.color: "#ccc"
//             radius: 5

//             ColumnLayout {
//                 anchors.fill: parent
//                 anchors.margins: 5
//                 spacing: 5

//                 Text {
//                     text: "角色列表"
//                     font.bold: true
//                     font.pixelSize: 18
//                 }

//                 ListView {
//                     id: listView
//                     Layout.fillWidth: true
//                     Layout.fillHeight: true
//                     model: characterListModel
//                     clip: true
//                     delegate: Rectangle {
//                         width: parent.width
//                         height: 60
//                         color: ListView.isCurrentItem ? "#e0e0e0" : "transparent"
//                         border.color: "#ddd"
//                         radius: 3

//                         RowLayout {
//                             anchors.fill: parent
//                             anchors.margins: 5
//                             spacing: 8

//                             // 头像占位
//                             Rectangle {
//                                 width: 48
//                                 height: 48
//                                 radius: 24
//                                 color: "#aaa"
//                                 Image {
//                                     anchors.fill: parent
//                                     source: model.avatarPath ? "file:///" + model.avatarPath : ""
//                                     fillMode: Image.PreserveAspectCrop
//                                     visible: source != ""
//                                 }
//                                 Text {
//                                     anchors.centerIn: parent
//                                     text: model.name ? model.name.charAt(0).toUpperCase() : "?"
//                                     visible: parent.children[0].source == ""
//                                     font.pixelSize: 20
//                                 }
//                             }

//                             ColumnLayout {
//                                 Layout.fillWidth: true
//                                 spacing: 2
//                                 Text {
//                                     text: model.name
//                                     font.bold: true
//                                     font.pixelSize: 14
//                                     elide: Text.ElideRight
//                                     Layout.fillWidth: true
//                                 }
//                                 Text {
//                                     text: "版本: " + (model.version === 2 ? "V2" : "V1")
//                                     font.pixelSize: 10
//                                     color: "gray"
//                                 }
//                                 Text {
//                                     text: model.description ? model.description.substring(0, 30) + "..." : ""
//                                     font.pixelSize: 10
//                                     color: "#555"
//                                     elide: Text.ElideRight
//                                     Layout.fillWidth: true
//                                 }
//                             }
//                         }

//                         MouseArea {
//                             anchors.fill: parent
//                             onClicked: {
//                                 listView.currentIndex = index
//                                 currentCharacterId = model.id
//                                 loadCharacterDetails(model.id)
//                             }
//                         }
//                     }
//                 }

//                 Button {
//                     text: "新建角色"
//                     Layout.fillWidth: true
//                     onClicked: openCreateDialog()
//                 }

//                 Button {
//                     text: "从文件导入角色卡"
//                     Layout.fillWidth: true
//                     onClicked: importFileDialog.open()
//                 }

//                 Button {
//                     text: "导入角色 (JSON)"
//                     Layout.fillWidth: true
//                     onClicked: importJsonDialog.open()
//                 }
//             }
//         }

//         // 右侧：角色详情及操作
//         Rectangle {
//             Layout.fillWidth: true
//             Layout.fillHeight: true
//             border.color: "#ccc"
//             radius: 5

//             ColumnLayout {
//                 anchors.fill: parent
//                 anchors.margins: 10
//                 spacing: 10

//                 Text {
//                     text: "角色详情"
//                     font.bold: true
//                     font.pixelSize: 18
//                 }

//                 ScrollView {
//                     Layout.fillWidth: true
//                     Layout.fillHeight: true
//                     clip: true

//                     ColumnLayout {
//                         width: parent.width
//                         spacing: 12

//                         // 基本信息展示及编辑
//                         LabeledTextField {
//                             label: "ID"
//                             text: characterDetail.id
//                             readOnly: true
//                             Layout.fillWidth: true
//                         }

//                         LabeledTextField {
//                             id: nameField
//                             label: "名称 *"
//                             text: characterDetail.name
//                             Layout.fillWidth: true
//                         }

//                         LabeledTextField {
//                             id: descField
//                             label: "描述"
//                             text: characterDetail.description
//                             Layout.fillWidth: true
//                         }

//                         LabeledTextField {
//                             id: personalityField
//                             label: "性格"
//                             text: characterDetail.personality
//                             Layout.fillWidth: true
//                         }

//                         LabeledTextField {
//                             id: firstMsgField
//                             label: "首发消息"
//                             text: characterDetail.firstMessage
//                             Layout.fillWidth: true
//                         }

//                         LabeledTextField {
//                             id: systemPromptField
//                             label: "系统提示词"
//                             text: characterDetail.systemPrompt
//                             Layout.fillWidth: true
//                         }

//                         RowLayout {
//                             Layout.fillWidth: true
//                             Button {
//                                 text: "更新角色"
//                                 onClicked: updateCurrentCharacter()
//                             }
//                             Button {
//                                 text: "复制角色"
//                                 onClicked: duplicateCurrentCharacter()
//                             }
//                             Button {
//                                 text: "删除角色"
//                                 onClicked: deleteCurrentCharacter()
//                                 highlighted: true
//                             }
//                         }

//                         RowLayout {
//                             Layout.fillWidth: true
//                             Button {
//                                 text: "导出为 PNG"
//                                 onClicked: exportPngDialog.open()
//                             }
//                             Button {
//                                 text: "导出为 JSON"
//                                 onClicked: exportCurrentCharacterToJson()
//                             }
//                         }

//                         Rectangle {
//                             height: 1
//                             color: "#ccc"
//                             Layout.fillWidth: true
//                         }

//                         Text {
//                             text: "操作日志"
//                             font.bold: true
//                         }
//                         TextArea {
//                             id: infoText
//                             Layout.fillWidth: true
//                             Layout.minimumHeight: 100
//                             readOnly: true
//                             wrapMode: TextEdit.Wrap
//                             text: "等待操作..."
//                         }
//                     }
//                 }
//             }
//         }
//     }

//     // 存储当前查看的角色详细 (JS 对象)
//     property var characterDetail: ({
//         id: "",
//         name: "",
//         description: "",
//         personality: "",
//         firstMessage: "",
//         systemPrompt: "",
//         avatarPath: "",
//         version: 2,
//         extensions: ({})
//     })

//     // 加载角色详情
//     function loadCharacterDetails(charId) {
//         if (!charId) return
//         var detail = CharacterManager.getCharacterById(charId)
//         if (detail) {
//             characterDetail.id = detail.id
//             characterDetail.name = detail.name
//             characterDetail.description = detail.description
//             characterDetail.personality = detail.personality
//             characterDetail.firstMessage = detail.firstMessage
//             characterDetail.systemPrompt = detail.systemPrompt
//             characterDetail.avatarPath = detail.avatarPath
//             characterDetail.version = detail.version
//             characterDetail.extensions = detail.extensions || ({})
//             // 刷新输入框显示
//             nameField.text = detail.name
//             descField.text = detail.description
//             personalityField.text = detail.personality
//             firstMsgField.text = detail.firstMessage
//             systemPromptField.text = detail.systemPrompt
//         } else {
//             infoText.text = "错误：无法加载角色 " + charId
//         }
//     }

//     // 更新当前角色
//     function updateCurrentCharacter() {
//         if (!currentCharacterId) {
//             infoText.text = "请先选择一个角色"
//             return
//         }
//         var updated = {
//             id: currentCharacterId,
//             name: nameField.text,
//             description: descField.text,
//             personality: personalityField.text,
//             firstMessage: firstMsgField.text,
//             systemPrompt: systemPromptField.text,
//             avatarPath: characterDetail.avatarPath,
//             version: characterDetail.version,
//             extensions: characterDetail.extensions
//         }
//         var success = CharacterManager.updateCharacter(updated)
//         if (success) {
//             infoText.text = "角色更新成功: " + updated.name
//             refreshCharacterList()
//             loadCharacterDetails(currentCharacterId)
//         } else {
//             infoText.text = "更新失败，请检查字段"
//         }
//     }

//     // 复制当前角色
//     function duplicateCurrentCharacter() {
//         if (!currentCharacterId) return
//         var newId = ""
//         var ok = CharacterManager.duplicateCharacter(currentCharacterId, newId)
//         if (ok) {
//             infoText.text = "复制成功，新角色ID: " + newId
//             refreshCharacterList()
//             currentCharacterId = newId
//             loadCharacterDetails(newId)
//         } else {
//             infoText.text = "复制失败"
//         }
//     }

//     // 删除当前角色
//     function deleteCurrentCharacter() {
//         if (!currentCharacterId) return
//         var success = CharacterManager.deleteCharacter(currentCharacterId)
//         if (success) {
//             infoText.text = "角色已删除: " + characterDetail.name
//             currentCharacterId = ""
//             characterDetail = ({ id:"", name:"", description:"", personality:"", firstMessage:"", systemPrompt:"" })
//             refreshCharacterList()
//         } else {
//             infoText.text = "删除失败"
//         }
//     }

//     // 打开创建角色对话框
//     function openCreateDialog() {
//         createDialog.name = ""
//         createDialog.desc = ""
//         createDialog.personality = ""
//         createDialog.firstMsg = ""
//         createDialog.systemPrompt = ""
//         createDialog.open()
//     }

//     // 导出 PNG 对话框
//     Platform.FileDialog {
//         id: exportPngDialog
//         title: "导出角色为 PNG"
//         fileMode: Platform.FileDialog.SaveFile
//         nameFilters: ["PNG 图片 (*.png)"]
//         onAccepted: {
//             var savePath = file.toString().replace(/^file:\/\//, "")
//             var ok = CharacterManager.exportCharacterToPng(currentCharacterId, savePath)
//             infoText.text = ok ? "导出成功: " + savePath : "导出失败"
//         }
//     }

//     // 导入 PNG 对话框
//     Platform.FileDialog {
//         id: importFileDialog
//         title: "导入角色卡"
//         fileMode: Platform.FileDialog.OpenFile
//         nameFilters: ["PNG 或 json (*.png;*.json)"]
//         onAccepted: {
//             var filePath = file.toString().replace(/^file:\/{3}/, "")
//             console.log(filePath)
//             var newChar = CharacterService.importCharacterFromFile(filePath)
//             console.log(newChar)
//             if (newChar && newChar.id) {
//                 infoText.text = "导入成功: " + newChar.name
//                 refreshCharacterList()
//                 currentCharacterId = newChar.id
//                 loadCharacterDetails(newChar.id)
//             } else {
//                 infoText.text = "导入失败，文件可能不是有效的角色卡"
//             }
//         }
//     }

//     // 导入 JSON 对话框
//     Platform.FileDialog {
//         id: importJsonDialog
//         title: "导入 JSON 角色卡"
//         fileMode: Platform.FileDialog.OpenFile
//         nameFilters: ["JSON 文件 (*.json)"]
//         onAccepted: {
//             var filePath = file.toString().replace(/^file:\/\//, "")
//             // 需要读取文件内容并转为 JSON 对象，实际项目可用 C++ 辅助，这里演示直接调用 importCharacterFromJson
//             // 注：importCharacterFromJson 接受 QJsonObject，在 QML 中传递 JavaScript 对象可能自动转换
//             var fileContent = readFileUtf8(filePath) // 需自己实现或使用 C++ 工具，简化演示
//             if (fileContent) {
//                 var jsonObj = JSON.parse(fileContent)
//                 var newChar = CharacterManager.importCharacterFromJson(jsonObj)
//                 if (newChar && newChar.id) {
//                     infoText.text = "JSON 导入成功: " + newChar.name
//                     refreshCharacterList()
//                     currentCharacterId = newChar.id
//                     loadCharacterDetails(newChar.id)
//                 } else {
//                     infoText.text = "JSON 导入失败"
//                 }
//             } else {
//                 infoText.text = "无法读取文件"
//             }
//         }
//     }

//     // 导出当前角色为 JSON
//     function exportCurrentCharacterToJson() {
//         var jsonObj = CharacterManager.exportCharacterToJson(currentCharacterId)
//         if (jsonObj) {
//             var jsonStr = JSON.stringify(jsonObj, null, 2)
//             saveJsonDialog.jsonContent = jsonStr
//             saveJsonDialog.open()
//         } else {
//             infoText.text = "导出 JSON 失败"
//         }
//     }

//     Platform.FileDialog {
//         id: saveJsonDialog
//         property string jsonContent: ""
//         title: "保存 JSON 文件"
//         fileMode: Platform.FileDialog.SaveFile
//         nameFilters: ["JSON 文件 (*.json)"]
//         onAccepted: {
//             var savePath = file.toString().replace(/^file:\/\//, "")
//             var ok = writeFileUtf8(savePath, saveJsonDialog.jsonContent) // 辅助写文件
//             infoText.text = ok ? "JSON 保存成功" : "保存失败"
//         }
//     }

//     // 创建角色对话框
//     Dialog {
//         id: createDialog
//         title: "新建角色"
//         modal: true
//         standardButtons: Dialog.Ok | Dialog.Cancel
//         property string name: ""
//         property string desc: ""
//         property string personality: ""
//         property string firstMsg: ""
//         property string systemPrompt: ""

//         onAccepted: {
//             var newCharacter = {
//                 id: "",   // 由 manager 生成
//                 name: createDialog.name,
//                 description: createDialog.desc,
//                 personality: createDialog.personality,
//                 firstMessage: createDialog.firstMsg,
//                 systemPrompt: createDialog.systemPrompt,
//                 avatarPath: "",
//                 version: 2,
//                 extensions: ({})
//             }
//             var newId = CharacterManager.createCharacter(newCharacter)
//             if (newId !== "") {
//                 infoText.text = "角色创建成功: " + createDialog.name + " ID:" + newId
//                 refreshCharacterList()
//                 currentCharacterId = newId
//                 loadCharacterDetails(newId)
//             } else {
//                 infoText.text = "角色创建失败"
//             }
//         }

//         ColumnLayout {
//             anchors.fill: parent
//             LabeledTextField { label: "名称 *"; text: createDialog.name; onTextChanged: createDialog.name = text; Layout.fillWidth: true }
//             LabeledTextField { label: "描述"; text: createDialog.desc; onTextChanged: createDialog.desc = text; Layout.fillWidth: true }
//             LabeledTextField { label: "性格"; text: createDialog.personality; onTextChanged: createDialog.personality = text; Layout.fillWidth: true }
//             LabeledTextField { label: "首发消息"; text: createDialog.firstMsg; onTextChanged: createDialog.firstMsg = text; Layout.fillWidth: true }
//             LabeledTextField { label: "系统提示词"; text: createDialog.systemPrompt; onTextChanged: createDialog.systemPrompt = text; Layout.fillWidth: true }
//         }
//     }

//     // 辅助组件：带标签的文本框
//     component LabeledTextField: ColumnLayout {
//         property string label: ""
//         property string text: ""
//         property bool readOnly: false
//         spacing: 2
//         Text { text: label; font.pixelSize: 12; color: "#555" }
//         TextField {
//             text: parent.text
//             readOnly: parent.readOnly
//             onTextChanged: parent.text = text
//             Layout.fillWidth: true
//         }
//     }

//     // 简化的文件读写辅助函数（实际应通过 C++ 文件工具实现）
//     function readFileUtf8(path) {
//         // 占位，实际可通过 Qt.callMethod 调用 Utils.readJsonFile 等
//         console.warn("需要实现 readFileUtf8，建议使用 Utils.readJsonFile 返回字符串")
//         return ""
//     }
//     function writeFileUtf8(path, content) {
//         console.warn("需要实现 writeFileUtf8，建议使用 Utils.writeJsonFile")
//         return false
//     }
// }