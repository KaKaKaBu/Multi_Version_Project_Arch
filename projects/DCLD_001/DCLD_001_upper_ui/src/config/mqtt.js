export const boardMqttConfig = {
  brokerHost: '121.40.131.194',
  brokerPort: 1883,
  username: 'yskj',
  password: 'yskj@123',
  deviceClientId: 'DCLD_001',
  panelClientIdPrefix: 'DCLD_001_panel',
  commandTopic: 'DCLD_001',
  telemetryTopic: 'DCLD_001/web',
  keepalive: 60,
  useTLS: false,
  wsEndpoint: '',
}

export function buildClientId(suffix = 'web') {
  const base = boardMqttConfig.panelClientIdPrefix || boardMqttConfig.deviceClientId
  return `${base}_${suffix}_${Date.now().toString(16)}`
}
