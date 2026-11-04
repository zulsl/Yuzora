// import QtQuick
// import QtQuick.Controls
// import QtQuick.Layouts
// import com.example.device 1.0

// ApplicationWindow {
//     visible: true
//     width: 400
//     height: 600
//     title: "DeviceHelper 测试"

//     // 用于收集测试日志
//     property string logText: ""

//     Component.onCompleted: {
//         runAllTests()
//     }

//     function runAllTests() {
//         appendLog("=== 开始测试 DeviceHelper ===")
//         appendLog("设备类型: " + deviceTypeString())
//         appendLog("isMobile: " + DeviceHelper.isMobile)
//         appendLog("isPC: " + DeviceHelper.isPC)
//         appendLog("isTablet: " + DeviceHelper.isTablet)
//         appendLog("屏幕密度: " + DeviceHelper.screenDensity)
//         appendLog("屏幕尺寸: " + DeviceHelper.screenSize.width + " x " + DeviceHelper.screenSize.height)
//         appendLog("dpToPx(10): " + DeviceHelper.dpToPx(10))
//         appendLog("pxToDp(20): " + DeviceHelper.pxToDp(20))
//         appendLog("缩放字体大小(基础12): " + DeviceHelper.getScaledFontSize(12))
//         appendLog("应用数据路径: " + DeviceHelper.getAppDataPath())
//         appendLog("外部存储路径: " + DeviceHelper.getExternalStoragePath())
//         appendLog("检查相机权限: " + DeviceHelper.checkPermission("android.permission.CAMERA"))
//         appendLog("检查图片权限: " + DeviceHelper.checkPermission("android.permission.READ_MEDIA_IMAGES"))
//         appendLog("检查存储权限: " + DeviceHelper.checkPermission("android.permission.READ_EXTERNAL_STORAGE"))
//         appendLog("=== 测试完成 ===")
//     }

//     function deviceTypeString() {
//         // 注意枚举在QML中变为int，需要手动转换
//         var t = DeviceHelper.deviceType
//         switch(t) {
//             case 0: return "Windows"
//             case 1: return "macOS"
//             case 2: return "Linux"
//             case 3: return "Android"
//             case 4: return "iOS"
//             default: return "Unknown"
//         }
//     }

//     function appendLog(msg) {
//         logText += msg + "\n"
//         console.log(msg) // 控制台中文输出
//     }

//     // 界面布局
//     ColumnLayout {
//         anchors.fill: parent

//         Label {
//             text: "DeviceHelper 测试工具"
//             font.bold: true
//             font.pixelSize: 18
//             Layout.alignment: Qt.AlignHCenter
//         }

//         ScrollView {
//             Layout.fillWidth: true
//             Layout.fillHeight: true
//             TextArea {
//                 id: logView
//                 readOnly: true
//                 text: logText
//                 wrapMode: TextArea.Wrap
//                 font.pixelSize: 14
//             }
//         }

//         RowLayout {
//             Layout.fillWidth: true
//             Button {
//                 text: "重新运行测试"
//                 onClicked: {
//                     logText = ""
//                     runAllTests()
//                 }
//             }
//             Button {
//                 text: "请求存储权限"
//                 onClicked: {
//                     DeviceHelper.requestPermission("android.permission.READ_EXTERNAL_STORAGE")
//                 }
//             }
//             Button {
//                 text: "请求相机权限"
//                 onClicked: {
//                     DeviceHelper.requestPermission("android.permission.CAMERA")
//                 }
//             }
//         }

//         // 权限结果连接
//         Connections {
//             target: DeviceHelper
//             function onPermissionGranted(perm) {
//                 appendLog("权限已授予: " + perm)
//             }
//             function onPermissionDenied(perm) {
//                 appendLog("权限被拒绝: " + perm)
//             }
//         }
//     }
// }
