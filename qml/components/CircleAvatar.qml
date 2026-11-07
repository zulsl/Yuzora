import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects

import Yuzora 1.0

// 圆形头像
// 职责：全项目唯一的头像渲染实现。
// 方案：Qt5Compat.GraphicalEffects.OpacityMask。
//       Image 只作为纹理源，圆形遮罩用白色圆角 Rectangle 生成 alpha。
//       Image 按 3x 解码，避免 SVG/高 DPI 下模糊。
Item {
    id: root

    property url source
    property int size: Theme.avatarMd
    property string fallbackText: ""
    property color placeholderColor: Theme.bgSurfaceVariant

    // 3x 解码，保证高 DPI 与 SVG 清晰度
    readonly property int renderScale: 3

    readonly property bool imageReady: avatarImage.status === Image.Ready

    implicitWidth: size
    implicitHeight: size

    // 占位底色（图片未加载 / 为空时可见）
    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: root.placeholderColor
        border.width: 1
        border.color: Theme.borderDefault

        Label {
            anchors.centerIn: parent
            visible: root.fallbackText.length > 0 && !root.imageReady
            text: root.fallbackText
            font.pixelSize: Math.round(root.size * 0.42)
            font.family: Theme.fontFamily
            color: Theme.textSecondary
        }
    }

    // 图片纹理源：不在场景里直接绘制
    Image {
        id: avatarImage
        anchors.fill: parent
        source: root.source
        fillMode: Image.PreserveAspectCrop
        smooth: true
        asynchronous: true
        sourceSize: Qt.size(root.size * root.renderScale,
                            root.size * root.renderScale)
        visible: false
    }

    // 圆形遮罩：白色圆角矩形，圆外 alpha 为 0
    Item {
        id: avatarMask
        anchors.fill: parent
        layer.enabled: true
        // 遮罩也按 3x 生成，避免圆形边缘被低分辨率遮罩放大后发虚
        layer.textureSize: Qt.size(root.size * root.renderScale,
                                   root.size * root.renderScale)
        layer.smooth: true
        visible: false

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: "white"
            antialiasing: true
        }
    }

    // 用遮罩输出圆形图片
    OpacityMask {
        anchors.fill: parent
        source: avatarImage
        maskSource: avatarMask
        visible: root.imageReady
    }
}
