ZHJG_001 智能井盖多版本工程

构建示例：
cmake -S projects/ZHJG_001 -B build/ZHJG_001_v1 -G Ninja -DAPP_VERSION=1 -DCMAKE_TOOLCHAIN_FILE=cmake/stm32-gcc-toolchain.cmake
cmake --build build/ZHJG_001_v1

版本范围：APP_VERSION=1..3
1: MQ-4沼气 + 水位 + MPU6050倾斜 + OLED + 声光报警 + 4键阈值设置
2: 版本1 + L76K GPS + A7670C短信报警（短信包含经纬度）
3: 版本1 + L76K GPS + ESP-01S WiFi MQTT + Flutter APP

默认 MQTT：
- Client ID: ZHJG_001
- 命令 Topic: ZHJG_001
- 遥测 Topic: ZHJG_001/web

上位机：
- Flutter 主上位机: ZHJG_001_upper_ui_flutter，覆盖 v1-v3 的 App/Web 主界面与版本化导出
- Debug 编译显示通信日志，Release 编译隐藏通信日志
- 不包含 mock/simulation 功能

导出示例：
python tools/export_project.py --project ZHJG_001 --batch --versions 1-3 -o ../exports --clean --extras readme.txt,PRODUCT_SPEC.md
