import { actuatorCatalog, getSupportedSensors, sensorCatalog } from '@/config/versionCapabilities'

export function buildGetStatusCommand() {
  return { cmd: 'get_status' }
}

export function buildSetModeCommand(mode) {
  return { cmd: 'set_mode', mode }
}

export function buildSetDeviceCommand(device, state) {
  return { cmd: 'set_device', device, state: state ? 1 : 0 }
}

export function buildSetThresholdCommand(sensor, value) {
  return { cmd: 'set_threshold', sensor, value: Number(value) }
}

export function createDefaultTelemetry(features) {
  const telemetry = {
    pm25: 0,
    temp: 0,
    humidity: 0,
    smoke: 0,
    co: 0,
    fan: 0,
    buzzer: 0,
    light: 0,
    mode: 'auto',
    alarm: 0,
  }

  for (const sensor of Object.values(sensorCatalog)) {
    if (sensor.feature !== 'common' && !features[sensor.feature]) {
      telemetry[sensor.key] = null
    }
  }

  return telemetry
}

export function createDefaultThresholds(features) {
  return getSupportedSensors(features).reduce((acc, sensor) => {
    acc[sensor.thresholdKey] = sensor.defaultThreshold
    return acc
  }, {})
}

function toNumber(value, fallback = 0) {
  const number = Number(value)
  return Number.isFinite(number) ? number : fallback
}

function toSwitch(value) {
  return value ? 1 : 0
}

function normalizeMode(mode) {
  return ['auto', 'manual', 'threshold'].includes(mode) ? mode : 'auto'
}

export function parseTelemetry(payload) {
  if (payload == null) {
    return {}
  }
  if (typeof payload === 'object' && !(payload instanceof ArrayBuffer)) {
    return payload
  }
  const text = payload instanceof ArrayBuffer ? arrayBufferToString(payload) : String(payload)
  try {
    return JSON.parse(text)
  } catch (error) {
    return { raw: text }
  }
}

export function normalizeTelemetry(payload, features, previous = createDefaultTelemetry(features), thresholds = createDefaultThresholds(features)) {
  const raw = parseTelemetry(payload)
  const next = { ...previous }

  for (const sensor of getSupportedSensors(features)) {
    if (raw[sensor.key] !== undefined) {
      next[sensor.key] = toNumber(raw[sensor.key], previous[sensor.key] || 0)
    }
  }

  for (const actuator of Object.values(actuatorCatalog)) {
    if (raw[actuator.key] !== undefined) {
      next[actuator.key] = toSwitch(raw[actuator.key])
    }
  }

  if (raw.mode !== undefined) {
    next.mode = normalizeMode(raw.mode)
  }

  next.alarm = raw.alarm !== undefined ? toSwitch(raw.alarm) : calculateAlarm(next, thresholds, features)
  return { telemetry: next, raw }
}

export function normalizeThresholds(payload, features, previous = createDefaultThresholds(features)) {
  const raw = parseTelemetry(payload)
  const source = raw.thresholds && typeof raw.thresholds === 'object' ? raw.thresholds : raw
  const next = { ...previous }

  for (const sensor of getSupportedSensors(features)) {
    const key = sensor.thresholdKey
    if (source[key] !== undefined) {
      next[key] = toNumber(source[key], previous[key] ?? sensor.defaultThreshold)
    }
  }

  return next
}

export function calculateAlarm(telemetry, thresholds, features) {
  return getSupportedSensors(features).some((sensor) => {
    const value = telemetry[sensor.key]
    const threshold = thresholds[sensor.thresholdKey]
    return value != null && threshold != null && Number(value) > Number(threshold)
  }) ? 1 : 0
}

export function serializeCommand(command) {
  return JSON.stringify(command)
}

export function arrayBufferToString(buffer) {
  if (typeof TextDecoder !== 'undefined') {
    return new TextDecoder('utf-8').decode(buffer)
  }
  const bytes = new Uint8Array(buffer)
  return Array.from(bytes).map((item) => String.fromCharCode(item)).join('')
}

export function stringToArrayBuffer(text) {
  if (typeof TextEncoder !== 'undefined') {
    return new TextEncoder().encode(text).buffer
  }
  const buffer = new ArrayBuffer(text.length)
  const bytes = new Uint8Array(buffer)
  for (let i = 0; i < text.length; i += 1) {
    bytes[i] = text.charCodeAt(i) & 0xff
  }
  return buffer
}
