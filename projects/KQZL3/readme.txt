KQZL3 空气质量检测系统 (版本 1-14)

支持功能：
- PM2.5、温湿度、烟雾、CO检测
- 自动/手动/阈值设置三种工作模式
- 风扇、蜂鸣器、灯光控制
- WiFi/蓝牙远程控制 (部分版本)

构建指定版本 (1-14)：
cmake -S . -B build -G "Ninja" -DAPP_VERSION=5 -DCMAKE_TOOLCHAIN_FILE=D:/ProjectJBL/PerfabProj/project_template/cmake/stm32-gcc-toolchain.cmake -DCMAKE_MAKE_PROGRAM=D:/ProgramFile/ST/STM32CubeCLT_1.20.0/Ninja/bin/ninja.exe
cmake --build build

批量导出所有版本：
python ../../tools/export_project.py --project KQZL3 --batch --versions 1-14 -o ../../exports --clean --extras readme.txt,PRODUCT_SPEC.md

引脚连接：
- OLED: PB6(SCL), PB7(SDA)
- DHT11: PA5
- PM2.5 ADC: PA0
- PM2.5 LED: PA12 (低电平点亮)
- MQ-2 ADC: PA1 (版本5+)
- MQ-7 ADC: PA4 (版本11+)
- 风扇继电器: PA11
- 蜂鸣器: PA8
- LED灯光: PA6
- 按键: PB4(K1), PB5(K2), PB13(K3), PB14(K4)
- ESP8266: PB10(TX), PB11(RX), PB0(PD), PB1(RST)

上位机：
- Flutter 主上位机: KQZL3_upper_ui_flutter，覆盖 v3/v4/v6/v7/v8/v10/v12/v13/v14
- uni-app 保留工程: KQZL3_upper_ui，仅用于 v9 微信小程序

Flutter 构建示例：
cd KQZL3_upper_ui_flutter
flutter pub get
flutter build apk --debug --dart-define=UPPER_VERSION=14
flutter build web --dart-define=UPPER_VERSION=10

v9 微信小程序构建：
cd KQZL3_upper_ui
npm run build:mp-weixin:v9

详见 PRODUCT_SPEC.md
