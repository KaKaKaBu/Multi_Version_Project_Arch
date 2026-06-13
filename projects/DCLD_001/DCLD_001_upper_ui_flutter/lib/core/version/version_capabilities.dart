import '../models/dcld_models.dart';

const upperVersionDefine = String.fromEnvironment('UPPER_VERSION', defaultValue: '7');
const upperFeaturesDefine = String.fromEnvironment('UPPER_FEATURES', defaultValue: '');

const versionFeatureMatrix = <int, List<String>>{
  1: ['common'],
  2: ['common', 'temp'],
  3: ['common', 'temp', 'voice'],
  4: ['common', 'temp', 'remote', 'wifi', 'app'],
  5: ['common', 'temp', 'remote', 'ble', 'app'],
  6: ['common', 'temp', 'voice', 'remote', 'wifi', 'app'],
  7: ['common', 'temp', 'voice', 'remote', 'wifi', 'app', 'camera'],
};

const metricCatalog = <MetricDefinition>[
  MetricDefinition(
    key: 'distance_cm',
    label: '当前距离',
    unit: 'cm',
    precision: 0,
    feature: 'common',
    thresholdKey: 'distance',
  ),
  MetricDefinition(
    key: 'raw_distance_cm',
    label: '原始距离',
    unit: 'cm',
    precision: 0,
    feature: 'common',
  ),
  MetricDefinition(
    key: 'temperature_c',
    label: '环境温度',
    unit: '°C',
    precision: 1,
    feature: 'temp',
  ),
];

const thresholdCatalog = <ThresholdDefinition>[
  ThresholdDefinition(
    key: 'distance',
    label: '距离报警阈值',
    unit: 'cm',
    defaultValue: 80,
    step: 5,
    min: 20,
    max: 400,
  ),
];

final modeOptions = WorkMode.values;

int resolveUpperVersion([String value = upperVersionDefine]) {
  final version = int.tryParse(value) ?? 7;
  if (version >= 1 && version <= 7) {
    return version;
  }
  return 7;
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
  final matrixFeatures = versionFeatureMatrix[resolvedVersion] ?? versionFeatureMatrix[7]!;
  final merged = fromDefine.isNotEmpty ? fromDefine : matrixFeatures;
  return VersionCapabilities(version: resolvedVersion, features: _dedupe(merged));
}

List<MetricDefinition> getSupportedMetrics(VersionCapabilities capabilities) {
  return metricCatalog
      .where((item) => item.feature == 'common' || capabilities.has(item.feature))
      .toList(growable: false);
}

List<ThresholdDefinition> getSupportedThresholds() => thresholdCatalog;

List<TransportDefinition> getSupportedTransports(VersionCapabilities capabilities) {
  final transports = <TransportDefinition>[];
  if (capabilities.has('wifi')) {
    transports.add(const TransportDefinition(key: 'mqtt', label: 'WiFi MQTT'));
  }
  if (capabilities.has('ble')) {
    transports.add(const TransportDefinition(key: 'ble', label: 'JDY-31 蓝牙'));
  }
  return transports;
}

String describeVersion(VersionCapabilities capabilities) {
  final parts = <String>['超声波测距', 'OLED', '声光报警', '阈值设置'];
  if (capabilities.has('temp')) parts.add('温度补偿');
  if (capabilities.has('voice')) parts.add('语音播报');
  if (capabilities.has('wifi')) parts.add('WiFi App');
  if (capabilities.has('ble')) parts.add('蓝牙 App');
  if (capabilities.has('camera')) parts.add('ESP32-CAM');
  return "DCLD-${capabilities.version.toString().padLeft(3, '0')} · ${parts.join(' + ')}";
}

String enabledFeatureSummary(VersionCapabilities capabilities) {
  return getSupportedMetrics(capabilities).map((item) => item.label).join('/');
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
