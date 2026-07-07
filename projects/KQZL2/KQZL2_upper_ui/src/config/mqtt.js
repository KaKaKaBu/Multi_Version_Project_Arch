export const boardMqttConfig = {
  brokerHost: '121.40.131.194',
  brokerPort: 1883,
  username: 'yskj',
  password: 'yskj@123',
  deviceClientId: 'KQZL2',
  panelClientIdPrefix: 'KQZL2_panel',
  commandTopic: 'KQZL2',
  telemetryTopic: 'KQZL2/web',
  keepalive: 60,
  useTLS: false,
  wsEndpoint: '',
}

export function buildClientId(suffix = 'web') {
  const base = boardMqttConfig.panelClientIdPrefix || boardMqttConfig.deviceClientId
  return `${base}_${suffix}_${Date.now().toString(16)}`
}

