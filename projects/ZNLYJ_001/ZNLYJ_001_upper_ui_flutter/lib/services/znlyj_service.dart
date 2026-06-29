import 'dart:async';
import '../core/transport/transport_service.dart';
import '../core/config/debug_flags.dart';
import '../core/version/version_capabilities.dart';
import 'telemetry_model.dart';
import 'package:flutter/foundation.dart';
import 'dart:convert';

class ZnlyjService {
  final TransportService transport;
  final VersionCapabilities capabilities;
  final _dataController = StreamController<ZnlyjData>.broadcast();
  ZnlyjData _latest = const ZnlyjData();

  Stream<ZnlyjData> get onData => _dataController.stream;
  ZnlyjData get latestData => _latest;
  bool get hasLight => capabilities.has('light');
  bool get hasWeight => capabilities.has('weight');

  ZnlyjService({required this.transport, required this.capabilities}) {
    transport.onMessage.listen(_onMessage);
  }

  void _onMessage(String raw) {
    try {
      final json = jsonDecode(raw) as Map<String, dynamic>;
      if (json['type'] == 'telemetry') {
        _latest = ZnlyjData.fromJson(json);
        _dataController.add(_latest);
      }
    } catch (e) {
      if (upperUiShowCommunicationLog) debugPrint('ZnlyjService: $e');
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
  }

  void setThreshold({
    double? temp,
    double? humidity,
    double? light,
    double? weight,
  }) {
    final p = <String, dynamic>{};
    if (temp != null) p['temp'] = temp;
    if (humidity != null) p['humidity'] = humidity;
    if (light != null) p['light'] = light;
    if (weight != null) p['weight'] = weight;
    if (p.isNotEmpty) sendCommand('set_threshold', p);
  }

  void setRack(bool open) => sendCommand('set_rack', {'open': open ? 1 : 0});
  void setMode(String mode) => sendCommand('set_mode', {'mode': mode});
  void requestStatus() => sendCommand('get_status', null);
  void dispose() => _dataController.close();
}
