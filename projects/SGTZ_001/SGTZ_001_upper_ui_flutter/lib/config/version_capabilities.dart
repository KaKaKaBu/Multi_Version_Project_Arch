import 'package:mvp_flutter_common/mvp_flutter_common.dart';

export 'package:mvp_flutter_common/mvp_flutter_common.dart' show VersionCapabilities;

VersionCapabilities resolveProjectUpperFeatures() {
  return resolveUpperFeaturesWithDefaults(
    defaultVersion: '7',
    defaultFeatures: 'common,weight,height,bmi,voice',
  );
}
