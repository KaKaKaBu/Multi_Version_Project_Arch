# AQMJ_001 智能门禁

STM32F103C8T6 门禁/门铃系列固件，覆盖 AQMJ-001 至 AQMJ-016 与 AQMJ-018；AQMJ-017 为 51 版本，暂未接入。

## 驱动接入

- `ir_remote`：红外遥控输入驱动，替代 315M 无线控制模块，按 `input_driver_t` 返回 1-5 键值。
- `isd1820`：ISD1820 录音/播放模块，按 `audio_recorder_driver_t` 暴露录音与播放 GPIO 控制。
- `e18_presence`：门前有人检测。
- `ds1302_rtc`：北京时间与最近门铃呼叫时间。
- `gl5506` + `relay`：光照检测与补光/驱离灯控制。
- `tts_uart`：JR6001/TTS 语音播报串口透传。
- `jdy31` / `esp8266_mqtt`：蓝牙、WiFi、华为云版本远程查看与控制。

## 本地按键

- K1：在家/外出切换。
- K2：设防/撤防切换。
- K3：清除报警；留言版本为播放留言。
- K4：灯光切换；留言版本为录音开关。
- 门铃输入：接入同一按键服务的第 5 路。

## 远程命令

- `get_status`
- `clear_alarm`
- `doorbell`
- `play_message`
- `record_start`
- `record_stop`
- 属性：`armed`、`home`、`lamp`
