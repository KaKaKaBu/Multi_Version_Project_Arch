DCLD_001 倒车雷达多版本工程

构建示例：
cmake -S projects/DCLD_001 -B build/DCLD_001_v1 -G Ninja -DAPP_VERSION=1 -DCMAKE_TOOLCHAIN_FILE=cmake/stm32-gcc-toolchain.cmake
cmake --build build/DCLD_001_v1

版本范围：APP_VERSION=1..7
1: 超声波测距 + OLED + 声光报警 + 阈值设置
2: 版本1 + DS18B20温度补偿
3: 版本2 + SU03T语音播报
4: 版本2 + ESP-01S WiFi MQTT + APP
5: 版本2 + JDY-31蓝牙 + APP
6: 版本3 + ESP-01S WiFi MQTT + APP
7: 版本6 + ESP32-CAM视频流URL APP展示

上位机：
- Flutter 主上位机: DCLD_001_upper_ui_flutter，覆盖 v1-v7 的 App/Web 主界面与版本化导出
- uni-app 历史工程: DCLD_001_upper_ui，仅作为迁移对照和回退参考保留

Flutter 构建示例：
cd DCLD_001_upper_ui_flutter
flutter pub get
flutter analyze
flutter test
flutter build apk --debug --dart-define=UPPER_VERSION=7 --dart-define=UPPER_FEATURES=common,temp,voice,remote,wifi,app,camera

导出示例：
python tools/export_project.py --project DCLD_001 --batch --versions 1-7 -o ../exports --clean --extras readme.txt,PRODUCT_SPEC.md
