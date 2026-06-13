import '../models/dcld_models.dart';

const boardMqttConfig = ConnectionConfig(
  host: '121.40.131.194',
  port: 1883,
  wsEndpoint: '',
  username: 'yskj',
  password: 'yskj@123',
  commandTopic: 'DCLD_001',
  telemetryTopic: 'DCLD_001/web',
  keepalive: 60,
  useTLS: false,
);

const boardMqttDeviceClientId = 'DCLD_001';
const boardMqttPanelClientIdPrefix = 'DCLD_001_panel';

String buildClientId([String suffix = 'flutter']) {
  final timestamp = DateTime.now().millisecondsSinceEpoch.toRadixString(16);
  return '${boardMqttPanelClientIdPrefix}_${suffix}_$timestamp';
}
