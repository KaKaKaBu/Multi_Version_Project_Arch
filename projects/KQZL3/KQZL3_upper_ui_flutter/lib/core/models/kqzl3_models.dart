enum WorkMode {
  auto('auto', '自动模式'),
  manual('manual', '手动模式'),
  threshold('threshold', '阈值设置');

  const WorkMode(this.key, this.label);

  final String key;
  final String label;

  static WorkMode fromKey(Object? value) {
    return WorkMode.values.firstWhere(
      (item) => item.key == value,
      orElse: () => WorkMode.auto,
    );
  }
}

class SensorDefinition {
  const SensorDefinition({
    required this.key,
    required this.label,
    required this.unit,
    required this.thresholdKey,
    required this.defaultThreshold,
    required this.step,
    required this.precision,
    required this.feature,
  });

  final String key;
  final String label;
  final String unit;
  final String thresholdKey;
  final num defaultThreshold;
  final num step;
  final int precision;
  final String feature;
}

class ActuatorDefinition {
  const ActuatorDefinition({
    required this.key,
    required this.label,
    required this.commandDevice,
  });

  final String key;
  final String label;
  final String commandDevice;
}

class TransportDefinition {
  const TransportDefinition({required this.key, required this.label});

  final String key;
  final String label;
}

class VersionCapabilities {
  const VersionCapabilities({required this.version, required this.features});

  final int version;
  final List<String> features;

  bool has(String feature) => features.contains(feature);

  Map<String, bool> get flags => {
    for (final feature in features) feature: true,
  };
}

class TelemetryState {
  const TelemetryState({
    required this.values,
    required this.mode,
    required this.alarm,
  });

  final Map<String, num?> values;
  final WorkMode mode;
  final int alarm;

  num? operator [](String key) => values[key];

  TelemetryState copyWith({
    Map<String, num?>? values,
    WorkMode? mode,
    int? alarm,
  }) {
    return TelemetryState(
      values: values ?? Map<String, num?>.from(this.values),
      mode: mode ?? this.mode,
      alarm: alarm ?? this.alarm,
    );
  }

  Map<String, dynamic> toJson() => {
    ...values,
    'mode': mode.key,
    'alarm': alarm,
  };
}

class ThresholdState {
  const ThresholdState(this.values);

  final Map<String, num> values;

  num? operator [](String key) => values[key];

  ThresholdState copyWithValue(String key, num value) {
    return ThresholdState({...values, key: value});
  }

  Map<String, dynamic> toJson() => Map<String, dynamic>.from(values);
}

class ConnectionConfig {
  const ConnectionConfig({
    required this.host,
    required this.port,
    required this.wsEndpoint,
    required this.username,
    required this.password,
    required this.commandTopic,
    required this.telemetryTopic,
    required this.keepalive,
    required this.useTLS,
  });

  final String host;
  final int port;
  final String wsEndpoint;
  final String username;
  final String password;
  final String commandTopic;
  final String telemetryTopic;
  final int keepalive;
  final bool useTLS;

  ConnectionConfig copyWith({
    String? host,
    int? port,
    String? wsEndpoint,
    String? username,
    String? password,
    String? commandTopic,
    String? telemetryTopic,
    int? keepalive,
    bool? useTLS,
  }) {
    return ConnectionConfig(
      host: host ?? this.host,
      port: port ?? this.port,
      wsEndpoint: wsEndpoint ?? this.wsEndpoint,
      username: username ?? this.username,
      password: password ?? this.password,
      commandTopic: commandTopic ?? this.commandTopic,
      telemetryTopic: telemetryTopic ?? this.telemetryTopic,
      keepalive: keepalive ?? this.keepalive,
      useTLS: useTLS ?? this.useTLS,
    );
  }
}

class LogEntry {
  const LogEntry({required this.id, required this.direction, required this.payload});

  final String id;
  final String direction;
  final String payload;
}
