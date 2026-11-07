import QtQuick
import QtQuick.Layouts

import Yuzora 1.0

// 页面骨架
// 职责：统一"顶栏 + 内容区"，并处理安全区。
// 键盘避让说明：Android 用 android:windowSoftInputMode="adjustResize" 让窗口自身收缩，
//   因此这里默认不做任何键盘上抬（keyboardAware=false），否则会双重位移；
//   底部安全区默认也不额外留白（respectBottomSafeArea=false），因为窗口已让开系统栏。
//   需要手动控制时，页面可把这两个开关打开，或直接读取 keyboardInset 自行布局。
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
    // 是否由页面骨架统一吸收软键盘高度（整块内容上抬）。
    // Android 已用 windowSoftInputMode=adjustResize 让窗口自身收缩，
    // 这里默认关闭，否则会出现"窗口已变矮 + QML 再抬一次"的双重位移。
    property bool keyboardAware: false
    // 是否额外给底部留出系统安全区。
    // 默认关闭：adjustResize 下窗口本身已经让开了导航栏/键盘，再加一份就是多余空白。
    property bool respectBottomSafeArea: false
    property alias rightItems: toolbar.rightItems

    signal leftClicked()

    // 软键盘高度：键盘收起时 keyboardRectangle 为空（高度 0）。
    // 注意 Android 上该值不一定可靠（窗口收缩后坐标系变化），因此只作参考，
    // 默认不参与布局；桌面上可用 DeviceHelper.testKeyboardInset 模拟验证。
    readonly property int keyboardInset: Math.max(Qt.inputMethod.keyboardRectangle.height,
                                                  DeviceHelper.testKeyboardInset)

    readonly property int bottomInset: (respectBottomSafeArea ? DeviceHelper.safeAreaBottom : 0)
                                       + (keyboardAware ? keyboardInset : 0)

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
            // 宽窗口下左侧菜单常驻，不再需要汉堡按钮；窄窗口才显示
            showLeftButton: root.showLeftButton && !Theme.isExpanded
            onLeftClicked: root.leftClicked()
        }

        // 内容外框：页面内边距
        Item {
            id: bodyFrame
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: root.contentPadding
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
