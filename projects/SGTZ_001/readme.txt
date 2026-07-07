SGTZ_001 51 电子秤项目

默认版本：APP_VERSION=7，身高体重 BMI + OLED + 提示。

当前 C51 固件采用本地轮询架构：
- 芯片目标 STC89C52，默认 11.0592MHz 晶振。
- HX711 称重，上电空载去皮。
- HCSR04 超声波测高，按 200cm 顶部安装高度换算身高。
- OLED 显示体重、身高、BMI 和状态。
- K1 锁定/计算，K2 重测，K3 暂停/恢复提示。
- v1 支持拨动开关自动/手动风扇控制，v2 支持阈值报警。
- v5/v6 使用 ESP8266 AT 固件连接 WiFi/MQTT，UART0 默认 9600。
- v7 使用 P3.1/P3.0 UART0 透传 GB2312 语音文本，默认 9600。

构建：
cmake -S projects/SGTZ_001 -B build/SGTZ_001_c51 -G Ninja -DTARGET_PLATFORM=mcs51_sdcc
cmake --build build/SGTZ_001_c51

版本切换使用 -DMVP_APP_VERSION=1..7。
