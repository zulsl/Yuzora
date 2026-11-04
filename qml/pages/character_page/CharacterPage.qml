import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8
import QtQuick.Dialogs 6.8
import QtQuick.Controls.Material 6.8
import Yuzora 1.0


/**
 * 角色管理主页面 - PC/手机双端适配
 * PC：左右分栏 1:3 比例
 * 手机：列表页 + 栈式详情页
 */
Page {
    id: root
    header: MyToolBar{
        title:"角色管理"
        onLeftClicked: sideDrawer.open()
    }

    // 主体布局切换
    Item {
        anchors.fill: parent

        // PC端布局：左右分栏 1:3
        Loader {
            anchors.fill: parent
            active: !Theme.isMobile
            sourceComponent: pcLayout
        }

        // 手机端布局：仅列表
        Loader {
            anchors.fill: parent
            active: Theme.isMobile
            sourceComponent: mobileLayout
        }
    }

    // PC布局组件
    Component {
        id: pcLayout
        RowLayout {
            anchors.fill: parent
            visible: !Theme.isMobile
            spacing: 0

            CharacterListPanel {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1 // 宽度权重 1
            }

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 3 // 宽度权重 3
                visible: CharacterViewModel.hasSelection

                CharacterDetailForm {}
            }
        }
    }

    // 手机端列表组件
    Component {
        id: mobileLayout
        CharacterListPanel {}
    }

    // 手机端详情页组件（用于栈跳转）
    Component {
        id: characterDetailPage

        Page{
            header: MyToolBar{
                leftIconSource:"qrc:/icons/res/arrow-narrow-left.svg"
                title:"角色详情"
                onLeftClicked: mainWindow.mainStack.pop()
            }

            CharacterDetailForm {}
        }

    }


    // 全局对话框
    FileDialog {
        id: importDialog
        title: "导入角色卡"
        nameFilters: ["角色卡文件 (*.png *.json)", "所有文件 (*.*)"]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            CharacterViewModel.importCharacter(selectedFile)
        }
    }

    FileDialog {
        id: exportDialog
        title: "导出角色卡"
        nameFilters: ["PNG 角色卡 (*.png)", "JSON 角色卡 (*.json)"]
        fileMode: FileDialog.SaveFile
        defaultSuffix: "png"
        onAccepted: {
            const localPath = selectedFile.toLocalFile()
            CharacterViewModel.exportCurrentCharacter(localPath)
        }
    }
    
    // 消息提示监听
    Connections {
        target: CharacterViewModel
        function onOperationError(errorMsg) {
            errorToast.show(errorMsg)
            console.warn(errorMsg)
        }
        function onOperationTip(tipMsg) {
            tipToast.show(tipMsg)
            console.log(tipMsg)
        }
    }

}