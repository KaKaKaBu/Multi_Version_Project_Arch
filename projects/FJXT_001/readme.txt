FJXT_001 车窗防夹系统

版本:
1: 基础版，防夹检测、电机控制、到位、一键/点动开关、OLED、声光提醒、按键。
2: 版本1 + 蓝牙 APP。
3: 版本1 + WiFi APP。
4: 版本3 + ESP-CAM 地址与视频流链接上报。
5: 版本1 + 云平台 APP。
6: 版本1 + 语音播报。
7: 版本6 + 蓝牙 APP。
8: 版本6 + WiFi APP。
9: 版本8 + ESP-CAM 地址与视频流链接上报。
10: 版本6 + 云平台 APP。

构建示例:
cmake -S projects/FJXT_001 -B build/FJXT_001_v1 -G Ninja -DAPP_VERSION=1
cmake --build build/FJXT_001_v1 -j1

远程控制命令:
{"cmd":"open"}
{"cmd":"close"}
{"cmd":"open_step"}
{"cmd":"close_step"}
{"cmd":"stop"}
{"cmd":"get_status"}

CAM 版本预留:
{"cmd":"camera_info","params":{"ip":"192.168.1.88","stream":"http://192.168.1.88:81/stream"}}
