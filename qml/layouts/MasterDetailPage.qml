import QtQuick

import Yuzora 1.0

// 主从页面壳
// 职责：统一列表态/详情态的标题、返回图标和返回动作。
// 页面只维护 detailVisible，并实现 onBackRequested。
PageScaffold {
    id: root

    property bool detailVisible: false
    property string listTitle: ""
    property string detailTitle: ""

    signal backRequested()

    title: detailVisible ? (detailTitle.length > 0 ? detailTitle : listTitle)
                         : listTitle
    leftIconSource: detailVisible
                    ? "qrc:/icons/res/arrow-narrow-left.svg"
                    : "qrc:/icons/res/menu.svg"

    onLeftClicked: {
        if (detailVisible)
            root.backRequested()
        else
            Ui.requestDrawer()
    }
}
