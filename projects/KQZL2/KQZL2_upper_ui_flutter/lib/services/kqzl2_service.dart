import 'dart:async';
import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import 'telemetry_model.dart';

class Kqzl2Service {
  Kqzl2Service({required this.transport, required this.capabilities}) {
    _transportSub = transport.onMessage.listen(_onMessage);
  }

  final TransportService transport;
  final VersionCapabilities capabilities;
  final _dataController = StreamController<AirQualityTelemetry>.broadcast();
  StreamSubscription<String>? _transportSub;

  AirQualityTelemetry _latest = AirQualityTelemetry(
    thresholds: AirQualityTelemetry.defaultThresholds(),
  );

  Stream<AirQualityTelemetry> get onData => _dataController.stream;
  AirQualityTelemetry get latestData => _latest;

  void _onMessage(String raw) {
    try {
      final decoded = jsonDecode(raw);
      if (decoded is! Map<String, dynamic> ||
          !AirQualityTelemetry.isTelemetryPayload(decoded)) {
        return;
      }
      _latest = AirQualityTelemetry.fromJson(decoded, previous: _latest);
      _dataController.add(_latest);
    } catch (error) {
      if (upperUiShowCommunicationLog) {
        debugPrint('Kqzl2Service parse error: $error');
      }
    }
  }

  void sendCommand(String cmd, [Map<String, dynamic>? fields]) {
    final msg = <String, dynamic>{'cmd': cmd};
    if (fields != null) msg.addAll(fields);
    final payload = jsonEncode(msg);
    transport.send(payload);
    if (upperUiShowCommunicationLog) {
      debugPrint('Kqzl2Service send: $payload');
    }
  }

  void requestStatus() => sendCommand('get_status');
  void setMode(String mode) => sendCommand('set_mode', {'mode': mode});
  void setDevice(String device, bool state) =>
      sendCommand('set_device', {'device': device, 'state': state ? 1 : 0});
  void setThreshold(String sensor, int value) =>
      sendCommand('set_threshold', {'sensor': sensor, 'value': value});

  void dispose() {
    _transportSub?.cancel();
    _dataController.close();
  }
}
