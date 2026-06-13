const compileVersion = typeof __UPPER_VERSION__ !== 'undefined' ? __UPPER_VERSION__ : '7'
const compileFeatureFlags = typeof __UPPER_FEATURE_FLAGS__ !== 'undefined' ? __UPPER_FEATURE_FLAGS__ : {}
const compileFeatures = typeof __UPPER_FEATURES__ !== 'undefined' ? __UPPER_FEATURES__ : []

export const versionFeatureMatrix = {
  1: ['common'],
  2: ['common', 'temp'],
  3: ['common', 'temp', 'voice'],
  4: ['common', 'temp', 'remote', 'wifi', 'app'],
  5: ['common', 'temp', 'remote', 'ble', 'app'],
  6: ['common', 'temp', 'voice', 'remote', 'wifi', 'app'],
  7: ['common', 'temp', 'voice', 'remote', 'wifi', 'app', 'camera'],
}

export const metricCatalog = {
  distance_cm: { key: 'distance_cm', label: '当前距离', unit: 'cm', thresholdKey: 'distance', defaultThreshold: 80, step: 5, precision: 0, feature: 'common' },
  raw_distance_cm: { key: 'raw_distance_cm', label: '原始距离', unit: 'cm', thresholdKey: 'distance', defaultThreshold: 80, step: 5, precision: 0, feature: 'common' },
  temperature_c: { key: 'temperature_c', label: '环境温度', unit: '°C', thresholdKey: null, defaultThreshold: null, step: 1, precision: 1, feature: 'temp' },
}

export const thresholdCatalog = {
  distance: { key: 'distance', label: '报警阈值', unit: 'cm', defaultThreshold: 80, step: 5, min: 20, max: 400 },
}

export const modeOptions = [
  { key: 'auto', label: '自动模式' },
  { key: 'threshold', label: '阈值设置' },
]

function toFeatureFlags(features) {
  return features.reduce((acc, key) => {
    acc[key] = true
    return acc
  }, {})
}

export function resolveUpperVersion() {
  const value = Number(compileVersion || 7)
  if (Number.isInteger(value) && value >= 1 && value <= 7) {
    return value
  }
  return 7
}

export function resolveUpperFeatures(version = resolveUpperVersion()) {
  const fromCompile = Array.isArray(compileFeatures) && compileFeatures.length > 0 ? compileFeatures : []
  const matrixFeatures = versionFeatureMatrix[version] || versionFeatureMatrix[7]
  const merged = fromCompile.length > 0 ? fromCompile : matrixFeatures
  const flags = Object.keys(compileFeatureFlags || {}).length > 0 ? compileFeatureFlags : toFeatureFlags(merged)
  return { list: merged, flags }
}

export function hasFeature(feature, features = resolveUpperFeatures().flags) {
  return !!features[feature]
}

export function getSupportedMetrics(features = resolveUpperFeatures().flags) {
  return Object.values(metricCatalog).filter((item) => item.feature === 'common' || features[item.feature])
}

export function getSupportedThresholds() {
  return Object.values(thresholdCatalog)
}

export function getSupportedTransports(features = resolveUpperFeatures().flags) {
  const transports = []
  if (features.wifi) transports.push({ key: 'mqtt', label: 'WiFi MQTT' })
  if (features.ble) transports.push({ key: 'ble', label: 'JDY-31 蓝牙' })
  return transports
}

export function describeVersion(version = resolveUpperVersion(), features = resolveUpperFeatures(version).flags) {
  const parts = ['超声波测距', 'OLED', '声光报警', '阈值设置']
  if (features.temp) parts.push('温度补偿')
  if (features.voice) parts.push('语音播报')
  if (features.wifi) parts.push('WiFi App')
  if (features.ble) parts.push('蓝牙 App')
  if (features.camera) parts.push('ESP32-CAM')
  return `DCLD-${String(version).padStart(3, '0')} · ${parts.join(' + ')}`
}
