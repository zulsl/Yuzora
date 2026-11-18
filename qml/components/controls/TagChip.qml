import QtQuick
import QtQuick.Controls

// 状态角标
// 职责：统一的"标签/状态"小胶囊，替代各页面各自拼 Label + Rectangle 的写法。
// 可配置属性：text、tone（success / warning / danger / neutral）。
Rectangle {
    id: root

    property string text: ""
    property string tone: "success"

    readonly property color _fg: tone === "success" ? Theme.success
                                : tone === "warning" ? Theme.warning
                                : tone === "danger" ? Theme.danger
                                : Theme.textSecondary
    readonly property color _bg: tone === "success" ? Theme.successSubtle
                                : tone === "warning" ? Theme.bgSurfaceVariant
                                : tone === "danger" ? Theme.dangerSubtle
                                : Theme.bgSurfaceVariant

    implicitWidth: label.implicitWidth + Theme.spaceSm
    implicitHeight: label.implicitHeight + Theme.spaceXs
    radius: Theme.radiusSm
    color: _bg

    Label {
        id: label
        anchors.centerIn: parent
        text: root.text
        font.pixelSize: Theme.fontCaption
        font.family: Theme.fontFamily
        color: root._fg
    }
}
