class VersionCapabilities {
  final int version;
  final List<String> features;

  const VersionCapabilities({required this.version, required this.features});

  bool has(String feature) => features.contains(feature);
}

VersionCapabilities resolveUpperFeatures() {
  const versionStr = String.fromEnvironment('UPPER_VERSION', defaultValue: '3');
  const featuresStr = String.fromEnvironment(
    'UPPER_FEATURES',
    defaultValue: 'common,wifi,ble,control',
  );

  return VersionCapabilities(
    version: int.tryParse(versionStr) ?? 1,
    features: featuresStr.split(',').map((e) => e.trim()).toList(),
  );
}
