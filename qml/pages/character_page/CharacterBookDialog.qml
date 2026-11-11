import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 角色世界书绑定管理器（单本语义）
// 职责：查看 / 解绑当前绑定；无绑定时从已有文集中挑一本继续绑定。
// 关键：绑定只写入**编辑草稿**，随表单的「保存」一起提交，本对话框不直接改库。
// 状态二态：有绑定 → 一行文集名 + 解绑；无绑定 → 直接铺开可选文集列表。
Dialog {
    id: root

    // 文集列表最多这么高，超出滚动，避免文集多时把对话框撑出屏幕
    property int maxListHeight: Theme.controlLg * 5

    readonly property bool bound: CharacterViewModel.characterBookId.length > 0
    // 绑定的文集已被删除：id 还在草稿里，但查不到名字
    readonly property bool missing: root.bound
                                    && CharacterViewModel.characterBookName.length === 0

    title: "角色世界书绑定"
    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(parent ? parent.width - Theme.spaceXl * 2 : 360, 460)
    padding: Theme.spaceLg
    closePolicy: Popup.CloseOnEscape

    // 世界书页可能刚增删或改名过文集，打开时刷新列表并强制重算绑定展示。
    // 第二步是必要的：只读属性的值会被 QML 缓存，漏发一次 NOTIFY 就会一直显示旧名字。
    onAboutToShow: {
        WorldInfoViewModel.refreshBookList()
        CharacterViewModel.refreshCharacterBookBinding()
    }

    background: Rectangle {
        color: Theme.bgElevated
        radius: Theme.radiusMd
        border.width: 1
        border.color: Theme.borderDefault
    }

    contentItem: ColumnLayout {
        spacing: Theme.spaceMd

        Label {
            Layout.fillWidth: true
            text: "使用该角色聊天时会自动启用绑定的世界书"
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.fontBody
            font.family: Theme.fontFamily
            color: Theme.textPrimary
        }

        // ---------- 有绑定：文集名 + 解绑 ----------
        RowLayout {
            Layout.fillWidth: true
            visible: root.bound
            spacing: Theme.spaceSm

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: Theme.controlLg
                radius: Theme.radiusSm
                color: Theme.bgSurfaceVariant
                border.width: 1
                border.color: Theme.borderDefault

                Label {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spaceMd
                    anchors.rightMargin: Theme.spaceMd
                    verticalAlignment: Text.AlignVCenter
                    text: root.missing ? "（原世界书已不存在）"
                                       : CharacterViewModel.characterBookName
                    font.pixelSize: Theme.fontBody
                    font.family: Theme.fontFamily
                    color: root.missing ? Theme.textDisabled : Theme.textPrimary
                    elide: Text.ElideRight
                }
            }

            AppButton {
                text: "解绑"
                variant: "danger"
                onClicked: CharacterViewModel.unbindCharacterBook()
            }
        }

        // ---------- 无绑定：从已有文集中选择 ----------
        ColumnLayout {
            Layout.fillWidth: true
            visible: !root.bound
            spacing: Theme.spaceXs

            ListView {
                id: bookList
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(contentHeight, root.maxListHeight)
                clip: true
                spacing: Theme.spaceXs
                model: WorldInfoViewModel.bookModel
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                delegate: ItemDelegate {
                    id: bookDelegate
                    required property int index
                    required property var modelData

                    width: ListView.view ? ListView.view.width : 0
                    height: Theme.touchTarget
                    // 停用的文集也能绑：启用是全局开关，绑定是另一个维度
                    onClicked: {
                        CharacterViewModel.bindCharacterBook(bookDelegate.modelData.id)
                        root.close()
                    }

                    contentItem: RowLayout {
                        spacing: Theme.spaceSm

                        Label {
                            Layout.fillWidth: true
                            text: bookDelegate.modelData.name || ""
                            font.pixelSize: Theme.fontBody
                            font.family: Theme.fontFamily
                            elide: Text.ElideRight
                            color: bookDelegate.modelData.isEnabled ? Theme.textPrimary
                                                                    : Theme.textDisabled
                        }

                        Label {
                            visible: !bookDelegate.modelData.isEnabled
                            text: "未启用"
                            font.pixelSize: Theme.fontCaption
                            font.family: Theme.fontFamily
                            color: Theme.textDisabled
                        }
                    }

                    background: Rectangle {
                        radius: Theme.radiusSm
                        color: bookDelegate.hovered ? Theme.rowHover : "transparent"
                    }
                }
            }

            EmptyState {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.controlLg * 2
                visible: bookList.count === 0
                text: "还没有世界书"
                detail: "请先在世界书页创建或导入文集"
            }
        }
    }

    footer: Item {
        implicitHeight: Theme.touchTarget + Theme.spaceSm
        AppButton {
            anchors.right: parent.right
            anchors.rightMargin: Theme.spaceLg
            anchors.verticalCenter: parent.verticalCenter
            text: "关闭"
            variant: "ghost"
            onClicked: root.close()
        }
    }
}
