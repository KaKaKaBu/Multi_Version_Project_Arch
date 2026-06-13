import '../models/kqzl3_models.dart';

const upperVersionDefine = String.fromEnvironment('UPPER_VERSION', defaultValue: '14');
const upperFeaturesDefine = String.fromEnvironment('UPPER_FEATURES', defaultValue: '');

const versionFeatureMatrix = <int, List<String>>{
  1: ['common'],
  2: ['common', 'dht11'],
  3: ['common', 'dht11', 'remote', 'wifi', 'app'],
  4: ['common', 'dht11', 'remote', 'ble', 'app'],
  5: ['common', 'dht11', 'mq2'],
  6: ['common', 'dht11', 'mq2', 'remote', 'wifi', 'app'],
  7: ['common', 'dht11', 'mq2', 'remote', 'ble', 'app'],
  8: ['common', 'dht11', 'mq2', 'remote', 'wifi', 'aliyun', 'app'],
  9: ['common', 'dht11', 'mq2', 'remote', 'wifi', 'mpWeixin'],
  10: ['common', 'dht11', 'mq2', 'remote', 'wifi', 'web'],
  11: ['common', 'dht11', 'mq2', 'mq7'],
  12: ['common', 'dht11', 'mq2', 'mq7', 'remote', 'wifi', 'app'],
  13: ['common', 'dht11', 'mq2', 'mq7', 'remote', 'ble', 'app'],
  14: ['common', 'dht11', 'mq2', 'mq7', 'remote', 'wifi', 'aliyun', 'app'],
};

const sensorCatalog = <SensorDefinition>[
  SensorDefinition(
    key: 'pm25',
    label: 'PM2.5',
    unit: 'μg/m³',
    thresholdKey: 'pm25',
    defaultThreshold: 100,
    step: 10,
    precision: 0,
    feature: 'common',
  ),
  SensorDefinition(
    key: 'temp',
    label: '温度',
    unit: '°C',
    thresholdKey: 'temp',
    defaultThreshold: 35,
    step: 1,
    precision: 0,
    feature: 'dht11',
  ),
  SensorDefinition(
    key: 'humidity',
    label: '湿度',
    unit: '%',
    thresholdKey: 'humidity',
    defaultThreshold: 80,
    step: 5,
    precision: 0,
    feature: 'dht11',
  ),
  SensorDefinition(
    key: 'smoke',
    label: '烟雾',
    unit: 'ppm',
    thresholdKey: 'smoke',
    defaultThreshold: 500,
    step: 50,
    precision: 0,
    feature: 'mq2',
  ),
  SensorDefinition(
    key: 'co',
    label: '一氧化碳',
    unit: 'ppm',
    thresholdKey: 'co',
    defaultThreshold: 50,
    step: 5,
    precision: 0,
    feature: 'mq7',
  ),
];

const actuatorCatalog = <ActuatorDefinition>[
  ActuatorDefinition(key: 'fan', label: '排风风扇', commandDevice: 'fan'),
  ActuatorDefinition(key: 'buzzer', label: '蜂鸣器', commandDevice: 'buzzer'),
  ActuatorDefinition(key: 'light', label: '灯光报警', commandDevice: 'light'),
];

final modeOptions = WorkMode.values;

int resolveUpperVersion([String value = upperVersionDefine]) {
  final version = int.tryParse(value) ?? 14;
  if (version >= 1 && version <= 14) {
    return version;
  }
  return 14;
}

VersionCapabilities resolveUpperFeatures({
  int? version,
  String compileFeatures = upperFeaturesDefine,
}) {
  final resolvedVersion = version ?? resolveUpperVersion();
  final fromDefine = compileFeatures
      .split(',')
      .map((item) => item.trim())
      .where((item) => item.isNotEmpty)
      .toList();
  final matrixFeatures = versionFeatureMatrix[resolvedVersion] ?? versionFeatureMatrix[14]!;
  final merged = fromDefine.isNotEmpty ? fromDefine : matrixFeatures;
  return VersionCapabilities(version: resolvedVersion, features: _dedupe(merged));
}

List<SensorDefinition> getSupportedSensors(VersionCapabilities capabilities) {
  return sensorCatalog
      .where((item) => item.feature == 'common' || capabilities.has(item.feature))
      .toList(growable: false);
}

List<SensorDefinition> getSupportedThresholds(VersionCapabilities capabilities) {
  return getSupportedSensors(capabilities);
}

List<ActuatorDefinition> getSupportedActuators() => actuatorCatalog;

List<TransportDefinition> getSupportedTransports(VersionCapabilities capabilities) {
  final transports = <TransportDefinition>[];
  if (capabilities.has('wifi')) {
    transports.add(
      TransportDefinition(
        key: 'mqtt',
        label: capabilities.has('aliyun') ? '阿里云 MQTT' : 'WiFi MQTT',
      ),
    );
  }
  if (capabilities.has('ble')) {
    transports.add(const TransportDefinition(key: 'ble', label: 'JDY-31 蓝牙'));
  }
  return transports;
}

String describeVersion(VersionCapabilities capabilities) {
  final parts = <String>['PM2.5', '排风', '蜂鸣器', '灯光'];
  if (capabilities.has('dht11')) {
    parts.insert(1, '温湿度');
  }
  if (capabilities.has('mq2')) {
    parts.insert(capabilities.has('dht11') ? 2 : 1, '烟雾');
  }
  if (capabilities.has('mq7')) {
    parts.insert(capabilities.has('mq2') ? 3 : 1, 'CO');
  }
  if (capabilities.has('wifi')) {
    parts.add(
      capabilities.has('aliyun')
          ? '阿里云'
          : capabilities.has('mpWeixin')
              ? '微信小程序'
              : capabilities.has('web')
                  ? '网页'
                  : 'WiFi App',
    );
  }
  if (capabilities.has('ble')) {
    parts.add('蓝牙 App');
  }
  return "KQZL3-${capabilities.version.toString().padLeft(3, '0')} · ${parts.join(' + ')}";
}

List<String> _dedupe(List<String> values) {
  final seen = <String>{};
  final result = <String>[];
  for (final value in values) {
    if (seen.add(value)) {
      result.add(value);
    }
  }
  return result;
}
