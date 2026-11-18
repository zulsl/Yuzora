import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 捐赠二维码
// 只显示裁切后的支付宝收款码；不显示遮罩姓名、不点击放大、不设金额按钮。
Dialog {
    id: root

    readonly property int qrSize: Math.min(260, Math.max(160, width - Theme.spaceLg * 2))

    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(420, Theme.windowWidth - Theme.spaceXl * 2)
    height: Math.min(520, Theme.windowHeight * 0.70)
    padding: 0
    closePolicy: Popup.CloseOnEscape

    background: Rectangle {
        color: Theme.bgElevated
        radius: Theme.radiusMd
        border.width: 1
        border.color: Theme.borderDefault
    }

    contentItem: Item {
        implicitWidth: 420
        implicitHeight: 400
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spaceLg
            spacing: Theme.spaceSm

            RowLayout {
                Layout.fillWidth: true

                Item { Layout.fillWidth: true }

                AppButton {
                    text: "关闭"
                    variant: "secondary"
                    onClicked: root.close()
                }
            }

            Label {
                Layout.fillWidth: true
                text: "支持开发"
                font.pixelSize: Theme.fontTitle
                font.bold: true
                font.family: Theme.fontFamily
                color: Theme.textPrimary
            }

            Label {
                Layout.fillWidth: true
                text: "如果语昼对你有帮助，可以请我喝杯奶茶。"
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontBody
                font.family: Theme.fontFamily
                color: Theme.textPrimary
            }

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: root.qrSize
                height: root.qrSize
                radius: Theme.radiusSm
                color: "#FFFFFF"
                border.width: 1
                border.color: Theme.borderDefault
                clip: true

                Image {
                    id: qrImage
                    anchors.fill: parent
                    source: "qrc:/icons/res/donation/alipay.jpg"
                    sourceSize.width: 600
                    sourceSize.height: 600
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                }
            }

            Label {
                Layout.fillWidth: true
                visible: qrImage.status === Image.Error
                text: "捐赠二维码加载失败。"
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontBodySm
                font.family: Theme.fontFamily
                color: Theme.danger
            }

            Label {
                Layout.fillWidth: true
                text: "支付宝扫码 · 自愿支持，金额随意 · 不影响任何功能"
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontBodySm
                font.family: Theme.fontFamily
                color: Theme.textSecondary
            }
        }
    }
}
