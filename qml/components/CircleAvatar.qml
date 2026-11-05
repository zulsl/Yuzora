import QtQuick
import QtQuick.Controls

// 圆形头像
// 职责：全项目唯一的头像渲染实现（原先存在 OpacityMask 与 MultiEffect 两套写法）。
// 用 Rectangle 的 radius + clip 直接裁剪，避免 Qt5Compat.GraphicalEffects 依赖，
// 也避免在列表中为每行开一个实时 MultiEffect。
// 可配置属性：source（图片 url）、size、fallbackText（无图时显示的首字）。
Item {
    id: root

    property url source
    property int size: Theme.avatarMd
    property string fallbackText: ""
    property color placeholderColor: Theme.bgSurfaceVariant

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
            visible: root.fallbackText.length > 0
            text: root.fallbackText
            font.pixelSize: Math.round(root.size * 0.42)
            font.family: Theme.fontFamily
            color: Theme.textSecondary
        }
    }

    // 图片本体：radius 取一半 + clip 即为正圆裁剪
    Rectangle {
        anchors.fill: parent
        radius: width / 2
        clip: true
        color: "transparent"

        Image {
            anchors.fill: parent
            source: root.source
            fillMode: Image.PreserveAspectCrop
            smooth: true
            mipmap: false                 // 小尺寸无需多级纹理
            asynchronous: true
            sourceSize: Qt.size(root.size * 2, root.size * 2)   // 高分屏不糊
        }
    }
}
