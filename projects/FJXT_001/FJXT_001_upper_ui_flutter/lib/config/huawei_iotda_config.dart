import 'package:mvp_flutter_common/mvp_flutter_common.dart';

export 'package:mvp_flutter_common/mvp_flutter_common.dart'
    show HuaweiIotdaConfig, HuaweiMqttCredential;

const defaultHuaweiIotdaConfig = HuaweiIotdaConfig(
  hostname: '7e87c47089.st1.iotda-device.cn-east-3.myhuaweicloud.com',
  websocketPath: '/mqtt',
  port: 443,
  appDeviceId: String.fromEnvironment(
    'HUAWEI_APP_DEVICE_ID',
    defaultValue: '6a476514e094d615924ed7ef_FJXT_app',
  ),
  appDeviceSecret: String.fromEnvironment(
    'HUAWEI_APP_DEVICE_SECRET',
    defaultValue: 'FJXT_app',
  ),
  stm32DeviceId: String.fromEnvironment(
    'HUAWEI_STM32_DEVICE_ID',
    defaultValue: '6a476514e094d615924ed7ef_FJXT_stm32',
  ),
  serviceId: String.fromEnvironment('HUAWEI_SERVICE_ID', defaultValue: 'FJXT'),
  appCustomPublishTopic: String.fromEnvironment(
    'HUAWEI_APP_PUB_TOPIC',
    defaultValue: '/jiabailie/M2M/FJXT_app/up',
  ),
  appCustomSubscribeTopic: String.fromEnvironment(
    'HUAWEI_APP_SUB_TOPIC',
    defaultValue: '/jiabailie/M2M/FJXT_app/down',
  ),
);
