import QtQuick
import QtQuick.Controls


Item {
    id: root
    anchors.fill: parent

    signal maskClicked()

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: "#66000000"
        opacity: hoverArea.containsMouse ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 140 } }

        ToolButton {
            anchors.centerIn: parent
            icon.source:"qrc:/icons/res/camera.svg"
        }
    }

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.maskClicked()
    }
}

