import QtQuick
import QtQuick.Controls

// 轻提示（Toast）
// 职责：替代 Main.qml 里两个 ToolTip 实现，以及各页面重复的 errorToast/tipToast 调用。
// 用法：在页面或 Main 中放一个 AppToast，然后 toast.showToast("已保存", false)。
// 它自己负责居中、自动消失，并避开底部安全区。
Popup {
    id: root

    property string message: ""
    property bool isError: false
    readonly property int autoHideMs: 3000

    function showToast(msg, error) {
        if (msg === undefined || msg === null || msg === "") return
        message = String(msg)
        isError = (error === true)
        open()
        hideTimer.restart()
    }

    parent: Overlay.overlay
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round(parent.height - height - Theme.space2xl
                            - DeviceHelper.safeAreaBottom) : 0
    width: Math.min(label.implicitWidth + Theme.spaceLg * 2,
                    parent ? parent.width - Theme.spaceXl * 2 : 400)
    padding: 0
    modal: false
    focus: false
    closePolicy: Popup.NoAutoClose

    background: Rectangle {
        radius: Theme.radiusSm
        color: root.isError ? Theme.danger : Theme.bgElevated
        border.width: 1
        border.color: root.isError ? Theme.danger : Theme.borderDefault
        opacity: 0.97
    }

    contentItem: Label {
        id: label
        text: root.message
        font.pixelSize: Theme.fontBodySm
        font.family: Theme.fontFamily
        color: root.isError ? Theme.textOnAccent : Theme.textPrimary
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        topPadding: Theme.spaceSm
        bottomPadding: Theme.spaceSm
        leftPadding: Theme.spaceLg
        rightPadding: Theme.spaceLg
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1
                              duration: Theme.durationEnter; easing.type: Theme.easingOut }
            NumberAnimation { property: "scale"; from: 0.96; to: 1
                              duration: Theme.durationEnter; easing.type: Theme.easingOut }
        }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0
                          duration: Theme.durationExit; easing.type: Theme.easingIn }
    }

    Timer {
        id: hideTimer
        interval: root.autoHideMs
        repeat: false
        onTriggered: root.close()
    }
}
