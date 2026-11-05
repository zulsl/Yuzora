import QtQuick
import QtQuick.Layouts

// 可展开行
// 职责：统一的"标题行 + 按需加载的详情区"，带高度过渡动画。
// 原先 BookEntryListView 与 PromptsListView 各自复制了一整套（约 190 行）。
// 可配置属性：
//   expanded      —— 是否展开（通常绑定 model.expandedState）
//   bodyComponent —— 详情区组件（收起时不创建）
//   bodyData      —— 传给详情区根元素的上下文数据（通常是 model）
// 标题行内容写在花括号里。
Item {
    id: root

    property bool expanded: false
    property Component bodyComponent: null
    property var bodyData: null
    property int headerHeight: Theme.controlLg

    default property alias headerItems: headerRow.data

    implicitHeight: headerHeight + (root.expanded ? loader.implicitHeight : 0)
    clip: true

    Behavior on implicitHeight {
        NumberAnimation {
            duration: Theme.durationState
            easing.type: Theme.easingInOut
        }
    }

    RowLayout {
        id: headerRow
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: root.headerHeight
        spacing: Theme.spaceSm
    }

    // 收起时不创建详情，避免无谓的绑定与渲染
    Loader {
        id: loader
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: headerRow.bottom
        active: root.expanded && root.bodyComponent !== null
        sourceComponent: root.bodyComponent
        visible: root.expanded
        onLoaded: {
            if (item && root.bodyData !== null)
                item.bodyData = root.bodyData
        }
    }
}
