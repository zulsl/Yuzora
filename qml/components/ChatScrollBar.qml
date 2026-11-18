import QtQuick

// 聊天列表的稳定滚动条。
//
// 为什么不用自动 ScrollBar：
// ListView 对不等高 delegate 的 contentHeight 是"已分配项平均高度"估算出来的，
// 长气泡进出缓存时它会来回跳（Qt 文档 "Variable Delegate Size and Section Labels"
// 一节明确说了这一点），ScrollBar 的 thumb 长度跟着 contentHeight 算，就会时长时短。
// 这里在滚动期间冻结总高度，停下来 250ms 后才同步；thumb 长度因此不会在滚动中跳动。
Item {
    id: root

    property Flickable flickable: null
    property color color: Theme.textSecondary

    readonly property int minThumbHeight: 32
    readonly property int edgeMargin: 4

    width: 10
    visible: flickable && flickable.contentHeight > flickable.height + 1

    // 滚动期间保持不变的总高度；停下来后由 idleTimer 同步
    property real stableContentHeight: 0

    readonly property real trackHeight: Math.max(0, height - edgeMargin * 2)
    readonly property real thumbHeight: {
        if (!flickable || stableContentHeight <= 0 || trackHeight <= 0)
            return trackHeight
        const ratio = flickable.height / stableContentHeight
        return Math.min(trackHeight, Math.max(minThumbHeight, trackHeight * ratio))
    }
    readonly property real travel: Math.max(0, trackHeight - thumbHeight)
    readonly property real progress: {
        if (!flickable)
            return 0
        // 贴顶/贴底直接用 Flickable 自己的判断，避免估算高度和实际差太多时 thumb 到不了头
        if (flickable.atYEnd)
            return 1
        if (flickable.atYBeginning)
            return 0
        const denominator = Math.max(1, stableContentHeight - flickable.height)
        return Math.max(0, Math.min(1, flickable.contentY / denominator))
    }

    onFlickableChanged: syncHeight()
    Component.onCompleted: syncHeight()

    function syncHeight() {
        // contentHeight 在 ListView 完成布局前可能是 -1，别拿它当总高度
        if (flickable && flickable.contentHeight > 0
                && !flickable.moving && !flickable.flicking)
            stableContentHeight = flickable.contentHeight
    }

    function scrollToThumbTop(y) {
        if (!flickable)
            return
        const p = travel <= 0 ? 0 : Math.max(0, Math.min(1, (y - edgeMargin) / travel))
        flickable.contentY = p * Math.max(0, flickable.contentHeight - flickable.height)
    }

    // 滚动中只重启计时器，不做同步；停稳后再取一次 contentHeight
    Connections {
        target: root.flickable
        function onContentHeightChanged() { idleTimer.restart() }
        function onContentYChanged() { idleTimer.restart() }
        function onMovementEnded() { idleTimer.restart() }
        function onFlickEnded() { idleTimer.restart() }
    }

    Timer {
        id: idleTimer
        interval: 250
        repeat: false
        onTriggered: root.syncHeight()
    }

    Behavior on stableContentHeight {
        NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
    }

    Rectangle {
        id: thumb
        width: root.width
        height: root.thumbHeight
        y: root.edgeMargin + root.progress * root.travel
        radius: width / 2
        color: root.color
        opacity: mouse.containsMouse || mouse.pressed ? 0.75 : 0.35
        Behavior on opacity { NumberAnimation { duration: 120 } }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.PointingHandCursor

        property real grabOffset: 0

        onPressed: (mouse) => {
            // 点在 thumb 上就拖，点在轨道上先跳过去再拖
            if (mouse.y >= thumb.y && mouse.y <= thumb.y + thumb.height) {
                grabOffset = mouse.y - thumb.y
            } else {
                grabOffset = thumb.height / 2
                root.scrollToThumbTop(mouse.y)
            }
        }
        onPositionChanged: (mouse) => {
            if (pressed)
                root.scrollToThumbTop(mouse.y - grabOffset)
        }
        onWheel: (wheel) => {
            if (root.flickable)
                root.flickable.contentY -= wheel.angleDelta.y / 120 * 60
            wheel.accepted = true
        }
    }
}
