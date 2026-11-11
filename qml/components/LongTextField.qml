import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 长文本字段
//
// 解决什么问题：几万字的文本如果直接塞进 TextArea，每次切换角色都要整段重排版
// （实测 37 000 字约 330 ms，界面明显卡顿），而排版只能发生在 GUI 线程。
//
// 策略（阈值 Theme.metrics.longTextThreshold，默认 3000 字）：
//   - 文本长度 ≤ 阈值：直接显示可编辑 TextArea，行为与以前完全一致；
//   - 文本长度 > 阈值：只渲染**只读预览**（截断到 previewLength 字），
//     点"编辑全文"才创建 TextArea 承载全文 —— 排版成本推迟到用户真正要改的时候。
//
// 尺寸：空内容一行高（Theme.controlMd），随内容自然增长，
//       达到 maxLines 后封顶并在内部滚动。
//
// 回写：编辑时通过 edited(value) 交出最新文本，由使用方决定怎么存。
Item {
    id: root

    property string label: ""
    property string content: ""
    property string placeholder: ""
    // 最大自然文本行数；空内容时仍保持一行高
    property int maxLines: 8
    // 切换记录时用它判断"换人了"，避免编辑态残留
    // （不能用 content 判断：用户打字时 content 也在变）
    property string recordId: ""

    // 预览截断长度（只影响超长文本的预览，越小越省）
    property int previewLength: 400

    readonly property bool isLong: content.length > Theme.metrics.longTextThreshold
    // 当前是否显示全文编辑器：短文本始终可编辑；长文本要用户点"编辑全文"
    readonly property bool showEditor: editing || !isLong

    // 自然行高与高度上限
    readonly property real lineHeight: fontMetrics.height
    readonly property real minHeight: Theme.controlMd
    readonly property real maxHeight: Math.max(minHeight,
                         lineHeight * maxLines + area.topPadding + area.bottomPadding)

    // 用户编辑了文本
    signal edited(string value)

    property bool editing: false

    onRecordIdChanged: {
        editing = false
        if (typeof area !== "undefined" && area)
            area.focus = false
        syncEditorText(true)
    }

    // TextArea.text 不能直接绑定 content：
    // 输入会走 edited -> 父级草稿 -> content，再回写 text，形成 binding loop。
    // 编辑中只向上回写；非编辑态/切换记录时再由这里同步。
    function syncEditorText(force) {
        if (typeof area === "undefined" || !area)
            return

        if (!root.showEditor) {
            if (area.text.length > 0)
                area.text = ""
            return
        }

        if ((force || !area.activeFocus) && area.text !== root.content)
            area.text = root.content
    }

    onContentChanged: syncEditorText(false)
    onShowEditorChanged: syncEditorText(true)
    Component.onCompleted: syncEditorText(true)

    implicitWidth: column.implicitWidth
    implicitHeight: column.implicitHeight

    FontMetrics {
        id: fontMetrics
        font: area.font
    }

    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.spaceXs

        Label {
            visible: root.label.length > 0
            text: root.label
            font.pixelSize: Theme.fontBodySm
            font.family: Theme.fontFamily
            color: Theme.textSecondary
        }

        // ---------- 预览态（仅超长文本）----------
        Rectangle {
            visible: !root.showEditor
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(root.maxHeight,
                Math.max(root.minHeight,
                         previewText.contentHeight + Theme.spaceSm * 2))
            radius: Theme.radiusSm
            color: Theme.bgSurfaceVariant
            border.width: 1
            border.color: Theme.borderDefault
            clip: true

            Text {
                id: previewText
                anchors.fill: parent
                anchors.margins: Theme.spaceSm
                text: root.content.length > 0
                      ? root.content.substring(0, root.previewLength)
                      : root.placeholder
                color: root.content.length > 0 ? Theme.textPrimary : Theme.textDisabled
                font.pixelSize: Theme.fontBody
                font.family: Theme.fontFamily
                wrapMode: Text.Wrap
                elide: Text.ElideRight
                clip: true
            }
        }

        // ---------- 编辑态 ----------
        ScrollView {
            id: editorScroll
            visible: root.showEditor
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(root.maxHeight,
                Math.max(root.minHeight,
                         area.contentHeight + area.topPadding + area.bottomPadding))
            contentWidth: availableWidth
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            TextArea {
                id: area
                // 给内部滚动条留出空间，避免滚动条压住文字
                width: Math.max(0, editorScroll.availableWidth - Theme.spaceMd)
                placeholderText: root.placeholder
                wrapMode: Text.WordWrap
                textFormat: TextEdit.PlainText
                font.pixelSize: Theme.fontBody
                font.family: Theme.fontFamily
                color: Theme.textPrimary
                // 【关键】不在预览态时不要给这个 TextArea 文本：
                // 即使 visible:false，QQuickTextEdit 收到文本变更也会**同步**计算文档尺寸，
                // 等于把整段文本排版了一遍 —— 实测这一项单独就是 8ms/千字。
                // 文本由 syncEditorText() 单向同步，不能直接绑定 content。
                onTextChanged: if (activeFocus) root.edited(text)
            }
        }

        // ---------- 底部操作条 ----------
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm

            Label {
                text: root.content.length > 0 ? (root.content.length + " 字") : ""
                font.pixelSize: Theme.fontCaption
                font.family: Theme.fontFamily
                color: Theme.textDisabled
            }

            Item { Layout.fillWidth: true }

            // 超长文本：展开全文编辑
            AppButton {
                visible: root.isLong && !root.editing
                text: "编辑全文"
                variant: "ghost"
                onClicked: root.editing = true
            }

            // 超长文本：编辑完收起，回到轻量预览
            AppButton {
                visible: root.isLong && root.editing
                text: "收起"
                variant: "ghost"
                onClicked: root.editing = false
            }
        }
    }
}
