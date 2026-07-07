import 'dart:convert';

import 'package:crypto/crypto.dart';

class HuaweiIotdaConfig {
  const HuaweiIotdaConfig({
    required this.hostname,
    required this.websocketPath,
    required this.port,
    required this.appDeviceId,
    required this.appDeviceSecret,
    required this.stm32DeviceId,
    required this.serviceId,
    required this.appCustomPublishTopic,
    required this.appCustomSubscribeTopic,
    this.keepAliveSeconds = 60,
  });

  final String hostname;
  final String websocketPath;
  final int port;
  final String appDeviceId;
  final String appDeviceSecret;
  final String stm32DeviceId;
  final String serviceId;
  final String appCustomPublishTopic;
  final String appCustomSubscribeTopic;
  final int keepAliveSeconds;

  String get websocketUrl => 'wss://$hostname:$port$websocketPath';

  HuaweiMqttCredential createCredential({DateTime? now}) {
    final timestamp = _utcHour(now ?? DateTime.now().toUtc());
    return HuaweiMqttCredential(
      clientId: '${appDeviceId}_0_0_$timestamp',
      username: appDeviceId,
      password: _hmacSha256(appDeviceSecret, timestamp),
    );
  }
}

class HuaweiMqttCredential {
  const HuaweiMqttCredential({
    required this.clientId,
    required this.username,
    required this.password,
  });

  final String clientId;
  final String username;
  final String password;
}

String _utcHour(DateTime utc) {
  String two(int value) => value.toString().padLeft(2, '0');
  return '${utc.year}${two(utc.month)}${two(utc.day)}${two(utc.hour)}';
}

String _hmacSha256(String secret, String timestamp) {
  final hmac = Hmac(sha256, utf8.encode(timestamp));
  return hmac.convert(utf8.encode(secret)).toString();
}
