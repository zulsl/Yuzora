import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 隐私说明全文
// 只讲数据处理事实，不写免责声明，也不声称做了加密。
Dialog {
    id: root

    property string privacyText: [
        "本软件由开发者作为客户端分发，开发者不运营业务服务端，不收集、不上传你的聊天记录。",
        "会话、设置和 API Key 保存在本地。使用模型时，消息会直接发送给你自行配置的第三方 API 提供商；其数据处理受该提供商的隐私政策约束。",
        "“检查更新”会访问 GitHub ；点击 GitHub、QQ 群等外链会交给系统浏览器或对应客户端处理。",
        "第三方模型服务可能记录请求内容，请自行判断是否发送敏感信息。"
    ].join("\n\n")

    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(560, Theme.windowWidth - Theme.spaceXl * 2)
    height: Math.min(620, Theme.windowHeight * 0.72)
    padding: 0
    closePolicy: Popup.CloseOnEscape

    onOpened: {
        if (scroll.contentItem)
            scroll.contentItem.contentY = 0
    }

    background: Rectangle {
        color: Theme.bgElevated
        radius: Theme.radiusMd
        border.width: 1
        border.color: Theme.borderDefault
    }

    contentItem: Item {
        implicitWidth: 560
        implicitHeight: 400
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spaceLg
            spacing: Theme.spaceSm

            Label {
                Layout.fillWidth: true
                text: "隐私说明"
                font.pixelSize: Theme.fontTitle
                font.bold: true
                font.family: Theme.fontFamily
                color: Theme.textPrimary
            }

            RowLayout {
                Layout.fillWidth: true

                Item { Layout.fillWidth: true }

                AppButton {
                    text: "关闭"
                    variant: "secondary"
                    onClicked: root.close()
                }
            }

            ScrollView {
                id: scroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                TextArea {
                    width: scroll.availableWidth
                    text: root.privacyText
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextArea.WrapAtWordBoundaryOrAnywhere
                    textFormat: TextArea.PlainText
                    font.pixelSize: Theme.fontBodySm
                    font.family: Theme.fontFamily
                    color: Theme.textSecondary
                    padding: 0
                    background: null
                }
            }
        }
    }
}
