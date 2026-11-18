import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Yuzora 1.0

// 关于页的分组列表
// 对应参考图里的浅灰圆角卡片：用户直接写 AboutRow 子项即可。
Rectangle {
    id: root

    default property alias content: column.data

    implicitHeight: column.implicitHeight
    color: Theme.bgSurfaceVariant
    radius: Theme.radiusMd
    clip: true

    Column {
        id: column
        width: parent.width
        spacing: 0
    }
}
