import '../config/huawei_iotda_config.dart';
import 'transport_service.dart';

class HuaweiIotdaTransport extends MockTransport {
  HuaweiIotdaTransport(HuaweiIotdaConfig config);

  @override
  Future<bool> connect(Map<String, String> config) async {
    throw UnsupportedError('当前平台不支持华为云 MQTT WebSocket，请使用 Android 或桌面端运行。');
  }
}
