import 'dart:async';
import 'dart:convert';
import 'package:flutter/foundation.dart';
import '../core/config/debug_flags.dart';
import '../core/transport/transport_service.dart';
import '../core/version/version_capabilities.dart';
import 'telemetry_model.dart';

class FjxtService {
  final TransportService transport;
  final VersionCapabilities capabilities;
  final _dataController = StreamController<WindowTelemetry>.broadcast();

  WindowTelemetry _latest = const WindowTelemetry();
  Stream<WindowTelemetry> get onData => _dataController.stream;
  WindowTelemetry get latestData => _latest;

  FjxtService({required this.transport, required this.capabilities}) {
    transport.onMessage.listen(_onMessage);
  }

  void _onMessage(String raw) {
    try {
      final json = jsonDecode(raw) as Map<String, dynamic>;
      if (json['type'] == 'telemetry') {
        _latest = WindowTelemetry.fromJson(json);
        _dataController.add(_latest);
      }
    } catch (error) {
      if (upperUiShowCommunicationLog) {
        debugPrint('FjxtService parse error: $error');
      }
    }
  }

  void sendCommand(String cmd, [Map<String, dynamic>? params]) {
    final msg = <String, dynamic>{'cmd': cmd};
    if (params != null) msg['params'] = params;
    final payload = jsonEncode(msg);
    transport.send(payload);
    if (upperUiShowCommunicationLog) {
      debugPrint('FjxtService send: $payload');
    }
  }

  void requestStatus() => sendCommand('get_status');
  void openFull() => sendCommand('open');
  void closeFull() => sendCommand('close');
  void openStep() => sendCommand('open_step');
  void closeStep() => sendCommand('close_step');
  void stop() => sendCommand('stop');

  void dispose() {
    _dataController.close();
  }
}
