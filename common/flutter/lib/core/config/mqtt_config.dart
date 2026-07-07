class MqttConfig {
  const MqttConfig({
    required this.host,
    required this.port,
    required this.clientId,
    required this.username,
    required this.password,
    required this.commandTopic,
    required this.telemetryTopic,
    this.cameraTopic,
    this.keepAliveSeconds = 20,
  });

  final String host;
  final int port;
  final String clientId;
  final String username;
  final String password;
  final String commandTopic;
  final String telemetryTopic;
  final String? cameraTopic;
  final int keepAliveSeconds;

  MqttConfig copyWith({
    String? host,
    int? port,
    String? clientId,
    String? username,
    String? password,
    String? commandTopic,
    String? telemetryTopic,
    String? cameraTopic,
    int? keepAliveSeconds,
  }) {
    return MqttConfig(
      host: host ?? this.host,
      port: port ?? this.port,
      clientId: clientId ?? this.clientId,
      username: username ?? this.username,
      password: password ?? this.password,
      commandTopic: commandTopic ?? this.commandTopic,
      telemetryTopic: telemetryTopic ?? this.telemetryTopic,
      cameraTopic: cameraTopic ?? this.cameraTopic,
      keepAliveSeconds: keepAliveSeconds ?? this.keepAliveSeconds,
    );
  }
}
