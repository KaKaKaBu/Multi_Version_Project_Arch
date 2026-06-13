# KQZL3 空气质量检测系统 — 产品功能规格

基于 STM32F103C8T6 的多功能空气质量检测系统，支持 PM2.5、温湿度、烟雾、CO 检测，具备自动/手动/阈值设置三种工作模式，支持 WiFi/蓝牙远程控制。

---

## 1. 产品概述

**MCU:** STM32F103C8T6  
**显示:** OLED (SSD1306, I2C)  
**传感器:** PM2.5 (GP2Y1014AU)、温湿度 (DHT11)、烟雾 (MQ-2)、CO (MQ-7)  
**执行器:** 继电器(风扇)、蜂鸣器、LED(灯光)  
**通信:** ESP8266 WiFi (部分版本)、JDY-31 蓝牙 (部分版本)  

---

## 2. 硬件清单

| 器件 | 型号/说明 | 驱动 |
|------|-----------|------|
| 主控 | STM32F103C8T6 | BSP + HAL |
| 显示 | OLED SSD1306 (I2C) | `display_driver_t` / "oled" |
| PM2.5传感器 | GP2Y1014AU (ADC) | `gas_sensor_t` / "pm25" |
| 温湿度传感器 | DHT11 (OneWire) | `temp_humidity_sensor_t` / "dht11" |
| 烟雾传感器 | MQ-2 (ADC) | `gas_sensor_t` / "mq2_smoke" |
| CO传感器 | MQ-7 (ADC) | `gas_sensor_t` / "mq7_co" |
| 继电器/风扇 | PA11 | `relay_driver_t` / "relay" |
| 蜂鸣器 | PA8 | `buzzer_driver_t` / "buzzer" |
| LED灯光 | PA6 | `led_driver_t` / "led" |
| WiFi模块 | ESP-01S (USART3) | `comm_driver_t` / "esp8266" |
| 蓝牙模块 | JDY-31 (USART2) | `comm_driver_t` / "jdy31" |
| 按键 | 4个 (PB4/PB5/PB13/PB14) | GPIO direct |

---

## 3. 版本功能矩阵

| 版本 | PM2.5 | 温湿度 | 烟雾 | CO | 风扇 | 蜂鸣器 | 灯光 | WiFi | 蓝牙 | 通信平台 |
|------|-------|--------|------|----|------|--------|------|------|------|----------|
| 001 | ✓ | - | - | - | ✓ | ✓ | ✓ | - | - | - |
| 002 | ✓ | ✓ | - | - | ✓ | ✓ | ✓ | - | - | - |
| 003 | ✓ | ✓ | - | - | ✓ | ✓ | ✓ | ✓ | - | App |
| 004 | ✓ | ✓ | - | - | ✓ | ✓ | ✓ | - | ✓ | App |
| 005 | ✓ | ✓ | ✓ | - | ✓ | ✓ | ✓ | - | - | - |
| 006 | ✓ | ✓ | ✓ | - | ✓ | ✓ | ✓ | ✓ | - | App |
| 007 | ✓ | ✓ | ✓ | - | ✓ | ✓ | ✓ | - | ✓ | App |
| 008 | ✓ | ✓ | ✓ | - | ✓ | ✓ | ✓ | ✓ | - | 阿里云 |
| 009 | ✓ | ✓ | ✓ | - | ✓ | ✓ | ✓ | ✓ | - | 微信小程序 |
| 010 | ✓ | ✓ | ✓ | - | ✓ | ✓ | ✓ | ✓ | - | 网页 |
| 011 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | - | - | - |
| 012 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | - | App |
| 013 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | - | ✓ | App |
| 014 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | - | 阿里云 |

---

## 4. 按键定义

| 按键 | 功能 |
|------|------|
| **K1 (PB4)** | 模式切换: 自动 → 手动 → 阈值设置 → 自动 |
| **K2 (PB5)** | 选择: 手动模式切换设备 / 阈值模式切换设置项 |
| **K3 (PB13)** | 增加/开启: 阈值模式增加值 / 手动模式开启设备 |
| **K4 (PB14)** | 减少/关闭: 阈值模式减少值 / 手动模式关闭设备 |

---

## 5. 工作模式说明

### 5.1 自动模式 (AUTO)
- OLED 实时显示各传感器数值
- 当任一传感器超过设定阈值时:
  - 继电器吸合 (风扇启动排风)
  - 蜂鸣器鸣叫 (声光报警)
  - LED 点亮 (灯光报警)
- 传感器数值恢复正常后自动关闭设备

### 5.2 手动模式 (MANUAL)
- OLED 显示当前选中的设备 (风扇/蜂鸣器/灯光)
- K2 切换选中设备
- K3 开启选中设备
- K4 关闭选中设备

### 5.3 阈值设置模式 (THRESHOLD)
- OLED 显示当前选中的阈值项目
- K2 切换设置项 (PM2.5/温度/湿度/烟雾/CO - 根据版本)
- K3 增加阈值
- K4 减少阈值

---

## 6. 引脚配置

| 功能 | 引脚 | 说明 |
|------|------|------|
| OLED SCL | PB6 | I2C时钟 |
| OLED SDA | PB7 | I2C数据 |
| DHT11 | PA5 | 温湿度数据 |
| PM2.5 ADC | PA0 | ADC通道0 |
| PM2.5 LED | PA12 | GP2Y1014AU 红外 LED 控制，低电平点亮 |
| MQ-2 ADC | PA1 | ADC通道1 (烟雾) |
| MQ-7 ADC | PA4 | ADC通道4 (CO) |
| 继电器/风扇 | PA11 | 排风控制 |
| 蜂鸣器 | PA8 | 报警声音 |
| LED灯光 | PA6 | 报警灯光 |
| 按键K1 | PB4 | 模式切换 |
| 按键K2 | PB5 | 选择 |
| 按键K3 | PB13 | 增加/开启 |
| 按键K4 | PB14 | 减少/关闭 |
| ESP8266 TX | PB10 | USART3 TX |
| ESP8266 RX | PB11 | USART3 RX |
| ESP8266 PD | PB0 | 电源控制 |
| ESP8266 RST | PB1 | 复位 |
| JDY-31 TX | PA2 | USART2 TX |
| JDY-31 RX | PA3 | USART2 RX |

---

## 7. WiFi/蓝牙通信协议 (JSON)

### 7.1 设备 → 手机 (遥测)
```json
{
  "pm25": 45,
  "temp": 25,
  "humidity": 60,
  "smoke": 120,
  "co": 15,
  "fan": 1,
  "buzzer": 0,
  "light": 1,
  "mode": "auto"
}
```

### 7.2 手机 → 设备 (命令)

| 命令 | JSON示例 | 功能 |
|------|----------|------|
| 设置模式 | `{"cmd":"set_mode","mode":"manual"}` | 切换工作模式 |
| 控制设备 | `{"cmd":"set_device","device":"fan","state":1}` | 开关设备 |
| 设置阈值 | `{"cmd":"set_threshold","sensor":"pm25","value":100}` | 设置报警阈值 |
| 查询状态 | `{"cmd":"get_status"}` | 请求发送遥测数据 |

### 7.3 WiFi/网页/小程序/App 默认 MQTT 参数

带有 WiFi/网页/小程序/App 的版本默认自动使用以下 MQTT 参数，与 `app/board_config.h` 保持一致：

| 参数 | 值 |
|------|----|
| Broker | `121.40.131.194` |
| Port | `1883` |
| Client ID | `KQZL3` |
| Username | `yskj` |
| Password | `yskj@123` |
| 设备订阅/上位机发布 Topic | `KQZL3` |
| 设备发布/上位机订阅 Topic | `KQZL3/web` |

---

## 8. 软件架构

### 8.1 任务 (sched_loop)

| 任务名 | 周期 | 职责 |
|--------|------|------|
| sensor_loop | 2000ms | 读取所有传感器数据 |
| alarm_loop | 1000ms | 自动模式下检查阈值并控制设备 |
| display_loop | 500ms | 更新OLED显示内容 |
| key_loop | 10ms | 按键消抖检测 |
| comm_loop | 事件驱动 | WiFi/蓝牙数据收发 |

### 8.2 代码结构

```
app/
├── app_main.c          - 主入口, sched_loop初始化
├── app_logic.c/h       - 应用逻辑 (传感器/显示/报警/按键)
├── app_callbacks.c     - 回调函数
├── board_config.h      - 硬件引脚配置
├── version_config.h    - 版本宏定义
└── stm32f10x_conf.h    - SPL配置
```

---

## 9. 构建说明

### 9.1 编译指定版本
```bash
cmake -S . -B build -G "Ninja" -DAPP_VERSION=5
cmake --build build
```

### 9.2 批量导出所有版本
```bash
python ../../tools/export_project.py --project KQZL3 --batch --versions 1-14 -o ../../exports --clean --extras readme.txt,PRODUCT_SPEC.md
```

---

## 10. 默认阈值参数

| 参数 | 默认值 | 步进 |
|------|--------|------|
| PM2.5阈值 | 100 μg/m³ | ±10 |
| 温度阈值 | 35 °C | ±1 |
| 湿度阈值 | 80 % | ±5 |
| 烟雾阈值 | 500 ppm | ±50 |
| CO阈值 | 50 ppm | ±5 |

---

*规格来源: KQZL3-001 ~ KQZL3-014 空气质量检测系统*
