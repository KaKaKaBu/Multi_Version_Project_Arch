import 'dart:convert';
import 'dart:typed_data';

import '../models/dcld_models.dart';
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

Map<String, dynamic> buildSetThresholdCommand(num value) => {
  'cmd': 'set_threshold',
  'value': value,
};

TelemetryState createDefaultTelemetry(VersionCapabilities capabilities) {
  final values = <String, num?>{
    'distance_cm': 0,
    'raw_distance_cm': 0,
    'temperature_c': capabilities.has('temp') ? 20 : null,
    'threshold_cm': 80,
  };

  for (final metric in getSupportedMetrics(capabilities)) {
    values.putIfAbsent(metric.key, () => null);
  }

  return TelemetryState(values: values, mode: WorkMode.auto, alarm: 0, cameraUrl: '');
}

ThresholdState createDefaultThresholds() {
  return ThresholdState({
    for (final threshold in getSupportedThresholds()) threshold.key: threshold.defaultValue,
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
  final thresholdBase = thresholds ?? createDefaultThresholds();
  final nextValues = Map<String, num?>.from(base.values);

  for (final metric in getSupportedMetrics(capabilities)) {
    if (raw.containsKey(metric.key)) {
      nextValues[metric.key] = _toNumber(raw[metric.key], base[metric.key] ?? 0);
    }
  }

  if (raw.containsKey('threshold_cm')) {
    nextValues['threshold_cm'] = _toNumber(raw['threshold_cm'], base['threshold_cm'] ?? 80);
  }

  final nextMode = raw.containsKey('mode') ? WorkMode.fromKey(raw['mode']) : base.mode;
  final nextCameraUrl = raw.containsKey('camera_url') ? (raw['camera_url']?.toString() ?? '') : base.cameraUrl;
  final next = TelemetryState(
    values: nextValues,
    mode: nextMode,
    alarm: raw.containsKey('alarm')
        ? _toSwitch(raw['alarm'])
        : calculateAlarm(
            TelemetryState(values: nextValues, mode: nextMode, alarm: base.alarm, cameraUrl: nextCameraUrl),
            thresholdBase,
          ),
    cameraUrl: nextCameraUrl,
  );

  return NormalizedTelemetry(telemetry: next, raw: raw);
}

ThresholdState normalizeThresholds(Object? payload, {ThresholdState? previous}) {
  final raw = parseTelemetry(payload);
  final source = raw['thresholds'] is Map
      ? Map<String, dynamic>.from(raw['thresholds'] as Map)
      : raw;
  final base = previous ?? createDefaultThresholds();
  final next = Map<String, num>.from(base.values);

  if (source.containsKey('distance')) {
    next['distance'] = _toNumber(source['distance'], base['distance'] ?? 80);
  }
  if (raw.containsKey('threshold_cm')) {
    next['distance'] = _toNumber(raw['threshold_cm'], base['distance'] ?? 80);
  }

  return ThresholdState(next);
}

int calculateAlarm(TelemetryState telemetry, ThresholdState thresholds) {
  final distance = telemetry['distance_cm'];
  final threshold = thresholds['distance'];
  if (distance == null || threshold == null) {
    return 0;
  }
  return distance > 0 && distance < threshold ? 1 : 0;
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
