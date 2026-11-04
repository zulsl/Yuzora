import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8
import QtQuick.Controls.Material 6.8
import Qt5Compat.GraphicalEffects
import Yuzora 1.0

/**
 * 角色列表面板 - 可复用
 * 包含：搜索栏、操作按钮、角色列表
 */
ColumnLayout {
    id: root
    spacing: 0

    // 顶部工具栏
    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        Layout.topMargin: 8
        Layout.bottomMargin: 4

        TextField {
            id: searchField
            visible: false
            Layout.fillWidth: true
            placeholderText: "搜索角色名称..."
            onTextChanged: CharacterViewModel.searchKeyword = text
            onAccepted: CharacterViewModel.searchCharacters()
        }

        MyToolButton {
            ToolTip.text:"新建"
            icon.source: "qrc:/icons/res/file-plus.svg"
            onClicked: CharacterViewModel.createNewCharacter()
        }
        MyToolButton {
            ToolTip.text:"导入角色卡"
            icon.source: "qrc:/icons/res/file-import.svg"
            onClicked: importDialog.open()
        }
    }

    // 角色列表
    ListView {
        id: characterListView
        Layout.fillHeight: true
        Layout.fillWidth: true
        clip: true
        spacing: 2
        model: CharacterViewModel.characterListModel
        currentIndex: CharacterViewModel.currentIndex
        rightMargin: 10
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOn
            width: 8
        }

        delegate: Item {
            id:delegateItem
            width: characterListView.width
            height: 68

            /*通过附加属性 ListView.view 可以拿到所属的 ListView 实例，
            配合 ListView.isCurrentItem 实现选中样式*/
            property bool isCurrent: ListView.isCurrentItem
            //交互状态
            property bool hovered: false//鼠标悬停
            property bool pressed: false//鼠标按下
            property bool checked: false
            property bool highlighted: false//选中高亮

            //边框背景
            Rectangle {
                anchors.fill: parent
                color: {
                    if (isCurrent) "#e6f2ff"
                    else if (delegateItem.pressed) "#c8def8"
                    else if (delegateItem.hovered) "#eeeeee"
                    else "#ffffff"
                }//"#3c78d8"
                border.color: isCurrent ? "#1890ff" : "transparent"
                border.width: 1
                radius: 4
                anchors.margins: 2
            }

            MouseArea {
                anchors.fill: parent

                hoverEnabled: true//悬停启用
                onEntered: delegateItem.hovered = true//鼠标进入
                onExited: {//鼠标离开
                    delegateItem.hovered = false
                    delegateItem.pressed = false
                }
                onPressed: delegateItem.pressed = true//按下
                onReleased: delegateItem.pressed = false//释放
                onCanceled: delegateItem.pressed = false//取消
                propagateComposedEvents: true//事件传播

                onClicked: {
                    //切换界面
                    CharacterViewModel.setCurrentIndex(index)
                    CharacterViewModel.selectCharacter(model.id)
                    CharacterViewModel.openSelectedCharChat();
                }
            }

            //项
            RowLayout {
                anchors.fill: parent
                spacing: 10


                // ----- 头像 -----
                Item {
                    id: avatarImage
                    Layout.leftMargin: 8
                    width: 52
                    height: 52

                    // 原始图片（仅作为遮罩输入，不直接显示）
                    Image {
                        id: avatarSource
                        anchors.fill: parent
                        source: avatarPath
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
                // //头像
                // Rectangle {
                //     Layout.leftMargin: 8
                //     width: 52
                //     height: 52
                //     clip: true
                //     Image {
                //         anchors.fill: parent
                //         source: avatarPath
                //         fillMode: Image.PreserveAspectCrop
                //         mipmap: true
                //         smooth: true
                //     }
                // }

                //文本
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: name
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                //更多
                ToolButton {
                    width: 28
                    height: 28
                    icon.width: 18
                    icon.height: 18
                    icon.source: "qrc:/icons/res/dots.svg"
                    onClicked: {
                        CharacterViewModel.setCurrentIndex(index)
                        CharacterViewModel.selectCharacter(model.id)
                        // 手机端点击更多推入详情页
                        if (Theme.isMobile) {
                            mainWindow.mainStack.push(characterDetailPage)
                        }
                    }
                }
            }
        }
    }
}