import 'dart:async';
import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import 'telemetry_model.dart';

class AqmjService {
  AqmjService({required this.transport, required this.capabilities}) {
    _transportSub = transport.onMessage.listen(_onMessage);
  }

  final TransportService transport;
  final VersionCapabilities capabilities;
  final _dataController = StreamController<AqmjTelemetry>.broadcast();
  StreamSubscription<String>? _transportSub;

  AqmjTelemetry _latest = const AqmjTelemetry();

  Stream<AqmjTelemetry> get onData => _dataController.stream;
  AqmjTelemetry get latestData => _latest;

  void _onMessage(String raw) {
    try {
      final decoded = jsonDecode(raw);
      if (decoded is! Map<String, dynamic> ||
          !AqmjTelemetry.isTelemetryPayload(decoded)) {
        return;
      }
      _latest = AqmjTelemetry.fromJson(decoded, previous: _latest);
      _dataController.add(_latest);
    } catch (error) {
      if (upperUiShowCommunicationLog) {
        debugPrint('AqmjService parse error: $error');
      }
    }
  }

  void sendCommand(String cmd, [Map<String, dynamic>? fields]) {
    final msg = <String, dynamic>{'cmd': cmd};
    if (fields != null) msg.addAll(fields);
    final payload = jsonEncode(msg);
    transport.send(payload);
    if (upperUiShowCommunicationLog) {
      debugPrint('AqmjService send: $payload');
    }
  }

  void requestStatus() => sendCommand('get_status');
  void clearAlarm() => sendCommand('clear_alarm');
  void doorbell() => sendCommand('doorbell');
  void playMessage() => sendCommand('play_message');
  void recordStart() => sendCommand('record_start');
  void recordStop() => sendCommand('record_stop');

  void setArmed(bool value) => sendCommand('set_armed', {'armed': value ? 1 : 0});
  void setHome(bool value) => sendCommand('set_home', {'home': value ? 1 : 0});
  void setLamp(bool value) => sendCommand('set_lamp', {'lamp': value ? 1 : 0});

  void dispose() {
    _transportSub?.cancel();
    _dataController.close();
  }
}
