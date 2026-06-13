import 'dart:convert';
import 'dart:typed_data';

import '../models/kqzl3_models.dart';
import '../version/version_capabilities.dart';

class NormalizedTelemetry {
  const NormalizedTelemetry({required this.telemetry, required this.raw});

  final TelemetryState telemetry;
  final Map<String, dynamic> raw;
}

Map<String, dynamic> buildGetStatusCommand() => {'cmd': 'get_status'};

Map<String, dynamic> buildSetModeCommand(WorkMode mode) => {
  'cmd': 'set_mode',
  'mode': mode.key,
};

Map<String, dynamic> buildSetDeviceCommand(String device, Object? state) => {
  'cmd': 'set_device',
  'device': device,
  'state': _toSwitch(state),
};

Map<String, dynamic> buildSetThresholdCommand(String sensor, num value) => {
  'cmd': 'set_threshold',
  'sensor': sensor,
  'value': value,
};

TelemetryState createDefaultTelemetry(VersionCapabilities capabilities) {
  final values = <String, num?>{
    'pm25': 0,
    'temp': 0,
    'humidity': 0,
    'smoke': 0,
    'co': 0,
    'fan': 0,
    'buzzer': 0,
    'light': 0,
  };

  for (final sensor in sensorCatalog) {
    if (sensor.feature != 'common' && !capabilities.has(sensor.feature)) {
      values[sensor.key] = null;
    }
  }

  return TelemetryState(values: values, mode: WorkMode.auto, alarm: 0);
}

ThresholdState createDefaultThresholds(VersionCapabilities capabilities) {
  return ThresholdState({
    for (final sensor in getSupportedSensors(capabilities))
      sensor.thresholdKey: sensor.defaultThreshold,
  });
}

Map<String, dynamic> parseTelemetry(Object? payload) {
  if (payload == null) {
    return <String, dynamic>{};
  }
  if (payload is Map) {
    return Map<String, dynamic>.from(payload);
  }

  final text = payload is Uint8List
      ? utf8.decode(payload)
      : payload is ByteBuffer
          ? utf8.decode(payload.asUint8List())
          : payload.toString();
  try {
    final parsed = jsonDecode(text);
    if (parsed is Map) {
      return Map<String, dynamic>.from(parsed);
    }
    return {'raw': parsed};
  } catch (_) {
    return {'raw': text};
  }
}

NormalizedTelemetry normalizeTelemetry(
  Object? payload,
  VersionCapabilities capabilities, {
  TelemetryState? previous,
  ThresholdState? thresholds,
}) {
  final raw = parseTelemetry(payload);
  final base = previous ?? createDefaultTelemetry(capabilities);
  final thresholdBase = thresholds ?? createDefaultThresholds(capabilities);
  final nextValues = Map<String, num?>.from(base.values);

  for (final sensor in getSupportedSensors(capabilities)) {
    if (raw.containsKey(sensor.key)) {
      nextValues[sensor.key] = _toNumber(raw[sensor.key], base[sensor.key] ?? 0);
    }
  }

  for (final actuator in actuatorCatalog) {
    if (raw.containsKey(actuator.key)) {
      nextValues[actuator.key] = _toSwitch(raw[actuator.key]);
    }
  }

  final nextMode = raw.containsKey('mode') ? WorkMode.fromKey(raw['mode']) : base.mode;
  final next = TelemetryState(
    values: nextValues,
    mode: nextMode,
    alarm: raw.containsKey('alarm')
        ? _toSwitch(raw['alarm'])
        : calculateAlarm(
            TelemetryState(values: nextValues, mode: nextMode, alarm: base.alarm),
            thresholdBase,
            capabilities,
          ),
  );

  return NormalizedTelemetry(telemetry: next, raw: raw);
}

ThresholdState normalizeThresholds(
  Object? payload,
  VersionCapabilities capabilities, {
  ThresholdState? previous,
}) {
  final raw = parseTelemetry(payload);
  final source = raw['thresholds'] is Map
      ? Map<String, dynamic>.from(raw['thresholds'] as Map)
      : raw;
  final base = previous ?? createDefaultThresholds(capabilities);
  final next = Map<String, num>.from(base.values);

  for (final sensor in getSupportedSensors(capabilities)) {
    final key = sensor.thresholdKey;
    if (source.containsKey(key)) {
      next[key] = _toNumber(source[key], base[key] ?? sensor.defaultThreshold);
    }
  }

  return ThresholdState(next);
}

int calculateAlarm(
  TelemetryState telemetry,
  ThresholdState thresholds,
  VersionCapabilities capabilities,
) {
  for (final sensor in getSupportedSensors(capabilities)) {
    final value = telemetry[sensor.key];
    final threshold = thresholds[sensor.thresholdKey];
    if (value != null && threshold != null && value > threshold) {
      return 1;
    }
  }
  return 0;
}

String serializeCommand(Map<String, dynamic> command) => jsonEncode(command);

String bytesToString(Uint8List bytes) => utf8.decode(bytes);

Uint8List stringToBytes(String text) => Uint8List.fromList(utf8.encode(text));

num _toNumber(Object? value, num fallback) {
  if (value is num) {
    return value.isFinite ? value : fallback;
  }
  final parsed = num.tryParse(value?.toString() ?? '');
  return parsed == null || !parsed.isFinite ? fallback : parsed;
}

int _toSwitch(Object? value) {
  if (value is bool) {
    return value ? 1 : 0;
  }
  if (value is num) {
    return value == 0 ? 0 : 1;
  }
  final text = value?.toString().toLowerCase();
  return text == null || text == '0' || text == 'false' || text.isEmpty ? 0 : 1;
}
