import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Yuzora 1.0

// 关于页的单行
// 图标 + 标题/副标题 + 尾部文本 + 右侧 chevron；整行可点击。
Item {
    id: root

    property string iconSource: ""
    property string title: ""
    property string subtitle: ""
    property string trailing: ""
    property bool showChevron: true
    property bool showDivider: true
    property color trailingColor: Theme.textSecondary
    signal clicked()

    width: parent ? parent.width : 0
    implicitHeight: subtitle.length > 0
                    ? Theme.touchTarget + Theme.spaceMd
                    : Theme.touchTarget + Theme.spaceSm

    Rectangle {
        id: background
        anchors.fill: parent
        // 留出圆角内边距：行悬停色不能盖住 AboutGroup 的圆角，
        // 否则鼠标移上去时整组看起来会从圆角变成方角。
        anchors.margins: Theme.spaceXs
        radius: Theme.radiusSm
        color: mouseArea.containsMouse ? Theme.rowHover : "transparent"
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spaceMd
        anchors.rightMargin: Theme.spaceMd
        spacing: Theme.spaceSm

        Item {
            Layout.preferredWidth: Theme.iconMd
            Layout.preferredHeight: Theme.iconMd
            Layout.alignment: Qt.AlignVCenter

            Image {
                id: sourceIcon
                anchors.fill: parent
                source: root.iconSource
                sourceSize.width: Theme.iconMd * 2
                sourceSize.height: Theme.iconMd * 2
                visible: false
            }

            ColorOverlay {
                anchors.fill: parent
                source: sourceIcon
                color: Theme.textSecondary
                visible: root.iconSource.length > 0
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Label {
                Layout.fillWidth: true
                text: root.title
                elide: Text.ElideRight
                font.pixelSize: Theme.fontBody
                font.family: Theme.fontFamily
                color: Theme.textPrimary
            }

            Label {
                Layout.fillWidth: true
                visible: root.subtitle.length > 0
                text: root.subtitle
                elide: Text.ElideRight
                font.pixelSize: Theme.fontCaption
                font.family: Theme.fontFamily
                color: Theme.textSecondary
            }
        }

        Label {
            visible: root.trailing.length > 0
            text: root.trailing
            elide: Text.ElideRight
            font.pixelSize: Theme.fontBodySm
            font.family: Theme.fontFamily
            color: root.trailingColor
        }

        Label {
            visible: root.showChevron
            text: "›"
            font.pixelSize: Math.round(Theme.fontBody * 1.5)
            font.family: Theme.fontFamily
            color: Theme.textDisabled
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: Theme.spaceMd + Theme.iconMd + Theme.spaceSm
        height: 1
        color: Theme.borderDefault
        visible: root.showDivider
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
