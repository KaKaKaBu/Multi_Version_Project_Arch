const compileVersion = typeof __UPPER_VERSION__ !== 'undefined' ? __UPPER_VERSION__ : '14'
const compileFeatureFlags = typeof __UPPER_FEATURE_FLAGS__ !== 'undefined' ? __UPPER_FEATURE_FLAGS__ : {}
const compileFeatures = typeof __UPPER_FEATURES__ !== 'undefined' ? __UPPER_FEATURES__ : []

export const versionFeatureMatrix = {
  1: ['common'],
  2: ['common', 'dht11'],
  3: ['common', 'dht11', 'remote', 'wifi', 'app'],
  4: ['common', 'dht11', 'remote', 'ble', 'app'],
  5: ['common', 'dht11', 'mq2'],
  6: ['common', 'dht11', 'mq2', 'remote', 'wifi', 'app'],
  7: ['common', 'dht11', 'mq2', 'remote', 'ble', 'app'],
  8: ['common', 'dht11', 'mq2', 'remote', 'wifi', 'aliyun', 'app'],
  9: ['common', 'dht11', 'mq2', 'remote', 'wifi', 'mpWeixin'],
  10: ['common', 'dht11', 'mq2', 'remote', 'wifi', 'web'],
  11: ['common', 'dht11', 'mq2', 'mq7'],
  12: ['common', 'dht11', 'mq2', 'mq7', 'remote', 'wifi', 'app'],
  13: ['common', 'dht11', 'mq2', 'mq7', 'remote', 'ble', 'app'],
  14: ['common', 'dht11', 'mq2', 'mq7', 'remote', 'wifi', 'aliyun', 'app'],
}

export const sensorCatalog = {
  pm25: { key: 'pm25', label: 'PM2.5', unit: 'μg/m³', thresholdKey: 'pm25', defaultThreshold: 100, step: 10, precision: 0, feature: 'common' },
  temp: { key: 'temp', label: '温度', unit: '°C', thresholdKey: 'temp', defaultThreshold: 35, step: 1, precision: 0, feature: 'dht11' },
  humidity: { key: 'humidity', label: '湿度', unit: '%', thresholdKey: 'humidity', defaultThreshold: 80, step: 5, precision: 0, feature: 'dht11' },
  smoke: { key: 'smoke', label: '烟雾', unit: 'ppm', thresholdKey: 'smoke', defaultThreshold: 500, step: 50, precision: 0, feature: 'mq2' },
  co: { key: 'co', label: '一氧化碳', unit: 'ppm', thresholdKey: 'co', defaultThreshold: 50, step: 5, precision: 0, feature: 'mq7' },
}

export const actuatorCatalog = {
  fan: { key: 'fan', label: '排风风扇', commandDevice: 'fan' },
  buzzer: { key: 'buzzer', label: '蜂鸣器', commandDevice: 'buzzer' },
  light: { key: 'light', label: '灯光报警', commandDevice: 'light' },
}

export const modeOptions = [
  { key: 'auto', label: '自动模式' },
  { key: 'manual', label: '手动模式' },
  { key: 'threshold', label: '阈值设置' },
]

function toFeatureFlags(features) {
  return features.reduce((acc, key) => {
    acc[key] = true
    return acc
  }, {})
}

export function resolveUpperVersion() {
  const value = Number(compileVersion || 14)
  if (Number.isInteger(value) && value >= 1 && value <= 14) {
    return value
  }
  return 14
}

export function resolveUpperFeatures(version = resolveUpperVersion()) {
  const fromCompile = Array.isArray(compileFeatures) && compileFeatures.length > 0 ? compileFeatures : []
  const matrixFeatures = versionFeatureMatrix[version] || versionFeatureMatrix[14]
  const merged = fromCompile.length > 0 ? fromCompile : matrixFeatures
  const flags = Object.keys(compileFeatureFlags || {}).length > 0 ? compileFeatureFlags : toFeatureFlags(merged)
  return { list: merged, flags }
}

export function hasFeature(feature, features = resolveUpperFeatures().flags) {
  return !!features[feature]
}

export function getSupportedSensors(features = resolveUpperFeatures().flags) {
  return Object.values(sensorCatalog).filter((item) => item.feature === 'common' || features[item.feature])
}

export function getSupportedThresholds(features = resolveUpperFeatures().flags) {
  return getSupportedSensors(features)
}

export function getSupportedActuators() {
  return Object.values(actuatorCatalog)
}

export function getSupportedTransports(features = resolveUpperFeatures().flags) {
  const transports = []
  if (features.wifi) transports.push({ key: 'mqtt', label: features.aliyun ? '阿里云 MQTT' : 'WiFi MQTT' })
  if (features.ble) transports.push({ key: 'ble', label: 'JDY-31 蓝牙' })
  return transports
}

export function describeVersion(version = resolveUpperVersion(), features = resolveUpperFeatures(version).flags) {
  const parts = ['PM2.5', '排风', '蜂鸣器', '灯光']
  if (features.dht11) parts.splice(1, 0, '温湿度')
  if (features.mq2) parts.splice(features.dht11 ? 2 : 1, 0, '烟雾')
  if (features.mq7) parts.splice(features.mq2 ? 3 : 1, 0, 'CO')
  if (features.wifi) parts.push(features.aliyun ? '阿里云' : features.mpWeixin ? '微信小程序' : features.web ? '网页' : 'WiFi App')
  if (features.ble) parts.push('蓝牙 App')
  return `KQZL3-${String(version).padStart(3, '0')} · ${parts.join(' + ')}`
}
