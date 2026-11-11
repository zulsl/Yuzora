import QtQuick

// 顶层页面宿主
// 职责：首次访问时懒加载页面，之后保活；切换只改 visible，并通知页面
// pageShown()/pageHidden() 生命周期。Main 不再自己维护 pageLoaded 数组。
Item {
    id: root

    property var pageComponents: []
    property int currentIndex: 0

    signal pageShown(int index)
    signal pageHidden(int index)

    // 只增不减：页面一旦创建就保活，避免切回来重建整份表单。
    property var loadedPages: []

    readonly property int count: pageComponents ? pageComponents.length : 0

    function ensureLoaded(index) {
        if (index < 0 || index >= count)
            return
        if (loadedPages[index] === true)
            return

        const next = loadedPages.slice()
        next[index] = true
        loadedPages = next
    }

    function itemAt(index) {
        if (index < 0 || index >= repeater.count)
            return null
        const loader = repeater.itemAt(index)
        return loader ? loader.item : null
    }

    function notifyShown(index) {
        if (index < 0 || _shownIndex === index)
            return
        _shownIndex = index
        const item = itemAt(index)
        if (item && typeof item.pageShown === "function")
            item.pageShown()
        pageShown(index)
    }

    function notifyHidden(index) {
        if (_shownIndex === index)
            _shownIndex = -1
        const item = itemAt(index)
        if (item && typeof item.pageHidden === "function")
            item.pageHidden()
        pageHidden(index)
    }

    property int _lastIndex: -1
    property int _shownIndex: -1

    onCurrentIndexChanged: {
        if (_lastIndex >= 0 && _lastIndex !== currentIndex)
            notifyHidden(_lastIndex)

        ensureLoaded(currentIndex)
        _lastIndex = currentIndex

        // Loader 可能在本帧稍后才创建 item；onLoaded 里会补一次 pageShown。
        Qt.callLater(function() {
            if (root.currentIndex === root._lastIndex)
                root.notifyShown(root.currentIndex)
        })
    }

    onPageComponentsChanged: {
        if (count > 0) {
            ensureLoaded(currentIndex)
            _lastIndex = currentIndex
            Qt.callLater(function() {
                root.notifyShown(root.currentIndex)
            })
        }
    }

    Repeater {
        id: repeater
        model: root.pageComponents

        delegate: Loader {
            id: pageLoader
            required property int index
            required property var modelData

            anchors.fill: parent
            active: root.loadedPages[index] === true
            visible: index === root.currentIndex
            sourceComponent: pageLoader.modelData
        }
    }
}
