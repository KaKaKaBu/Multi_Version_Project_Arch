import 'dart:convert';

import 'package:flutter_test/flutter_test.dart';
import 'package:dcld_001/core/models/dcld_models.dart';
import 'package:dcld_001/core/protocol/dcld_protocol.dart';
import 'package:dcld_001/core/version/version_capabilities.dart';

void main() {
  test('resolves v7 capabilities and description', () {
    final capabilities = resolveUpperFeatures(version: 7);

    expect(capabilities.features, containsAll(['common', 'temp', 'voice', 'remote', 'wifi', 'app', 'camera']));
    expect(getSupportedMetrics(capabilities).map((item) => item.key), ['distance_cm', 'raw_distance_cm', 'temperature_c']);
    expect(describeVersion(capabilities), contains('DCLD-007'));
    expect(describeVersion(capabilities), contains('ESP32-CAM'));
  });

  test('builds protocol commands compatible with firmware JSON', () {
    expect(buildGetStatusCommand(), {'cmd': 'get_status'});
    expect(buildSetModeCommand(WorkMode.auto), {'cmd': 'set_mode', 'mode': 'auto'});
    expect(buildSetModeCommand(WorkMode.threshold), {'cmd': 'set_mode', 'mode': 'threshold'});
    expect(buildSetThresholdCommand(80), {'cmd': 'set_threshold', 'value': 80});
  });

  test('normalizes telemetry and calculates distance alarm', () {
    final capabilities = resolveUpperFeatures(version: 7);
    final thresholds = createDefaultThresholds().copyWithValue('distance', 80);
    final normalized = normalizeTelemetry(
      jsonEncode({
        'distance_cm': '55',
        'raw_distance_cm': 54,
        'temperature_c': 25.5,
        'threshold_cm': 80,
        'mode': 'threshold',
        'camera_url': 'http://192.168.4.1:81/stream',
      }),
      capabilities,
      thresholds: thresholds,
    );

    expect(normalized.telemetry['distance_cm'], 55);
    expect(normalized.telemetry['raw_distance_cm'], 54);
    expect(normalized.telemetry['temperature_c'], 25.5);
    expect(normalized.telemetry.mode, WorkMode.threshold);
    expect(normalized.telemetry.cameraUrl, 'http://192.168.4.1:81/stream');
    expect(normalized.telemetry.alarm, 1);
  });

  test('uses explicit alarm and updates thresholds from telemetry', () {
    final thresholds = normalizeThresholds({'thresholds': {'distance': 120}, 'threshold_cm': 90});
    final telemetry = normalizeTelemetry(
      {'distance_cm': 140, 'alarm': 1},
      resolveUpperFeatures(version: 4),
      thresholds: thresholds,
    );

    expect(thresholds['distance'], 90);
    expect(telemetry.telemetry.alarm, 1);
  });

  test('drops unsupported temperature from defaults', () {
    final telemetry = createDefaultTelemetry(resolveUpperFeatures(version: 1));

    expect(telemetry['distance_cm'], 0);
    expect(telemetry['raw_distance_cm'], 0);
    expect(telemetry['temperature_c'], isNull);
  });
}
