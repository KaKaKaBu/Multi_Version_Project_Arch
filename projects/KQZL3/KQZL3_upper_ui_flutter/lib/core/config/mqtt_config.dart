import '../models/kqzl3_models.dart';

const boardMqttConfig = ConnectionConfig(
  host: '121.40.131.194',
  port: 1883,
  wsEndpoint: '',
  username: 'yskj',
  password: 'yskj@123',
  commandTopic: 'KQZL3',
  telemetryTopic: 'KQZL3/web',
  keepalive: 60,
  useTLS: false,
);

const boardMqttDeviceClientId = 'KQZL3';
const boardMqttPanelClientIdPrefix = 'KQZL3_panel';

String buildClientId([String suffix = 'flutter']) {
  final timestamp = DateTime.now().millisecondsSinceEpoch.toRadixString(16);
  return '${boardMqttPanelClientIdPrefix}_${suffix}_$timestamp';
}

class AliyunMqttConfig {
  const AliyunMqttConfig({
    required this.productKey,
    required this.deviceName,
    required this.regionId,
    required this.wsEndpoint,
    required this.commandTopicTemplate,
    required this.telemetryTopicTemplate,
  });

  final String productKey;
  final String deviceName;
  final String regionId;
  final String wsEndpoint;
  final String commandTopicTemplate;
  final String telemetryTopicTemplate;
}

const aliyunMqttConfig = AliyunMqttConfig(
  productKey: '',
  deviceName: '',
  regionId: 'cn-shanghai',
  wsEndpoint: '',
  commandTopicTemplate: '/sys/{productKey}/{deviceName}/thing/service/property/set',
  telemetryTopicTemplate: '/sys/{productKey}/{deviceName}/thing/event/property/post',
);

String resolveAliyunTopic(
  String template, {
  String productKey = '',
  String deviceName = '',
}) {
  return template
      .replaceAll('{productKey}', productKey)
      .replaceAll('{deviceName}', deviceName);
}
