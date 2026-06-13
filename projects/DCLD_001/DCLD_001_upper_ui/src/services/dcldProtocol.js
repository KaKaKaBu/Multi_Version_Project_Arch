import { getSupportedMetrics, getSupportedThresholds } from '@/config/versionCapabilities'

export function buildGetStatusCommand() {
  return { cmd: 'get_status' }
}

export function buildSetModeCommand(mode) {
  return { cmd: 'set_mode', mode }
}

export function buildSetThresholdCommand(value) {
  return { cmd: 'set_threshold', value: Number(value) }
}

export function createDefaultTelemetry(features) {
  const telemetry = {
    distance_cm: 0,
    raw_distance_cm: 0,
    temperature_c: features.temp ? 20 : null,
    threshold_cm: 80,
    mode: 'auto',
    alarm: 0,
    camera_url: '',
  }

  for (const metric of getSupportedMetrics(features)) {
    if (telemetry[metric.key] === undefined) {
      telemetry[metric.key] = null
    }
  }

  return telemetry
}

export function createDefaultThresholds() {
  return getSupportedThresholds().reduce((acc, threshold) => {
    acc[threshold.key] = threshold.defaultThreshold
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
  return ['auto', 'threshold'].includes(mode) ? mode : 'auto'
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

export function normalizeTelemetry(payload, features, previous = createDefaultTelemetry(features), thresholds = createDefaultThresholds()) {
  const raw = parseTelemetry(payload)
  const next = { ...previous }

  for (const metric of getSupportedMetrics(features)) {
    if (raw[metric.key] !== undefined) {
      next[metric.key] = toNumber(raw[metric.key], previous[metric.key] || 0)
    }
  }

  if (raw.distance_cm !== undefined) {
    next.distance_cm = toNumber(raw.distance_cm, previous.distance_cm || 0)
  }
  if (raw.raw_distance_cm !== undefined) {
    next.raw_distance_cm = toNumber(raw.raw_distance_cm, previous.raw_distance_cm || 0)
  }
  if (raw.temperature_c !== undefined) {
    next.temperature_c = toNumber(raw.temperature_c, previous.temperature_c || 20)
  }
  if (raw.threshold_cm !== undefined) {
    next.threshold_cm = toNumber(raw.threshold_cm, previous.threshold_cm || 80)
  }
  if (raw.camera_url !== undefined) {
    next.camera_url = String(raw.camera_url || '')
  }
  if (raw.mode !== undefined) {
    next.mode = normalizeMode(raw.mode)
  }

  next.alarm = raw.alarm !== undefined ? toSwitch(raw.alarm) : calculateAlarm(next, thresholds)
  return { telemetry: next, raw }
}

export function normalizeThresholds(payload, previous = createDefaultThresholds()) {
  const raw = parseTelemetry(payload)
  const source = raw.thresholds && typeof raw.thresholds === 'object' ? raw.thresholds : raw
  const next = { ...previous }

  if (source.distance !== undefined) {
    next.distance = toNumber(source.distance, previous.distance ?? 80)
  }
  if (raw.threshold_cm !== undefined) {
    next.distance = toNumber(raw.threshold_cm, previous.distance ?? 80)
  }

  return next
}

export function calculateAlarm(telemetry, thresholds) {
  const distance = Number(telemetry.distance_cm)
  const threshold = Number(thresholds.distance)
  return Number.isFinite(distance) && Number.isFinite(threshold) && distance > 0 && distance < threshold ? 1 : 0
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
