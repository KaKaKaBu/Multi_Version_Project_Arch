import 'package:mvp_flutter_common/mvp_flutter_common.dart';
import 'dart:async';
import 'dart:convert';

import 'package:flutter/foundation.dart';

import 'telemetry_model.dart';

class CommandLogEntry {
  const CommandLogEntry({
    required this.direction,
    required this.payload,
    required this.timestamp,
  });

  final String direction;
  final String payload;
  final DateTime timestamp;
}

class ZnczService {
  ZnczService({required this.transport}) {
    _transportSub = transport.onMessage.listen(_onMessage);
  }

  final TransportService transport;
  final _dataController = StreamController<SocketTelemetry>.broadcast();
  final _logController = StreamController<List<CommandLogEntry>>.broadcast();
  StreamSubscription<String>? _transportSub;
  final List<CommandLogEntry> _logs = <CommandLogEntry>[];
  SocketTelemetry _latest = const SocketTelemetry();

  Stream<SocketTelemetry> get onData => _dataController.stream;
  Stream<List<CommandLogEntry>> get onLogs => _logController.stream;
  SocketTelemetry get latestData => _latest;
  List<CommandLogEntry> get logs => List.unmodifiable(_logs);
  bool get isConnected => transport.isConnected;

  void _onMessage(String raw) {
    _pushLog('RX', raw);
    try {
      final decoded = jsonDecode(raw);
      if (decoded is! Map<String, dynamic> ||
          !SocketTelemetry.isTelemetryPayload(decoded)) {
        return;
      }
      _latest = SocketTelemetry.fromJson(decoded, previous: _latest);
      _dataController.add(_latest);
    } catch (error) {
      if (upperUiShowCommunicationLog) {
        debugPrint('ZnczService parse error: $error');
      }
    }
  }

  Future<void> sendCommand(String cmd, [Map<String, dynamic>? fields]) async {
    final payload = <String, dynamic>{'cmd': cmd};
    if (fields != null) payload.addAll(fields);
    final text = jsonEncode(payload);
    _pushLog('TX', text);
    await transport.send(text);
    if (upperUiShowCommunicationLog) {
      debugPrint('ZnczService send: $text');
    }
  }

  Future<void> requestStatus() => sendCommand('get_status');

  Future<void> setRelay(bool on) =>
      sendCommand('relay', <String, dynamic>{'state': on ? 1 : 0});

  Future<void> setOnTime(TimeParts time) =>
      sendCommand('set_on_time', time.toJson());

  Future<void> setOffTime(TimeParts time) =>
      sendCommand('set_off_time', time.toJson());

  Future<void> setDeviceTime(DateTime dateTime) {
    return sendCommand('set_time', <String, dynamic>{
      'year': dateTime.year,
      'month': dateTime.month,
      'day': dateTime.day,
      'hour': dateTime.hour,
      'minute': dateTime.minute,
      'second': dateTime.second,
    });
  }

  void _pushLog(String direction, String payload) {
    if (!upperUiShowCommunicationLog) return;
    _logs.insert(
      0,
      CommandLogEntry(
        direction: direction,
        payload: payload,
        timestamp: DateTime.now(),
      ),
    );
    if (_logs.length > 30) _logs.removeLast();
    _logController.add(List.unmodifiable(_logs));
  }

  void dispose() {
    _transportSub?.cancel();
    _dataController.close();
    _logController.close();
  }
}

class TimeParts {
  const TimeParts({
    required this.hour,
    required this.minute,
    required this.second,
  });

  final int hour;
  final int minute;
  final int second;

  Map<String, dynamic> toJson() => <String, dynamic>{
    'hour': hour,
    'minute': minute,
    'second': second,
  };

  static TimeParts? parse(String text) {
    final parts = text.trim().split(':');
    if (parts.length < 2 || parts.length > 3) return null;
    final hour = int.tryParse(parts[0]);
    final minute = int.tryParse(parts[1]);
    final second = parts.length == 3 ? int.tryParse(parts[2]) : 0;
    if (hour == null || minute == null || second == null) return null;
    if (hour < 0 || hour > 23) return null;
    if (minute < 0 || minute > 59) return null;
    if (second < 0 || second > 59) return null;
    return TimeParts(hour: hour, minute: minute, second: second);
  }
}
