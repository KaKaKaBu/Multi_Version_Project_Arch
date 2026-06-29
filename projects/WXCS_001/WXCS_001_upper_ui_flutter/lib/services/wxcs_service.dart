import 'dart:async';
import '../core/transport/transport_service.dart';
import '../core/config/debug_flags.dart';
import '../core/version/version_capabilities.dart';
import 'telemetry_model.dart';
import 'package:flutter/foundation.dart';
import 'dart:convert';

class WxcsService {
  final TransportService transport;
  final VersionCapabilities capabilities;
  final _dataController = StreamController<TelemetryData>.broadcast();

  TelemetryData _latest = const TelemetryData();
  Stream<TelemetryData> get onData => _dataController.stream;
  TelemetryData get latestData => _latest;

  WxcsService({required this.transport, required this.capabilities}) {
    transport.onMessage.listen(_onMessage);
  }

  void _onMessage(String raw) {
    try {
      final json = jsonDecode(raw) as Map<String, dynamic>;
      if (json['type'] == 'telemetry') {
        _latest = TelemetryData.fromJson(json);
        _dataController.add(_latest);
      }
    } catch (e) {
      if (upperUiShowCommunicationLog) {
        debugPrint('WxcsService parse error: $e');
      }
    }
  }

  Future<bool> connect(Map<String, String> config) => transport.connect(config);
  Future<void> disconnect() => transport.disconnect();
  bool get isConnected => transport.isConnected;

  void sendCommand(String cmd, Map<String, dynamic>? params) {
    final msg = <String, dynamic>{'cmd': cmd};
    if (params != null) msg['params'] = params;
    final String json = jsonEncode(msg);
    transport.send(json);
    if (upperUiShowCommunicationLog) {
      debugPrint('WxcsService send: ${jsonEncode(msg)}');
    }
  }

  void setThreshold({int? temp, int? aq, int? co}) {
    final params = <String, int>{};
    if (temp != null) params['temp'] = temp;
    if (aq != null) params['air_quality_ppm'] = aq;
    if (co != null) params['co_ppm'] = co;
    if (params.isNotEmpty) sendCommand('set_threshold', params);
  }

  void setMode(String mode) => sendCommand('set_mode', {'mode': mode});
  void setControl({int? fan, int? alarm}) {
    final params = <String, int>{};
    if (fan != null) params['fan'] = fan;
    if (alarm != null) params['alarm'] = alarm;
    if (params.isNotEmpty) sendCommand('set_control', params);
  }

  void requestStatus() => sendCommand('get_status', null);

  void dispose() {
    _dataController.close();
  }
}
