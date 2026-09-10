import 'package:mvp_flutter_common/mvp_flutter_common.dart';

export 'package:mvp_flutter_common/mvp_flutter_common.dart'
    show HuaweiIotdaConfig, HuaweiMqttCredential;

const defaultHuaweiIotdaConfig = HuaweiIotdaConfig(
  hostname: '7e87c47089.st1.iotda-device.cn-east-3.myhuaweicloud.com',
  websocketPath: '/mqtt',
  port: 443,
  appDeviceId: String.fromEnvironment(
    'HUAWEI_APP_DEVICE_ID',
    defaultValue: '6a476514e094d615924ed7ef_AQMJ_app',
  ),
  appDeviceSecret: String.fromEnvironment(
    'HUAWEI_APP_DEVICE_SECRET',
    defaultValue: 'AQMJ_app',
  ),
  stm32DeviceId: String.fromEnvironment(
    'HUAWEI_STM32_DEVICE_ID',
    defaultValue: '6a476514e094d615924ed7ef_AQMJ_stm32',
  ),
  serviceId: String.fromEnvironment('HUAWEI_SERVICE_ID', defaultValue: 'AQMJ'),
  appCustomPublishTopic: String.fromEnvironment(
    'HUAWEI_APP_PUB_TOPIC',
    defaultValue: '/jiabailie/M2M/AQMJ_app/up',
  ),
  appCustomSubscribeTopic: String.fromEnvironment(
    'HUAWEI_APP_SUB_TOPIC',
    defaultValue: '/jiabailie/M2M/AQMJ_app/down',
  ),
);
