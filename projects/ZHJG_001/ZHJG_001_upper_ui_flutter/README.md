# ZHJG_001 智能井盖上位机

该目录是 ZHJG_001 智能井盖项目的 Flutter 上位机源码，用于版本 3 的 WiFi/MQTT 远程监测，也可保留为版本 1/2 的本地查看与后续扩展基础。

## 功能

- 实时显示沼气浓度、水位、井盖倾角和综合报警状态。
- 支持 GPS 定位状态与经纬度显示。
- 支持自动模式 / 阈值设置模式切换。
- 支持下发沼气、水位、倾角报警阈值。
- 支持 MQTT 连接固件遥测与命令通道。

## 版本能力

| UPPER_VERSION | 对应产品 | 能力 |
| --- | --- | --- |
| 1 | ZHJG-001 | 沼气、水位、倾角、声光报警状态显示 |
| 2 | ZHJG-002 | 版本 1 + GPS / 短信能力状态显示 |
| 3 | ZHJG-003 | 版本 1 + GPS + WiFi/MQTT + APP 阈值设置和异常提醒 |

导出脚本会根据 `version_features.json` 写入 `upper_manifest.txt`。手动构建时可传入：

```bash
flutter build apk --debug \
  --dart-define=UPPER_VERSION=3 \
  --dart-define=UPPER_FEATURES=common,gps,wifi,app,alert
```

## MQTT 协议

默认连接参数与固件 `projects/ZHJG_001/board/board_config.h` 保持一致：

- Broker：`121.40.131.194:1883`
- Client ID：`ZHJG_001_upper`
- 命令 Topic：`ZHJG_001`
- 遥测 Topic：`ZHJG_001/web`

遥测数据为固件发布的 JSON：

```json
{
  "type": "telemetry",
  "version": "ZHJG_001",
  "version_no": 3,
  "data": {
    "methane_ppm": 1200,
    "water_level_percent": 82,
    "tilt_degree": 18,
    "alarm": 1,
    "mode": "auto"
  }
}
```

上位机下发命令：

```json
{"cmd":"get_status"}
{"cmd":"set_threshold","params":{"methane_ppm":900,"water_level_percent":75,"tilt_degree":12}}
{"cmd":"set_mode","params":{"mode":"auto"}}
{"cmd":"set_mode","params":{"mode":"threshold"}}
```
