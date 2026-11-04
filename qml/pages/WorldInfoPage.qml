import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 6.8
import QtQuick.Dialogs 6.8
import QtQuick.Controls.Material 6.8
import Yuzora 1.0

Page {
    id: root
    padding: 12
    header: MyToolBar{
        title:"世界书/知识书"
        onLeftClicked: sideDrawer.open()
    }

    RowLayout {
        anchors.fill: parent
        spacing: 12

        // ========== 左栏：文集列表 ==========
        Pane {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            padding: 0

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "世界书文集"; font.bold: true }
                    Item { Layout.fillWidth: true }
                    MyToolButton {
                        ToolTip.text:"新建"
                        icon.source: "qrc:/icons/res/file-plus.svg"
                        onClicked: WorldInfoViewModel.createBook()
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    MyToolButton {
                        ToolTip.text:"导入"
                        icon.source: "qrc:/icons/res/file-import.svg"
                        onClicked: importDialog.open()
                    }
                    MyToolButton {
                        ToolTip.text: "导出"
                        icon.source: "qrc:/icons/res/file-export.svg"
                        onClicked: exportBookDialog.open()
                        enabled: WorldInfoViewModel.currentBookId
                    }
                    MyToolButton {
                        ToolTip.text:"删除"
                        icon.source: "qrc:/icons/res/trash.svg"
                        onClicked: bookDeleteDialog.open()
                        enabled: WorldInfoViewModel.currentBookId
                    }
                    MyToolButton {
                        ToolTip.text:"重命名"
                        icon.source: "qrc:/icons/res/edit.svg"
                        onClicked: renameDialog.open()
                        enabled: WorldInfoViewModel.currentBookId
                    }
                }

                ListView {
                    id: bookListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: WorldInfoViewModel ? WorldInfoViewModel.bookModel : null
                    currentIndex: -1

                    delegate: ItemDelegate {
                        width: bookListView.width
                        highlighted: WorldInfoViewModel.currentBookId === model.id

                        onClicked: {
                            bookListView.currentIndex = index
                            WorldInfoViewModel.selectBook(model.id)
                        }

                        contentItem: RowLayout {
                            spacing: 2
                            CheckBox{
                                checked: model.isEnabled
                                onToggled: WorldInfoViewModel.toggleBookEnabled(model.id)
                            }


                            Label {
                                text: model.name
                                font.bold: true
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                                color: model.isEnabled ? palette.text : palette.mid
                            }

                        }
                    }
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
                }
            }
        }

        // ========== 右栏：条目列表 ==========
        Pane {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 3
            padding: 0

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "条目列表"; font.bold: true }
                    Item { Layout.fillWidth: true }
                    MyToolButton {
                        ToolTip.text:"新建条目"
                        icon.source: "qrc:/icons/res/file-plus.svg"
                        onClicked: WorldInfoViewModel.createEntry()
                    }
                }

                BookEntryListView{}
            }
        }

    }


    // ========== 对话框 ==========
    FileDialog {
        id: importDialog
        title: "导入世界书"
        nameFilters: ["JSON 文件 (*.json)", "所有文件 (*)"]
        fileMode: FileDialog.OpenFile
        onAccepted: WorldInfoViewModel.importBook(selectedFile)
    }
    FileDialog {
        id: exportBookDialog
        title: "导出世界书"
        nameFilters: ["JSON 文件 (*.json)"]
        fileMode: FileDialog.SaveFile
        onAccepted: WorldInfoViewModel.exportCurrentBook(fileUrl.toLocalFile())
    }
    MessageDialog {
        id: bookDeleteDialog
        title: "确认删除"
        text: "确定删除该世界书吗？"
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        onAccepted: WorldInfoViewModel.deleteBook()
    }
    Dialog {
        id: renameDialog
        title: "重命名"
        modal: true

        //按钮居中
        footer: DialogButtonBox {
            standardButtons: Dialog.Ok | Dialog.Cancel
            alignment: Qt.AlignHCenter //居中
        }
        // 全局窗口居中
        anchors.centerIn: Overlay.overlay

        onAccepted: {
            WorldInfoViewModel.currentBookName=nameInput.text
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8
            TextField {
                id: nameInput
                text: WorldInfoViewModel.currentBookName
                Layout.fillWidth: true
                placeholderText: "名称"
                focus: true
                Layout.preferredWidth: 300
            }
        }
    }


    MessageDialog {
        id: confirmDeleteEntry
        // icon: MessageDialog.Warning
        title: "确认删除"
        text: "确定删除该条目吗？"
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        // onAccepted: WorldInfoViewModel.deleteEntry(WorldInfoViewModel.currentEntry.id)
    }

}