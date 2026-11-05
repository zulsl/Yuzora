pragma Singleton
import QtQuick

// 性能探针（供 QML 使用）
//
// 用途：给页面切换、详情构建等关键路径计时，便于长期观察性能回归。
//
// 开启方式（二选一）：
//   1) 环境变量 YUZORA_PERF=1
//      Qt Creator：项目 → 运行 → Environment 里新增该变量
//   2) 启动参数 --perf
// 关闭时所有函数立即返回，零开销、零日志。
//
// 结果去哪看：
//   - Qt Creator 的「应用程序输出」面板
//   - 应用数据目录下的 perf.log（可直接用记事本打开，UTF-8）
//   日志位置可用 Perf.logPath() / DeviceHelper.perfLogPath() 查到。
//
// 用法：
//   Perf.startPage("角色管理")   → 记一次页面切换
//   mainStack.replace(pageComponent)
//   Perf.buildDone()             → 记录同步构建耗时（首帧由内部自动补上）
//
//   Perf.begin("读取角色"); ... ; Perf.end("读取角色")   → 任意区间
//   Perf.report()                                        → 打印汇总
QtObject {
    id: root

    readonly property bool enabled: DeviceHelper.perfEnabled

    property string _pageName: ""
    property real _pageStart: 0
    property bool _sessionStarted: false

    // 每个名字的起始时间
    property var _spans: ({})
    // 名字 -> { count, total, max }
    property var _stats: ({})

    // ================= 页面切换 =================
    function startPage(name) {
        if (!enabled) return
        _ensureSession()
        _pageName = name
        _pageStart = Date.now()
        frameProbe.running = false
        frameProbe.running = true
    }

    // 页面组件同步构建完成（replace() 返回）时调用
    function buildDone() {
        if (!enabled || _pageName === "") return
        const ms = Date.now() - _pageStart
        _record(_pageName + "（构建）", ms)
        _emit(_pageName + "（构建） " + ms + " ms")
    }

    // ================= 任意区间 =================
    function begin(name) {
        if (!enabled) return
        _ensureSession()
        _spans[name] = Date.now()
    }

    function end(name) {
        if (!enabled) return
        const t0 = _spans[name]
        if (t0 === undefined) return
        delete _spans[name]

        const ms = Date.now() - t0
        _record(name, ms)
        _emit(name + "  " + ms + " ms")
    }

    // ================= 汇总 =================
    function report() {
        if (!enabled) return

        const names = Object.keys(_stats)
        if (names.length === 0) {
            _emit("没有记录到任何耗时")
            return
        }

        _emit("===== 性能汇总 =====")
        names.sort().forEach(function (name) {
            const s = _stats[name]
            _emit(name
                  + "  次数 " + s.count
                  + "  平均 " + (s.total / s.count).toFixed(1) + " ms"
                  + "  最大 " + s.max + " ms")
        })
    }

    function reset() {
        _spans = ({})
        _stats = ({})
    }

    // 直接记录一个已经算好的耗时（外部自己掐表时用）
    function record(name, ms) {
        if (!enabled) return
        _record(name, ms)
    }

    // 从"现在"到"下一帧渲染完成"的耗时：用于测量"数据变了 → 界面真的画出来"的延迟
    function beginFrame(name) {
        if (!enabled) return
        _frameName = name
        _frameStart = Date.now()
        frameProbe2.running = false
        frameProbe2.running = true
    }

    property string _frameName: ""
    property real _frameStart: 0

    property FrameAnimation frameProbe2: FrameAnimation {
        running: false
        onTriggered: {
            running = false
            if (!root.enabled || root._frameName === "") return
            const ms = Date.now() - root._frameStart
            root._record(root._frameName + "（到下一帧）", ms)
            root._emit(root._frameName + "（到下一帧） " + ms + " ms")
            root._frameName = ""
        }
    }

    // 日志文件路径（供界面展示，例如"日志已写入 xxx"）
    function logPath() {
        return DeviceHelper.perfLogPath()
    }

    // ================= 内部 =================
    function _ensureSession() {
        if (_sessionStarted) return
        _sessionStarted = true
        _emit("===== 新会话 " + new Date().toLocaleString() + " =====")
    }

    function _emit(line) {
        console.log("[perf] " + line)
        // 同时落盘，方便不开 Qt Creator 时查看
        DeviceHelper.perfLog(line)
    }

    function _record(name, ms) {
        const s = _stats[name]
        if (s === undefined) {
            _stats[name] = { count: 1, total: ms, max: ms }
        } else {
            s.count += 1
            s.total += ms
            if (ms > s.max) s.max = ms
        }
    }

    // 首帧探针：页面 replace 之后，等到下一帧渲染完成才算"首帧"
    property FrameAnimation frameProbe: FrameAnimation {
        running: false
        onTriggered: {
            running = false
            if (!root.enabled || root._pageName === "") return
            const ms = Date.now() - root._pageStart
            root._record(root._pageName + "（首帧）", ms)
            root._emit(root._pageName + "（首帧） " + ms + " ms")
            root._pageName = ""
        }
    }
}
