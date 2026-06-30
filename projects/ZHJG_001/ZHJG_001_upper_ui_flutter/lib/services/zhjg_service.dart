import 'dart:async';
import 'dart:convert';
import 'package:flutter/foundation.dart';
import '../core/config/debug_flags.dart';
import '../core/transport/transport_service.dart';
import '../core/version/version_capabilities.dart';
import 'telemetry_model.dart';

class ZhjgService {
  final TransportService transport;
  final VersionCapabilities capabilities;
  final _dataController = StreamController<ManholeTelemetry>.broadcast();

  ManholeTelemetry _latest = const ManholeTelemetry();
  Stream<ManholeTelemetry> get onData => _dataController.stream;
  ManholeTelemetry get latestData => _latest;

  ZhjgService({required this.transport, required this.capabilities}) {
    transport.onMessage.listen(_onMessage);
  }

  void _onMessage(String raw) {
    try {
      final json = jsonDecode(raw) as Map<String, dynamic>;
      if (json['type'] == 'telemetry') {
        _latest = ManholeTelemetry.fromJson(json);
        _dataController.add(_latest);
      }
    } catch (error) {
      if (upperUiShowCommunicationLog) {
        debugPrint('ZhjgService parse error: $error');
      }
    }
  }

  Future<bool> connect(Map<String, String> config) => transport.connect(config);
  Future<void> disconnect() => transport.disconnect();
  bool get isConnected => transport.isConnected;

  void sendCommand(String cmd, Map<String, dynamic>? params) {
    final msg = <String, dynamic>{'cmd': cmd};
    if (params != null) msg['params'] = params;
    final payload = jsonEncode(msg);
    transport.send(payload);
    if (upperUiShowCommunicationLog) {
      debugPrint('ZhjgService send: $payload');
    }
  }

  void setThreshold({int? methanePpm, double? waterPercent, double? tiltDegree}) {
    final params = <String, dynamic>{};
    if (methanePpm != null) params['methane_ppm'] = methanePpm;
    if (waterPercent != null) params['water_level_percent'] = waterPercent;
    if (tiltDegree != null) params['tilt_degree'] = tiltDegree;
    if (params.isNotEmpty) sendCommand('set_threshold', params);
  }

  void setMode(String mode) => sendCommand('set_mode', {'mode': mode});
  void requestStatus() => sendCommand('get_status', null);

  void dispose() {
    _dataController.close();
  }
}
