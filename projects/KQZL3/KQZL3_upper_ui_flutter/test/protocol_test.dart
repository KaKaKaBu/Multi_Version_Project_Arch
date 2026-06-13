import 'dart:convert';

import 'package:flutter_test/flutter_test.dart';
import 'package:kqzl3/core/models/kqzl3_models.dart';
import 'package:kqzl3/core/protocol/kqzl3_protocol.dart';
import 'package:kqzl3/core/version/version_capabilities.dart';

void main() {
  test('resolves v14 capabilities and description', () {
    final capabilities = resolveUpperFeatures(version: 14);

    expect(capabilities.features, containsAll(['common', 'dht11', 'mq2', 'mq7', 'remote', 'wifi', 'aliyun', 'app']));
    expect(getSupportedSensors(capabilities).map((item) => item.key), ['pm25', 'temp', 'humidity', 'smoke', 'co']);
    expect(describeVersion(capabilities), contains('KQZL3-014'));
    expect(describeVersion(capabilities), contains('阿里云'));
  });

  test('builds protocol commands compatible with firmware JSON', () {
    expect(buildGetStatusCommand(), {'cmd': 'get_status'});
    expect(buildSetModeCommand(WorkMode.manual), {'cmd': 'set_mode', 'mode': 'manual'});
    expect(buildSetDeviceCommand('fan', true), {'cmd': 'set_device', 'device': 'fan', 'state': 1});
    expect(buildSetThresholdCommand('pm25', 120), {'cmd': 'set_threshold', 'sensor': 'pm25', 'value': 120});
  });

  test('normalizes telemetry and calculates alarm', () {
    final capabilities = resolveUpperFeatures(version: 14);
    final thresholds = createDefaultThresholds(capabilities).copyWithValue('pm25', 100);
    final normalized = normalizeTelemetry(
      jsonEncode({'pm25': '135', 'temp': 28, 'fan': true, 'mode': 'manual'}),
      capabilities,
      thresholds: thresholds,
    );

    expect(normalized.telemetry['pm25'], 135);
    expect(normalized.telemetry['fan'], 1);
    expect(normalized.telemetry.mode, WorkMode.manual);
    expect(normalized.telemetry.alarm, 1);
  });

  test('drops unsupported sensor values from defaults', () {
    final capabilities = resolveUpperFeatures(version: 4);
    final telemetry = createDefaultTelemetry(capabilities);

    expect(telemetry['pm25'], 0);
    expect(telemetry['temp'], 0);
    expect(telemetry['smoke'], isNull);
    expect(telemetry['co'], isNull);
  });
}
