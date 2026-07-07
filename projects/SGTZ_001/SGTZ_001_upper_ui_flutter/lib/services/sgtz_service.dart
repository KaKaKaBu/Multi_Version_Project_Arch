import 'package:mvp_flutter_common/mvp_flutter_common.dart';
import 'dart:async';
import 'dart:convert';

import 'package:flutter/foundation.dart';

import 'telemetry_model.dart';

class SgtzService {
  SgtzService({required this.transport, required this.capabilities}) {
    _transportSub = transport.onMessage.listen(_onMessage);
  }

  final TransportService transport;
  final VersionCapabilities capabilities;
  final _dataController = StreamController<ScaleTelemetry>.broadcast();
  StreamSubscription<String>? _transportSub;

  ScaleTelemetry _latest = const ScaleTelemetry();
  Stream<ScaleTelemetry> get onData => _dataController.stream;
  ScaleTelemetry get latestData => _latest;

  void _onMessage(String raw) {
    try {
      final decoded = jsonDecode(raw);
      if (decoded is! Map<String, dynamic> ||
          !ScaleTelemetry.isTelemetryPayload(decoded)) {
        return;
      }
      _latest = ScaleTelemetry.fromJson(decoded, previous: _latest);
      _dataController.add(_latest);
    } catch (error) {
      if (upperUiShowCommunicationLog) {
        debugPrint('SgtzService parse error: $error');
      }
    }
  }

  void sendCommand(String cmd, [Map<String, dynamic>? params]) {
    final msg = <String, dynamic>{'cmd': cmd};
    if (params != null && params.isNotEmpty) msg['params'] = params;
    final payload = jsonEncode(msg);
    transport.send(payload);
    if (upperUiShowCommunicationLog) {
      debugPrint('SgtzService send: $payload');
    }
  }

  void requestStatus() => sendCommand('get_status');
  void lock() => sendCommand('lock');
  void unlock() => sendCommand('unlock');
  void tare() => sendCommand('tare');
  void fanOn() => sendCommand('fan_on');
  void fanOff() => sendCommand('fan_off');
  void voiceToggle() => sendCommand('voice_toggle');
  void setThreshold(int thresholdG) =>
      sendCommand('set_threshold', {'threshold_g': thresholdG});

  void dispose() {
    _transportSub?.cancel();
    _dataController.close();
  }
}
