import QtQuick

// 主从栈（差异大的档位：Loader）
// 职责：窄窗口下"列表 → 推入详情"的页面栈行为，替代原先 CharacterPage 里
//       手写两份 Component + theme.isMobile 判断的写法。
// 可配置属性：listComponent（列表）、detailComponent（详情）、detailShown。
// 用法：
//     MasterDetailStack {
//         id: stack
//         listComponent: listPage
//         detailComponent: detailPage
//     }
//     stack.showDetail()
Item {
    id: root

    property Component listComponent: null
    property Component detailComponent: null
    property bool detailShown: false
    // 详情页标题，供详情组件内部读取
    property string detailTitle: ""

    signal detailClosed()

    function showDetail() { root.detailShown = true }
    function hideDetail() {
        if (!root.detailShown) return
        root.detailShown = false
        root.detailClosed()
    }

    Loader {
        id: listLoader
        anchors.fill: parent
        sourceComponent: root.listComponent
        // 详情展开时保留列表实例，返回时滚动位置与输入状态都还在
        visible: !root.detailShown
        enabled: !root.detailShown
    }

    Loader {
        id: detailLoader
        anchors.fill: parent
        active: root.detailShown
        sourceComponent: root.detailComponent
        opacity: root.detailShown ? 1 : 0
        visible: opacity > 0
        x: root.detailShown ? 0 : root.width * 0.12

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.durationState
                easing.type: Theme.easingInOut
            }
        }
        Behavior on x {
            NumberAnimation {
                duration: Theme.durationState
                easing.type: Theme.easingInOut
            }
        }
    }
}
