import QtQuick
import QtQuick.Layouts

// 自适应分栏（差异中档：LayoutItemProxy）
// 职责：宽窗口左右并排、窄窗口只显示主栏。子项始终是同一份对象实例，
//       靠 LayoutItemProxy 在两个布局位置之间"搬运"，不做 Loader 重建，
//       因此输入焦点、滚动位置、选中态在断点切换时都不会丢。
// 可配置属性：first（主栏）、second（次栏）、firstRatio / secondRatio（宽度权重）。
// 用法：
//     AdaptiveSplit {
//         first:  CharacterListPanel {}
//         second: CharacterDetailForm {}
//     }
Item {
    id: root

    property alias first: firstHost.data
    property alias second: secondHost.data

    property real firstRatio: 1
    property real secondRatio: 3
    property int spacing: Theme.gutter

    // 窄窗口只保留主栏，详情改由页面自行 push（MasterDetailStack）
    readonly property bool showSecond: width >= Theme.bpCompact

    readonly property real _total: Math.max(1, firstRatio + secondRatio)

    // 真实内容停放在这里（不参与渲染，只作为 LayoutItemProxy 的 target）
    Item { id: firstHost; visible: false; width: 0; height: 0 }
    Item { id: secondHost; visible: false; width: 0; height: 0 }

    // LayoutItemProxy 会接管目标项的位置与尺寸；如果直接绑定 children[0]，
    // 接管动作本身会改变 children，导致 "Binding loop detected for property target"。
    // 因此在创建完成后一次性捕获目标。
    property Item firstTarget: null
    property Item secondTarget: null

    Component.onCompleted: {
        if (firstHost.children.length > 0)
            firstTarget = firstHost.children[0]
        if (secondHost.children.length > 0)
            secondTarget = secondHost.children[0]
    }

    RowLayout {
        anchors.fill: parent
        spacing: root.showSecond ? root.spacing : 0

        LayoutItemProxy {
            id: firstProxy
            target: root.firstTarget
            visible: target !== null
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: root.showSecond
                ? Math.round(root.width * root.firstRatio / root._total)
                : root.width
        }

        LayoutItemProxy {
            id: secondProxy
            visible: root.showSecond && target !== null
            target: root.secondTarget
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: root.showSecond
                ? Math.round(root.width * root.secondRatio / root._total)
                : 0
        }
    }
}
