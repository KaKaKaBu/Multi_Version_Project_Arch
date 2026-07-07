import 'package:mvp_flutter_common/mvp_flutter_common.dart';

export 'package:mvp_flutter_common/mvp_flutter_common.dart' show MqttConfig;

MqttConfig defaultMqttConfig() {
  return MqttConfig(
    host: '121.40.131.194',
    port: 1883,
    clientId: 'yskj_zncz_001_panel_${DateTime.now().millisecondsSinceEpoch.toRadixString(16)}',
    username: 'yskj',
    password: 'yskj@123',
    commandTopic: 'ZNCZ_001/web',
    telemetryTopic: 'ZNCZ_001',
    keepAliveSeconds: 60,
  );
}
