import QtQuick 6.8
import QtQuick.Controls
import QtQuick.Layouts 6.8
import QtQuick.Window 6.8
import QtQuick.Effects 6.8
import QtQuick.Dialogs 6.8
import QtQuick.Controls.Material 6.8

import Yuzora 1.0

//主界面
//实现「左侧抽屉导航 + 页面栈切换」，默认首页为聊天界面，侧拉栏 5 个按钮对应全部功能页，完全匹配需求。

ApplicationWindow {
    id: mainWindow
    width: 1280
    height: 720
    maximumWidth: Screen.width
    // minimumWidth: 800
    // minimumHeight: 600
    visible: true

// 全局主题配置
    Material.theme: Theme.materialTheme//明暗主题-浅色Light 深色模式Dark
    Material.primary: Theme.primaryColor//主色
    Material.accent: Qt.lighter(Material.primary, 1.2)
    //Material.accent: Material.color(Material.Blue) //强调色
    //Material.foreground：前景色。默认根据 theme（亮/暗）自动变化（黑/白），
    //可以手动覆盖改变全局文字和图标的默认颜色。
    //Material.background：背景色。默认根据 theme 变化，可以手动覆盖页面背景色。

    font: Theme.applicationFont
    color: Theme.backgroundColor



// ========== 左侧侧边抽屉导航 ==========
    Drawer {
        id: sideDrawer
        width: 260
        height: parent.height
        padding: Theme.page_padding
        edge: Qt.LeftEdge
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        ColumnLayout {//(未完成)
            // anchors.fill: parent
            // spacing: 4

            // // 应用标题
            // Label {
            //     text: "Yuzora"
            //     font.pixelSize: 20
            //     font.bold: true
            //     Layout.bottomMargin: 12
            //     Layout.leftMargin: 8
            // }

            // ListModel {
            //     id: navModel
            //     ListElement { icon: "💬"; title: "聊天界面"; page: chatPage; transition: StackView.Immediate }
            //     ListElement { icon: "👤"; title: "角色管理"; page: characterPage; transition: StackView.Fade }
            //     ListElement { icon: "⚙️"; title: "对话补全预设"; page: presetPage; transition: StackView.Immediate }
            //     ListElement { icon: "🌐"; title: "API连接配置"; page: apiConfigPage; transition: StackView.Immediate }
            //     ListElement { icon: "📖"; title: "世界书管理"; page: worldInfoPage; transition: StackView.Immediate }
            //     ListElement { icon: "🔧"; title: "系统设置"; page: settingsPage; transition: StackView.Immediate }
            // }

            // ListView {
            //     id: navList
            //     Layout.fillWidth: true
            //     Layout.fillHeight: true
            //     model: navModel
            //     clip: true

            //     delegate: ItemDelegate {
            //         width: navList.width
            //         height: 40

            //         onClicked: {
            //             sideDrawer.close()
            //             mainStack.replace(model.page, model.transition)
            //         }

            //         contentItem: RowLayout {
            //             spacing: 8
            //             Label {
            //                 text: model.icon
            //             }
            //             Label {
            //                 text: model.title
            //             }
            //         }
            //     }
            // }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 4


            //顶部用户区域
            Pane {
                id:top
                Layout.fillWidth: true
                // height:72
                padding:16
                // bottomPadding: 12

                // ── 对外属性 ──
                property int size: 72
                property bool editable: true

                implicitWidth: size
                implicitHeight: size

                RowLayout{
                    anchors.fill: parent
                    spacing: 16

                    //头像
                    Rectangle{
                        width: 52
                        height: 52

                        // ── 圆形遮罩（不可见，只提供纹理）──
                        Rectangle {
                            id: mask
                            anchors.fill: parent
                            radius: width / 2
                            visible: false
                            layer.enabled: true
                        }

                        // ── 头像本体 ──
                        Image {
                            id: avatarImage
                            anchors.fill: parent
                            source: SettingsViewModel.userAvatar
                            sourceSize: Qt.size(top.size * 2, top.size * 2)   // 高分屏不糊
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            cache: false                                        // 避免换图不刷新

                            layer.enabled: true
                            layer.effect: MultiEffect {
                                maskEnabled: true
                                maskSource: mask
                                maskThresholdMin: 0.5
                                maskSpreadAtMin: 1.0
                            }
                        }

                        // ── Hover 遮罩 + 相机图标 ──
                        Rectangle {
                            anchors.fill: parent
                            radius: width / 2
                            color: "#66000000"
                            opacity: (hoverArea.containsMouse && top.editable) ? 1 : 0
                            Behavior on opacity { NumberAnimation { duration: 140 } }

                            Label {
                                anchors.centerIn: parent
                                text: "📷"
                                font.pixelSize: top.size * 0.32
                            }
                        }

                        MouseArea {
                            id: hoverArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: pictureDialog.open()
                        }

                    }

                    //---用户名---
                    TextInput {
                        id: nameInput
                        text: SettingsViewModel.userName
                        // font.pixelSize: 18
                        font.bold: true
                        // color: "white"
                        readOnly: true                      // 默认只读，只能显示
                        selectByMouse: true

                        // 双击解锁编辑
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            cursorShape: Qt.IBeamCursor
                            onDoubleClicked: {
                                nameInput.readOnly = false
                                nameInput.forceActiveFocus()
                                // nameInput.selectAll()
                            }
                        }

                        // 回车 / 失焦 → 锁定并保存
                        onAccepted: nameInput.readOnly = true
                        onActiveFocusChanged: {
                            if (!activeFocus) nameInput.readOnly = true
                        }

                        // Esc → 还原
                        Keys.onEscapePressed: nameInput.readOnly = true

                        // 保存
                        onReadOnlyChanged: {
                            if (readOnly) SettingsViewModel.userName = text.trim() || SettingsViewModel.userName
                        }
                    }

                    Item {Layout.fillWidth: true}
                }



                // ── 文件选择 ──
                FileDialog {
                    id: pictureDialog
                    title: "选择头像图片"
                    nameFilters: ["图片 (*.png *.jpg *.jpeg *.bmp *.webp)"]
                    onAccepted: {
                        SettingsViewModel.userAvatar = selectedFile
                    }
                }
            }

            // 导航按钮组
            Button {
                icon.source: "qrc:/icons/res/main_page/message-circle.svg"
                text: "聊天界面"
                Layout.fillWidth: true
                onClicked: {
                    sideDrawer.close()
                    mainStack.replace(chatPage)
                }
            }
            Button {
                icon.source: "qrc:/icons/res/main_page/plug-connected.svg"
                text: "API连接配置"
                Layout.fillWidth: true
                onClicked: {
                    sideDrawer.close()
                    mainStack.replace(apiConfigPage)
                }
            }
            Button {
                icon.source: "qrc:/icons/res/main_page/adjustments-horizontal.svg"
                text: "对话补全预设"
                Layout.fillWidth: true
                onClicked: {
                    sideDrawer.close()
                    mainStack.replace(presetPage)
                }
            }
            Button {
                icon.source: "qrc:/icons/res/main_page/users.svg"
                text: "角色管理"
                Layout.fillWidth: true
                onClicked: {
                    sideDrawer.close()
                    mainStack.replace(characterPage,StackView.Fade)
                }
            }
            Button {
                icon.source: "qrc:/icons/res/main_page/book-2.svg"
                text: "世界书管理"
                Layout.fillWidth: true
                onClicked: {
                    sideDrawer.close()
                    mainStack.replace(worldInfoPage)
                }
            }
            Button {
                icon.source: "qrc:/icons/res/main_page/settings.svg"
                text: "系统设置"
                Layout.fillWidth: true
                onClicked: {
                    sideDrawer.close()
                    mainStack.replace(settingsPage)
                }
            }

            Item { Layout.fillHeight: true }//占满垂直高度
        }
    }


    // ========== 主内容页面栈 ==========
    StackView {
        id: mainStack
        anchors.fill: parent
        initialItem: chatPage

        // 聊天主界面
        Component {
            id: chatPage
            ChatPage {}
        }

        // API连接配置界面
        Component {
            id: apiConfigPage
            ApiConfigPage {}
        }


        // 角色管理界面
        Component {
            id: characterPage
            CharacterPage {}
        }

        // 对话补全预设界面
        Component {
            id: presetPage
            CompletionPresetPage {}
        }


        // 世界书管理界面
        Component {
            id: worldInfoPage
            WorldInfoPage {}
        }

        // 设置界面
        Component {
            id: settingsPage
            SettingsPage {}
        }
    }



    // ========== 安卓物理返回键处理(未完成) ==========
    // Keys.onBackPressed: {
    //     // 抽屉打开则先关抽屉
    //     if (sideDrawer.opened) {
    //         sideDrawer.close()
    //     }
    //     // 页面栈深度>1则返回上一页
    //     else if (stackView.depth > 1) {
    //         stackView.pop()
    //     }
    //     // 首页则退出应用
    //     else {
    //         mainWindow.close()
    //     }
    // }


    // 全局错误监听
    Connections {
        target: ExceptionHandler
        function onErrorOccurred(code, message) {
            errorToast.show(message)
            console.warn("操作错误:", message)
        }
        function onTipOccurred(message) {
            tipToast.show(message)
            console.log("操作提示:", message)
        }
    }

    //角色切换跳转界面
    Connections {
        target: CharacterViewModel
        function onNavigateToChatPage() {
            mainStack.replace(chatPage)
        }
    }

    // 全局错误吐司提示
    ToolTip {
        id: errorToast
        x: (mainWindow.width - width) / 2
        y: mainWindow.height - height - 32
        timeout: 3000
        background: Rectangle {
            color: "#ff4444"
            radius: 6
        }
    }
    // 全局消息吐司提示
    ToolTip {
        id: tipToast
        x: (mainWindow.width - width) / 2
        y: mainWindow.height - height - 32
        timeout: 3000
        background: Rectangle {
            color: "#32CD32"
            radius: 6
        }
    }
}