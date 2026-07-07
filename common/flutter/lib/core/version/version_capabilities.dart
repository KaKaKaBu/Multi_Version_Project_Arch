class VersionCapabilities {
  final int version;
  final List<String> features;

  const VersionCapabilities({required this.version, required this.features});

  bool has(String feature) => features.contains(feature);
}

const _defaultFeatures = <String>['common'];
const _featuresByVersion = <int, List<String>>{
  1: ['common'],
  2: ['common', 'ble', 'app'],
  3: ['common', 'wifi', 'app'],
  4: ['common', 'wifi', 'app', 'camera'],
  5: ['common', 'cloud', 'app'],
  6: ['common', 'voice'],
  7: ['common', 'ble', 'app', 'voice'],
  8: ['common', 'wifi', 'app', 'voice'],
  9: ['common', 'wifi', 'app', 'camera', 'voice'],
  10: ['common', 'cloud', 'app', 'voice'],
};

VersionCapabilities resolveUpperFeatures() {
  const versionStr = String.fromEnvironment('UPPER_VERSION', defaultValue: '1');
  const featuresStr = String.fromEnvironment('UPPER_FEATURES', defaultValue: '');
  final version = int.tryParse(versionStr) ?? 1;
  final explicitFeatures = _parseFeatures(featuresStr);

  return VersionCapabilities(
    version: version,
    features: explicitFeatures.isNotEmpty
        ? explicitFeatures
        : (_featuresByVersion[version] ?? _defaultFeatures),
  );
}

VersionCapabilities resolveUpperFeaturesWithDefaults({
  required String defaultVersion,
  required String defaultFeatures,
}) {
  const versionStr = String.fromEnvironment('UPPER_VERSION');
  const featuresStr = String.fromEnvironment('UPPER_FEATURES');
  final version = int.tryParse(versionStr.isEmpty ? defaultVersion : versionStr) ?? 1;
  final explicitFeatures = _parseFeatures(
    featuresStr.isEmpty ? defaultFeatures : featuresStr,
  );

  return VersionCapabilities(version: version, features: explicitFeatures);
}

List<String> _parseFeatures(String raw) {
  final seen = <String>{};
  final features = <String>[];
  for (final item in raw.split(',')) {
    final feature = item.trim();
    if (feature.isEmpty || seen.contains(feature)) continue;
    seen.add(feature);
    features.add(feature);
  }
  return features;
}
