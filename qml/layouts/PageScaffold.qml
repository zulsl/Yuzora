import QtQuick
import QtQuick.Layouts

// 页面骨架
// 职责：统一"顶栏 + 内容区"，并集中处理三件移动端专项：
//   1. 安全区上下左右内边距由 AppToolBar / 本组件吸收，页面不用各自计算；
//   2. 软键盘弹出时把内容区整体上抬（bottomInset），聊天输入框、密钥输入框、预设文本框
//      都因此不会被遮挡；
//   3. 内容内边距走 Theme.pagePadding，窄窗口自动收窄一档。
// 可配置属性：title、leftIconSource、showLeftButton、showToolBar、contentPadding、
//             keyboardAware、rightItems。
// 页面用法：PageScaffold { title: "设置"; onLeftClicked: Ui.requestDrawer(); ... }
Item {
    id: root

    default property alias content: body.data

    property string title: ""
    property string subtitle: ""
    property string leftIconSource: "qrc:/icons/res/menu.svg"
    property bool showLeftButton: true
    property bool showToolBar: true
    property int contentPadding: Theme.pagePadding
    // 需要键盘避让的页面保持 true；纯展示页可关掉
    property bool keyboardAware: true
    property alias rightItems: toolbar.rightItems

    signal leftClicked()

    // 软键盘高度：键盘收起时 keyboardRectangle 为空（高度 0），因此不需要额外判断可见性。
    // 桌面上可用 DeviceHelper.testKeyboardInset 模拟键盘，便于验证遮挡。
    readonly property int keyboardInset: keyboardAware
        ? Math.max(Qt.inputMethod.keyboardRectangle.height, DeviceHelper.testKeyboardInset)
        : DeviceHelper.testKeyboardInset

    readonly property int bottomInset: Math.max(DeviceHelper.safeAreaBottom, keyboardInset)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        AppToolBar {
            id: toolbar
            visible: root.showToolBar
            Layout.fillWidth: true
            title: root.title
            subtitle: root.subtitle
            leftIconSource: root.leftIconSource
            showLeftButton: root.showLeftButton
            onLeftClicked: root.leftClicked()
        }

        // 内容外框：左右安全区 + 页面内边距 + 底部安全区/键盘
        Item {
            id: bodyFrame
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: DeviceHelper.safeAreaLeft + root.contentPadding
            Layout.rightMargin: DeviceHelper.safeAreaRight + root.contentPadding
            Layout.bottomMargin: root.bottomInset

            Item {
                id: body
                anchors.fill: parent
            }
        }
    }
}
