export const aliyunMqttConfig = {
  productKey: '',
  deviceName: '',
  regionId: 'cn-shanghai',
  wsEndpoint: '',
  commandTopicTemplate: '/sys/{productKey}/{deviceName}/thing/service/property/set',
  telemetryTopicTemplate: '/sys/{productKey}/{deviceName}/thing/event/property/post',
}

export function resolveAliyunTopic(template, productKey = aliyunMqttConfig.productKey, deviceName = aliyunMqttConfig.deviceName) {
  return template.replace('{productKey}', productKey).replace('{deviceName}', deviceName)
}
