import 'package:mvp_flutter_common/mvp_flutter_common.dart';

export 'package:mvp_flutter_common/mvp_flutter_common.dart' show MqttConfig;

const defaultMqttConfig = MqttConfig(
  host: '121.40.131.194',
  port: 1883,
  clientId: 'ZNLYJ_001_upper',
  username: 'yskj',
  password: 'yskj@123',
  commandTopic: 'ZNLYJ_001/sub',
  telemetryTopic: 'ZNLYJ_001/pub',
);
