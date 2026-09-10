import 'package:mvp_flutter_common/mvp_flutter_common.dart';

export 'package:mvp_flutter_common/mvp_flutter_common.dart'
    show VersionCapabilities;

VersionCapabilities resolveProjectUpperFeatures() {
  return resolveUpperFeaturesWithDefaults(
    defaultVersion: '15',
    defaultFeatures: 'common,remote,wifi,cloud,app,light,message',
  );
}

String describeAqmjVersion(VersionCapabilities capabilities) {
  final parts = <String>['门铃', '人体检测', '设防撤防', '万年历', 'OLED'];
  if (capabilities.has('light')) parts.add('光敏感应灯');
  if (capabilities.has('message')) parts.add('语音留言');
  if (capabilities.has('ir')) parts.add('红外遥控');
  if (capabilities.has('camera')) parts.add('视频监控');
  if (capabilities.has('cloud')) {
    parts.add('华为云App');
  } else if (capabilities.has('wifi')) {
    parts.add('WiFi App');
  } else if (capabilities.has('ble')) {
    parts.add('蓝牙App');
  }
  return 'AQMJ-${capabilities.version.toString().padLeft(3, '0')} · ${parts.join(' + ')}';
}
