import 'package:mvp_flutter_common/mvp_flutter_common.dart';

export 'package:mvp_flutter_common/mvp_flutter_common.dart'
    show VersionCapabilities;

VersionCapabilities resolveProjectUpperFeatures() {
  return resolveUpperFeaturesWithDefaults(
    defaultVersion: '24',
    defaultFeatures: 'common,servo,hx711,tts,dht11,remote,wifi,cloud,app',
  );
}

String describeYyyhVersion(VersionCapabilities capabilities) {
  final parts = <String>['定时提醒', '吃药检测', '药品分类', '药量显示', 'OLED'];
  if (capabilities.has('servo')) parts.add('舵机开盒');
  if (capabilities.has('hx711')) parts.add('剩余药量称重');
  if (capabilities.has('tts')) parts.add('TTS语音');
  if (capabilities.has('dht11')) parts.add('温湿度');
  if (capabilities.has('camera')) parts.add('视频监控');
  if (capabilities.has('cloud')) {
    parts.add('华为云App');
  } else if (capabilities.has('wifi')) {
    parts.add('WiFi App');
  } else if (capabilities.has('ble')) {
    parts.add('蓝牙App');
  }
  return 'YYYH-${capabilities.version.toString().padLeft(3, '0')} · ${parts.join(' + ')}';
}
