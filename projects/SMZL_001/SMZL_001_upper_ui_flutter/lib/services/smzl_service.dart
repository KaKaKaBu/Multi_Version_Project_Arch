import 'dart:async';
import '../core/transport/transport_service.dart';
import '../core/config/debug_flags.dart';
import '../core/version/version_capabilities.dart';
import 'telemetry_model.dart';
import 'package:flutter/foundation.dart';
import 'dart:convert';

class SmzlService {
  final TransportService transport;
  final VersionCapabilities capabilities;
  final _dataController = StreamController<SmzlData>.broadcast();
  SmzlData _latest = const SmzlData();

  Stream<SmzlData> get onData => _dataController.stream;
  SmzlData get latestData => _latest;

  SmzlService({required this.transport, required this.capabilities}) {
    transport.onMessage.listen(_onMessage);
  }

  void _onMessage(String raw) {
    try {
      final json = jsonDecode(raw) as Map<String, dynamic>;
      if (json['type'] == 'telemetry') {
        _latest = SmzlData.fromJson(json);
        _dataController.add(_latest);
      }
    } catch (e) {
      if (upperUiShowCommunicationLog) debugPrint('SmzlService: $e');
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
    int? hrU,
    int? hrL,
    int? sU,
    int? sL,
    double? tU,
    double? tL,
  }) {
    final p = <String, dynamic>{};
    if (hrU != null) p['hr_upper'] = hrU;
    if (hrL != null) p['hr_lower'] = hrL;
    if (sU != null) p['spo2_upper'] = sU;
    if (sL != null) p['spo2_lower'] = sL;
    if (tU != null) p['temp_upper'] = tU;
    if (tL != null) p['temp_lower'] = tL;
    if (p.isNotEmpty) sendCommand('set_threshold', p);
  }

  void clearTurnover() => sendCommand('clear_turnover', null);
  void requestStatus() => sendCommand('get_status', null);
  void dispose() => _dataController.close();
}
