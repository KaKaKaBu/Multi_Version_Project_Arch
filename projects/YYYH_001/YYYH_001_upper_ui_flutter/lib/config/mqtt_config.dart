import 'package:mvp_flutter_common/mvp_flutter_common.dart';

export 'package:mvp_flutter_common/mvp_flutter_common.dart' show MqttConfig;

const defaultMqttConfig = MqttConfig(
  host: '121.40.131.194',
  port: 1883,
  clientId: 'YYYH_panel',
  username: 'yskj',
  password: 'yskj@123',
  commandTopic: 'YYYH',
  telemetryTopic: 'YYYH/web',
  cameraTopic: 'YYYH/camera',
  keepAliveSeconds: 60,
);
