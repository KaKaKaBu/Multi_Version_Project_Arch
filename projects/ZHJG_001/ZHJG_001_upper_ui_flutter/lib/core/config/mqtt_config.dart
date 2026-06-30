class MqttConfig {
  const MqttConfig({
    required this.host,
    required this.port,
    required this.clientId,
    required this.username,
    required this.password,
    required this.commandTopic,
    required this.telemetryTopic,
    this.keepAliveSeconds = 20,
  });

  final String host;
  final int port;
  final String clientId;
  final String username;
  final String password;
  final String commandTopic;
  final String telemetryTopic;
  final int keepAliveSeconds;
}

const defaultMqttConfig = MqttConfig(
  host: '121.40.131.194',
  port: 1883,
  clientId: 'ZHJG_001_upper',
  username: 'yskj',
  password: 'yskj@123',
  commandTopic: 'ZHJG_001',
  telemetryTopic: 'ZHJG_001/web',
);
