import 'package:mvp_flutter_common/mvp_flutter_common.dart';

export 'package:mvp_flutter_common/mvp_flutter_common.dart' show MqttConfig;

const defaultMqttConfig = MqttConfig(
  host: '121.40.131.194',
  port: 1883,
  clientId: 'AQMJ_001_panel',
  username: 'yskj',
  password: 'yskj@123',
  commandTopic: 'AQMJ_001',
  telemetryTopic: 'AQMJ_001/web',
  cameraTopic: 'AQMJ_001/camera',
  keepAliveSeconds: 60,
);
