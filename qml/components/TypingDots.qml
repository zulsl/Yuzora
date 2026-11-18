import QtQuick

// "正在生成回复"的三点指示器。
// 只负责画和动：什么时候显示、延迟多久由 MessageBubble 决定，
// 这样动画不用理解消息状态，也方便以后别处复用。
Item {
    id: root

    property bool running: true
    property color color: Theme.textSecondary

    // 尺寸都跟着字号走，改基础字号时不需要再调这里
    property real dotSize: Math.max(4, Math.round(Theme.fontBody * 0.38))
    property real dotSpacing: Math.max(3, Math.round(dotSize * 0.66))
    property real bounce: Math.max(2, Math.round(dotSize * 0.5))

    // 一轮 1.2s：0.3s 上浮 + 0.3s 回落 + 0.6s 停，三个点错开 0.15s
    readonly property int cycleMs: 1200
    readonly property int stepMs: 300
    readonly property int staggerMs: 150

    implicitWidth: dotSize * 3 + dotSpacing * 2
    implicitHeight: dotSize + bounce

    Accessible.role: Accessible.StaticText
    Accessible.name: qsTr("正在生成回复")

    Row {
        anchors.centerIn: parent
        spacing: root.dotSpacing

        Repeater {
            model: 3

            delegate: Rectangle {
                id: dot
                width: root.dotSize
                height: root.dotSize
                radius: width / 2
                color: root.color
                opacity: 0.35

                // 一轮的总时长对每个点都一样，只是起点错开：
                // 这样循环不会让三个点越跑越散（如果只在开头加 Pause，相位会漂移）。
                SequentialAnimation {
                    // 委托被复用/页面切走时停掉，避免看不见还在耗帧
                    running: root.running && root.visible
                    loops: Animation.Infinite

                    PauseAnimation { duration: index * root.staggerMs }

                    ParallelAnimation {
                        NumberAnimation {
                            target: dot; property: "y"
                            from: 0; to: -root.bounce
                            duration: root.stepMs
                            easing.type: Easing.InOutSine
                        }
                        NumberAnimation {
                            target: dot; property: "opacity"
                            from: 0.35; to: 1.0
                            duration: root.stepMs
                            easing.type: Easing.InOutSine
                        }
                    }

                    ParallelAnimation {
                        NumberAnimation {
                            target: dot; property: "y"
                            from: -root.bounce; to: 0
                            duration: root.stepMs
                            easing.type: Easing.InOutSine
                        }
                        NumberAnimation {
                            target: dot; property: "opacity"
                            from: 1.0; to: 0.35
                            duration: root.stepMs
                            easing.type: Easing.InOutSine
                        }
                    }

                    PauseAnimation {
                        duration: root.cycleMs - index * root.staggerMs - root.stepMs * 2
                    }
                }
            }
        }
    }
}
