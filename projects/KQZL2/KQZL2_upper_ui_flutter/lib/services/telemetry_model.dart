class AirSensorSpec {
  const AirSensorSpec({
    required this.key,
    required this.label,
    required this.unit,
    required this.thresholdKey,
    required this.defaultThreshold,
    required this.step,
    required this.feature,
  });

  final String key;
  final String label;
  final String unit;
  final String thresholdKey;
  final int defaultThreshold;
  final int step;
  final String feature;
}

class AirActuatorSpec {
  const AirActuatorSpec({
    required this.key,
    required this.label,
    required this.commandDevice,
  });

  final String key;
  final String label;
  final String commandDevice;
}

const airSensorCatalog = <AirSensorSpec>[
  AirSensorSpec(
    key: 'pm25',
    label: 'PM2.5',
    unit: 'ug/m3',
    thresholdKey: 'pm25',
    defaultThreshold: 100,
    step: 10,
    feature: 'common',
  ),
  AirSensorSpec(
    key: 'temp',
    label: '温度',
    unit: 'C',
    thresholdKey: 'temp',
    defaultThreshold: 35,
    step: 1,
    feature: 'dht11',
  ),
  AirSensorSpec(
    key: 'humidity',
    label: '湿度',
    unit: '%',
    thresholdKey: 'humidity',
    defaultThreshold: 80,
    step: 5,
    feature: 'dht11',
  ),
  AirSensorSpec(
    key: 'smoke',
    label: '烟雾',
    unit: 'ppm',
    thresholdKey: 'smoke',
    defaultThreshold: 500,
    step: 50,
    feature: 'mq2',
  ),
  AirSensorSpec(
    key: 'co',
    label: 'CO',
    unit: 'ppm',
    thresholdKey: 'co',
    defaultThreshold: 50,
    step: 5,
    feature: 'mq7',
  ),
];

const airActuatorCatalog = <AirActuatorSpec>[
  AirActuatorSpec(key: 'fan', label: '排风', commandDevice: 'fan'),
  AirActuatorSpec(key: 'buzzer', label: '蜂鸣器', commandDevice: 'buzzer'),
  AirActuatorSpec(key: 'light', label: '灯光', commandDevice: 'light'),
];

class AirQualityTelemetry {
  const AirQualityTelemetry({
    this.version = 'KQZL2',
    this.versionNo = 14,
    this.pm25 = 0,
    this.temp,
    this.humidity,
    this.smoke,
    this.co,
    this.fan = false,
    this.buzzer = false,
    this.light = false,
    this.alarm = false,
    this.mode = 'auto',
    this.thresholds = const <String, int>{},
  });

  final String version;
  final int versionNo;
  final int pm25;
  final int? temp;
  final int? humidity;
  final int? smoke;
  final int? co;
  final bool fan;
  final bool buzzer;
  final bool light;
  final bool alarm;
  final String mode;
  final Map<String, int> thresholds;

  String get modeLabel {
    switch (mode) {
      case 'manual':
        return '手动模式';
      case 'threshold':
        return '阈值设置';
      case 'auto':
      default:
        return '自动模式';
    }
  }

  int? sensorValue(String key) {
    switch (key) {
      case 'pm25':
        return pm25;
      case 'temp':
        return temp;
      case 'humidity':
        return humidity;
      case 'smoke':
        return smoke;
      case 'co':
        return co;
      default:
        return null;
    }
  }

  bool actuatorState(String key) {
    switch (key) {
      case 'fan':
        return fan;
      case 'buzzer':
        return buzzer;
      case 'light':
        return light;
      default:
        return false;
    }
  }

  factory AirQualityTelemetry.fromJson(
    Map<String, dynamic> json, {
    AirQualityTelemetry? previous,
  }) {
    final base = previous ?? const AirQualityTelemetry();
    final data = _jsonMap(json['data']) ?? json;
    final rawThresholds = _jsonMap(data['thresholds']) ?? const {};
    final thresholds = Map<String, int>.from(base.thresholds);
    for (final sensor in airSensorCatalog) {
      if (rawThresholds[sensor.thresholdKey] != null) {
        thresholds[sensor.thresholdKey] = _toInt(
          rawThresholds[sensor.thresholdKey],
          thresholds[sensor.thresholdKey] ?? sensor.defaultThreshold,
        );
      }
      final flatKey = '${sensor.thresholdKey}_threshold';
      if (data[flatKey] != null) {
        thresholds[sensor.thresholdKey] = _toInt(
          data[flatKey],
          thresholds[sensor.thresholdKey] ?? sensor.defaultThreshold,
        );
      }
    }

    return AirQualityTelemetry(
      version: _firstText([data['version'], base.version]),
      versionNo: _toInt(data['version_no'], base.versionNo),
      pm25: _toInt(data['pm25'], base.pm25),
      temp: data.containsKey('temp') ? _toInt(data['temp'], base.temp ?? 0) : base.temp,
      humidity: data.containsKey('humidity')
          ? _toInt(data['humidity'], base.humidity ?? 0)
          : base.humidity,
      smoke: data.containsKey('smoke')
          ? _toInt(data['smoke'], base.smoke ?? 0)
          : base.smoke,
      co: data.containsKey('co') ? _toInt(data['co'], base.co ?? 0) : base.co,
      fan: _toBool(data['fan'], base.fan),
      buzzer: _toBool(data['buzzer'], base.buzzer),
      light: _toBool(data['light'], base.light),
      alarm: _toBool(data['alarm'], base.alarm),
      mode: _normalizeMode(_firstText([data['mode'], base.mode])),
      thresholds: thresholds,
    );
  }

  static Map<String, int> defaultThresholds() {
    return {
      for (final sensor in airSensorCatalog)
        sensor.thresholdKey: sensor.defaultThreshold,
    };
  }

  static bool isTelemetryPayload(Map<String, dynamic> json) {
    final data = _jsonMap(json['data']) ?? json;
    return json['type'] == 'telemetry' ||
        data.containsKey('pm25') ||
        data.containsKey('temp') ||
        data.containsKey('humidity') ||
        data.containsKey('smoke') ||
        data.containsKey('co') ||
        data.containsKey('fan') ||
        data.containsKey('buzzer') ||
        data.containsKey('light');
  }
}

Map<String, dynamic>? _jsonMap(Object? value) {
  if (value is Map) return Map<String, dynamic>.from(value);
  return null;
}

String _firstText(List<Object?> values) {
  for (final value in values) {
    final text = value?.toString().trim() ?? '';
    if (text.isNotEmpty) return text;
  }
  return '';
}

int _toInt(Object? value, int fallback) {
  if (value is num) return value.toInt();
  return int.tryParse(value?.toString() ?? '') ?? fallback;
}

bool _toBool(Object? value, bool fallback) {
  if (value == null) return fallback;
  if (value is bool) return value;
  if (value is num) return value.toInt() != 0;
  final text = value.toString().trim().toLowerCase();
  if (text == 'true' || text == 'on' || text == '1') return true;
  if (text == 'false' || text == 'off' || text == '0') return false;
  return fallback;
}

String _normalizeMode(String mode) {
  return switch (mode) {
    'manual' => 'manual',
    'threshold' => 'threshold',
    _ => 'auto',
  };
}
