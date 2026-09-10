import 'dart:async';
import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import 'telemetry_model.dart';

class YyyhService {
  YyyhService({required this.transport, required this.capabilities}) {
    _transportSub = transport.onMessage.listen(_onMessage);
  }

  final TransportService transport;
  final VersionCapabilities capabilities;
  final _dataController = StreamController<YyyhTelemetry>.broadcast();
  StreamSubscription<String>? _transportSub;

  YyyhTelemetry _latest = const YyyhTelemetry();

  Stream<YyyhTelemetry> get onData => _dataController.stream;
  YyyhTelemetry get latestData => _latest;

  void _onMessage(String raw) {
    try {
      final decoded = jsonDecode(raw);
      if (decoded is! Map<String, dynamic> ||
          !YyyhTelemetry.isTelemetryPayload(decoded)) {
        return;
      }
      _latest = YyyhTelemetry.fromJson(decoded, previous: _latest);
      _dataController.add(_latest);
    } catch (error) {
      if (upperUiShowCommunicationLog) {
        debugPrint('YyyhService parse error: $error');
      }
    }
  }

  void sendCommand(String cmd, [Map<String, dynamic>? fields]) {
    final msg = <String, dynamic>{'cmd': cmd};
    if (fields != null) msg.addAll(fields);
    final payload = jsonEncode(msg);
    transport.send(payload);
    if (upperUiShowCommunicationLog) {
      debugPrint('YyyhService send: $payload');
    }
  }

  void requestStatus() => sendCommand('get_status');
  void openBox() => sendCommand('open_box');
  void closeBox() => sendCommand('close_box');
  void ackTake() => sendCommand('ack_take');

  void setTime(DateTime time) => sendCommand('set_time', {
        'year': time.year,
        'month': time.month,
        'day': time.day,
        'hour': time.hour,
        'minute': time.minute,
        'second': time.second,
      });

  void setTimer(MedicineTimer timer) => sendCommand('set_timer', {
        'index': timer.index,
        'enabled': timer.enabled ? 1 : 0,
        'hour': timer.hour,
        'minute': timer.minute,
        'dose': timer.dose,
        'category': timer.category,
      });

  void setCount(int index, int value) => sendCommand('set_count', {
        'index': index,
        'value': value,
      });

  void dispose() {
    _transportSub?.cancel();
    _dataController.close();
  }
}
