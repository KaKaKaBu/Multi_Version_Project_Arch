import 'package:mvp_flutter_common/mvp_flutter_common.dart';

export 'package:mvp_flutter_common/mvp_flutter_common.dart' show MqttConfig;

const defaultMqttConfig = MqttConfig(
  host: '121.40.131.194',
  port: 1883,
  clientId: 'FJXT_001_upper',
  username: 'yskj',
  password: 'yskj@123',
  commandTopic: 'FJXT_001',
  telemetryTopic: 'FJXT_001/web',
  cameraTopic: 'FJXT_001',
);
