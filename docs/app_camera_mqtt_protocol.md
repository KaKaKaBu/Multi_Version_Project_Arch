# ESP32 OV3660 摄像头 MQTT 与监控画面接入说明

## 1. 设备发送的 JSON 数据格式

设备连接 Wi-Fi 和 MQTT 成功后，会周期性向 MQTT 发布设备在线与视频流地址信息。

### MQTT 连接参数

```c
#define BOARD_ESP8266_MQTT_BROKER "121.40.131.194"
#define BOARD_ESP8266_MQTT_PORT 1883U
#define BOARD_ESP8266_MQTT_CLIENT_ID "ZHJG_001_cam"
#define BOARD_ESP8266_MQTT_USER "yskj"
#define BOARD_ESP8266_MQTT_PASS "yskj@123"
#define BOARD_ESP8266_MQTT_SUB_TOPIC "ZHJG_001"
#define BOARD_ESP8266_MQTT_PUB_TOPIC "ZHJG_001/web"
```

当前 ESP32 工程中对应配置为：

```ini
CONFIG_MQTT_BROKER_URI="mqtt://121.40.131.194:1883"
CONFIG_MQTT_CLIENT_ID="ZHJG_001_cam"
CONFIG_MQTT_USER="yskj"
CONFIG_MQTT_PASSWORD="yskj@123"
CONFIG_MQTT_TOPIC_SERVER="ZHJG_001"
CONFIG_MQTT_TOPIC_CAM_UP="ZHJG_001/web"
```

### 设备发布主题

```text
ZHJG_001/web
```

### 设备发布 JSON

当 MJPEG 视频服务启用时，设备发布的数据格式如下：

```json
{
  "device_id": "esp32-ov3660",
  "cam_ip": "10.132.56.102",
  "mjpeg_url": "http://10.132.56.102:8080/stream",
  "mjpeg_port": 8080
}
```

字段说明：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `device_id` | string | 设备 ID，对应固件中的 `CONFIG_DEVICE_ID` |
| `cam_ip` | string | ESP32 摄像头设备当前局域网 IP |
| `mjpeg_url` | string | App 可访问的 MJPEG 视频流地址 |
| `mjpeg_port` | number | MJPEG HTTP 服务端口，当前为 `8080` |

如果 MJPEG 服务未启用，设备只发布：

```json
{
  "device_id": "esp32-ov3660",
  "cam_ip": "10.132.56.102"
}
```

## 2. App 如何显示监控画面

设备端当前提供的是 HTTP MJPEG 视频流，地址格式为：

```text
http://<cam_ip>:8080/stream
```

例如：

```text
http://10.132.56.102:8080/stream
```

App 的处理流程：

1. App 连接 MQTT broker：`121.40.131.194:1883`。
2. App 订阅设备发布主题：`ZHJG_001/web`。
3. App 收到设备 JSON 后，解析 `device_id`、`cam_ip`、`mjpeg_url`。
4. App 判断手机当前网络 IP 与设备 `cam_ip` 是否在同一个局域网。
5. 如果在同一个局域网，App 使用 `mjpeg_url` 打开视频流。
6. 如果不在同一个局域网，App 不连接视频流，并提示用户切换到设备所在 Wi-Fi。

### Android 显示方式建议

原生 `ImageView` 不能直接稳定播放 MJPEG。建议使用以下方式之一：

- 使用支持 MJPEG 的第三方控件或库。
- 使用 `WebView` 加载 `mjpeg_url`。
- 自己建立 HTTP 连接，按 MJPEG boundary 解析 JPEG 帧，然后刷新到 `ImageView`。

最简单方式是 `WebView`：

```kotlin
webView.settings.javaScriptEnabled = true
webView.loadUrl("http://10.132.56.102:8080/stream")
```

如果使用自定义播放器，需要按如下 HTTP 响应解析：

```http
Content-Type: multipart/x-mixed-replace; boundary=frame
```

每一帧格式：

```http
--frame
Content-Type: image/jpeg
Content-Length: <jpeg_length>

<jpeg binary data>
```

## 3. 仅同一局域网内才进行图像传输

当前设备发布的 `mjpeg_url` 是局域网地址，例如：

```text
http://10.132.56.102:8080/stream
```

这个地址只能在同一个局域网内访问，不能跨公网直接访问。因此 App 必须先判断 App 所在网络与设备 IP 是否属于同一局域网，只有满足条件才发起视频流连接。

### 判断规则

App 获取本机 Wi-Fi IP、子网掩码，然后与 MQTT 收到的 `cam_ip` 做网段判断：

```text
(app_ip & netmask) == (cam_ip & netmask)
```

如果相等，说明 App 和设备在同一个局域网，可以访问：

```text
http://<cam_ip>:8080/stream
```

如果不相等，App 不应打开视频流。

### 示例

可以连接：

```text
App IP:    10.132.56.88
Device IP: 10.132.56.102
Netmask:   255.255.255.0
Result:    同一局域网，允许视频传输
```

不连接：

```text
App IP:    192.168.1.20
Device IP: 10.132.56.102
Netmask:   255.255.255.0
Result:    不在同一局域网，不进行视频传输
```

### App 伪代码

```kotlin
data class CameraOnlineMessage(
    val device_id: String,
    val cam_ip: String,
    val mjpeg_url: String?,
    val mjpeg_port: Int?
)

fun isSameLan(appIp: Int, camIp: Int, netmask: Int): Boolean {
    return (appIp and netmask) == (camIp and netmask)
}

fun onMqttMessage(json: CameraOnlineMessage) {
    val appIp = getWifiIpAddress()
    val netmask = getWifiNetmask()
    val camIp = ipv4ToInt(json.cam_ip)

    if (!isSameLan(appIp, camIp, netmask)) {
        showMessage("手机和摄像头不在同一个局域网，无法查看实时画面")
        return
    }

    val streamUrl = json.mjpeg_url ?: "http://${json.cam_ip}:${json.mjpeg_port ?: 8080}/stream"
    openMjpegStream(streamUrl)
}
```

## 4. 服务器下发到设备的 JSON 格式

设备订阅主题：

```text
ZHJG_001
```

### 设置上传服务器地址

服务器或 App 可以向 `ZHJG_001` 发送：

```json
{
  "target_device_id": "esp32-ov3660",
  "server_ip": "10.132.56.50",
  "http_port": 5000
}
```

设备会转换为：

```text
http://10.132.56.50:5000/api/image
```

也可以直接发送完整上传地址：

```json
{
  "target_device_id": "esp32-ov3660",
  "upload_url": "http://10.132.56.50:5000/api/image"
}
```

### 触发单次拍照上传

```json
{
  "cmd": "capture_entry",
  "target_device_id": "esp32-ov3660",
  "upload_url": "http://10.132.56.50:5000/api/image"
}
```

字段说明：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `cmd` | string | 命令类型，`capture_entry` 表示立即拍照并上传一次 |
| `target_device_id` | string | 目标设备 ID；为空或不存在时，设备不做 ID 过滤 |
| `server_ip` | string | HTTP 上传服务器 IP |
| `http_port` | number | HTTP 上传服务器端口，默认 `5000` |
| `upload_url` | string | 完整 HTTP 上传地址，优先级高于 `server_ip` + `http_port` |

## 5. 推荐 App 逻辑

```text
启动 App
  |
  |-- 连接 MQTT broker
  |
  |-- 订阅 ZHJG_001/web
  |
  |-- 收到设备 JSON
        |
        |-- 解析 cam_ip / mjpeg_url
        |
        |-- 获取 App 当前 Wi-Fi IP 和子网掩码
        |
        |-- 判断是否同一局域网
              |
              |-- 是：打开 http://<cam_ip>:8080/stream 显示监控画面
              |
              |-- 否：不连接视频流，提示切换到设备所在 Wi-Fi
```

## 6. 注意事项

- MQTT 只负责设备发现、状态上报和命令下发，不直接传输实时视频。
- 实时画面通过 HTTP MJPEG 从设备局域网 IP 拉流。
- App 与设备不在同一局域网时，不进行图像传输。
- 如果需要公网查看画面，需要增加中转服务器、RTMP/RTSP 网关、WebRTC 或云端图片上传机制，当前局域网 MJPEG 地址不能直接公网访问。
- 当前视频流端口为 `8080`，路径为 `/stream`。
