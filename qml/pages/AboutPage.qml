import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Yuzora 1.0

// 关于语昼（推入式页面）
// 从设置页进入，不占侧栏导航项；左上角返回上一页。
// 关于页本身不联网，只有“检查更新”和 GitHub 链接会发起用户主动请求。
PageScaffold {
    id: root

    title: "关于语昼"
    leftIconSource: "qrc:/icons/res/arrow-narrow-left.svg"
    forceLeftButton: true
    onLeftClicked: Ui.requestBack()

    function pageShown() {
        resetScroll()
        // 页推入时第一个可聚焦控件（检查更新）可能触发 ScrollView 自动滚动；
        // 焦点/布局都稳定后再拉回顶部一次。
        Qt.callLater(resetScroll)
        scrollResetTimer.restart()
    }

    function resetScroll() {
        if (scroll.contentItem)
            scroll.contentItem.contentY = 0
    }

    Timer {
        id: scrollResetTimer
        interval: 100
        repeat: false
        onTriggered: root.resetScroll()
    }

    ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: Math.min(scroll.availableWidth, 1120)
            x: Math.max(0, (scroll.availableWidth - width) / 2)
            spacing: Theme.spaceLg

            // ========== 身份 ==========
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceMd

                Image {
                    Layout.preferredWidth: 72
                    Layout.preferredHeight: 72
                    Layout.alignment: Qt.AlignTop
                    source: "qrc:/icons/res/app/app_logo.png"
                    sourceSize.width: 144
                    sourceSize.height: 144
                    fillMode: Image.PreserveAspectFit
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceXs

                    Label {
                        text: "语昼"
                        font.pixelSize: Theme.fontDisplay
                        font.bold: true
                        font.family: Theme.fontFamily
                        color: Theme.textPrimary
                    }

                    Label {
                        text: "版本 " + AboutViewModel.currentVersion
                        font.pixelSize: Theme.fontBodySm
                        font.family: Theme.fontFamily
                        color: Theme.textSecondary
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "跨平台的 AI 角色聊天客户端"
                        wrapMode: Text.WordWrap
                        font.pixelSize: Theme.fontBodySm
                        font.family: Theme.fontFamily
                        color: Theme.textSecondary
                    }
                }
            }

            // ========== 更新 ==========
            SectionCard {
                Layout.fillWidth: true
                title: "更新"

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceSm

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Label {
                            text: AboutViewModel.statusText
                            font.pixelSize: Theme.fontBody
                            font.family: Theme.fontFamily
                            color: Theme.textPrimary
                        }

                        Label {
                            text: "通过 GitHub Releases 检查"
                            font.pixelSize: Theme.fontCaption
                            font.family: Theme.fontFamily
                            color: Theme.textSecondary
                        }
                    }

                    Item { Layout.fillWidth: true }

                    AppButton {
                        text: AboutViewModel.checking ? "检查中…" : "检查更新"
                        enabled: !AboutViewModel.checking
                        onClicked: AboutViewModel.checkForUpdates()
                    }
                }

                AppButton {
                    Layout.fillWidth: true
                    visible: AboutViewModel.showReleaseButton
                    text: AboutViewModel.hasUpdate ? "查看新版本" : "打开发布页"
                    variant: "secondary"
                    onClicked: Qt.openUrlExternally(AboutViewModel.releaseUrl)
                }
            }

            // ========== 链接与反馈 ==========
            AboutGroup {
                Layout.fillWidth: true

                AboutRow {
                    iconSource: "qrc:/icons/res/github.svg"
                    title: "GitHub"
                    subtitle: "github.com/zulsl/Yuzora"
                    onClicked: Qt.openUrlExternally("https://github.com/zulsl/Yuzora")
                }

                AboutRow {
                    iconSource: "qrc:/icons/res/edit.svg"
                    title: "建议反馈"
                    subtitle: "正式问题请优先提交 GitHub Issue，群聊用于交流。"
                    onClicked: Qt.openUrlExternally("https://github.com/zulsl/Yuzora/issues/new")
                }

                AboutRow {
                    iconSource: "qrc:/icons/res/message-circle.svg"
                    title: "交流群"
                    subtitle: AboutViewModel.groupNumber
                    trailing: "复制"
                    trailingColor: Theme.primaryColor
                    showChevron: false
                    onClicked: {
                        AboutViewModel.copyGroupNumber()
                        Ui.toast("群号已复制", false)
                    }
                }

                AboutRow {
                    iconSource: "qrc:/icons/res/brand-bilibili.svg"
                    title: "哔哩哔哩"
                    showDivider: false
                    onClicked: Qt.openUrlExternally("https://space.bilibili.com/261474153")
                }
            }


            // ========== 支持与信息 ==========
            AboutGroup {
                Layout.fillWidth: true

                AboutRow {
                    iconSource: "qrc:/icons/res/tip-jar.svg"
                    title: "打赏作者"
                    onClicked: donationDialog.open()
                }

                AboutRow {
                    iconSource: "qrc:/icons/res/spy.svg"
                    title: "隐私政策"
                    onClicked: privacyDialog.open()
                }

                AboutRow {
                    iconSource: "qrc:/icons/res/brand-tabler.svg"
                    title: "开源许可证"
                    subtitle: "GPL-3.0"
                    onClicked: Qt.openUrlExternally("https://github.com/zulsl/Yuzora/blob/main/LICENSE")
                }

            }

            // ========== 许可说明 ==========
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceXs

                Label {
                    Layout.fillWidth: true
                    text: "GNU General Public License v3.0"
                    wrapMode: Text.WordWrap
                    font.pixelSize: Theme.fontBodySm
                    font.family: Theme.fontFamily
                    color: Theme.textSecondary
                }

                Label {
                    Layout.fillWidth: true
                    text: "Qt 6.8.3（LGPLv3，动态链接）\nQt5Compat.GraphicalEffects（Qt 模块）"
                    wrapMode: Text.WordWrap
                    font.pixelSize: Theme.fontCaption
                    font.family: Theme.fontFamily
                    color: Theme.textSecondary
                }

                Label {
                    Layout.fillWidth: true
                    text: "SQLite（public domain）；Android 构建另含 OpenSSL（Apache 2.0）"
                    wrapMode: Text.WordWrap
                    font.pixelSize: Theme.fontCaption
                    font.family: Theme.fontFamily
                    color: Theme.textSecondary
                }
            }

            Label {
                Layout.fillWidth: true
                text: "© 2026 zulsl · 基于 Qt 6.8.3 构建"
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontCaption
                font.family: Theme.fontFamily
                color: Theme.textDisabled
            }

            Item { Layout.fillHeight: true }
        }
    }

    DonationDialog { id: donationDialog }
    PrivacyDialog { id: privacyDialog }
}
