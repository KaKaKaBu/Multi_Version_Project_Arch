import '../config/mqtt_config.dart';
import 'transport_service.dart';

class MqttTransport extends MockTransport {
  MqttTransport(MqttConfig config);

  @override
  Future<bool> connect(Map<String, String> config) async {
    throw UnsupportedError('当前平台不支持直连 MQTT TCP，请使用 Android 或桌面端运行。');
  }
}
