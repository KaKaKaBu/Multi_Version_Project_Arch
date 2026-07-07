import 'package:mvp_flutter_common/mvp_flutter_common.dart';

export 'package:mvp_flutter_common/mvp_flutter_common.dart'
    show VersionCapabilities;

VersionCapabilities resolveProjectUpperFeatures() {
  return resolveUpperFeaturesWithDefaults(
    defaultVersion: '14',
    defaultFeatures: 'common,pm25,dht11,mq2,mq7,remote,wifi,cloud,app',
  );
}

bool sensorSupported(VersionCapabilities capabilities, String feature) {
  return feature == 'common' || capabilities.has(feature);
}

String describeKqzl2Version(VersionCapabilities capabilities) {
  final parts = <String>['PM2.5', '排风', '蜂鸣器', '灯光'];
  if (capabilities.has('dht11')) parts.insert(1, '温湿度');
  if (capabilities.has('mq2')) parts.add('烟雾');
  if (capabilities.has('mq7')) parts.add('CO');
  if (capabilities.has('cloud')) {
    parts.add('华为云App');
  } else if (capabilities.has('wifi')) {
    parts.add(capabilities.has('web') ? '网页' : 'WiFi App');
  } else if (capabilities.has('ble')) {
    parts.add('蓝牙App');
  }
  return 'KQZL2-${capabilities.version.toString().padLeft(3, '0')} · ${parts.join(' + ')}';
}
