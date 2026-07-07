import 'dart:async';
import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart' as common;

import 'package:mvp_flutter_common/core/config/debug_flags.dart';
import '../../core/config/mqtt_config.dart';
import '../../core/models/dcld_models.dart';
import '../../core/protocol/dcld_protocol.dart';
import '../../core/version/version_capabilities.dart';

class DashboardController extends ChangeNotifier {
  DashboardController({VersionCapabilities? capabilities})
    : capabilities = capabilities ?? resolveUpperFeatures(),
      telemetry = createDefaultTelemetry(capabilities ?? resolveUpperFeatures()),
      thresholds = createDefaultThresholds(),
      connection = boardMqttConfig {
    communication = common.CommunicationController(
      capabilities: commonCapabilities,
      mqttConfig: _commonMqttConfig(connection),
      bleNamePrefix: 'JDY',
    );
    _subscriptions
      ..add(communication.transport.onMessage.listen(handleTelemetry))
      ..add(
        communication.transport.onConnectionChanged.listen((_) {
          notifyListeners();
        }),
      );
  }

  final VersionCapabilities capabilities;
  TelemetryState telemetry;
  ThresholdState thresholds;
  final ConnectionConfig connection;
  late final common.CommunicationController communication;
  bool pending = false;
  WorkMode? pendingMode;
  String? pendingThreshold;
  String lastSync = '';
  final logs = <LogEntry>[];
  final _subscriptions = <StreamSubscription<dynamic>>[];

  List<MetricDefinition> get supportedMetrics => getSupportedMetrics(capabilities);
  List<ThresholdDefinition> get supportedThresholds => getSupportedThresholds();
  String get versionDescription => describeVersion(capabilities);
  String get enabledMetricLabels => enabledFeatureSummary(capabilities);
  bool get hasRemote => capabilities.has('remote');
  bool get alarmActive => telemetry.alarm != 0 || calculateAlarm(telemetry, thresholds) != 0;
  bool get cameraEnabled => capabilities.has('camera');
  String get cameraUrl => telemetry.cameraUrl;
  String get modeLabel => telemetry.mode.label;
  String get lastSyncDisplay => lastSync.isEmpty ? '尚未同步' : lastSync;
  String get distanceDisplay => _formatValue(telemetry['distance_cm'], 0, 'cm');
  String get thresholdDisplay => _formatValue(thresholds['distance'], 0, 'cm');
  bool get connecting => communication.busy;
  String get transportState => communication.connected
      ? 'connected'
      : communication.busy
      ? 'connecting'
      : 'disconnected';
  String get connectionLabel => communication.connected
      ? '已连接'
      : communication.busy
      ? '连接中'
      : '未连接';

  common.VersionCapabilities get commonCapabilities => common.VersionCapabilities(
    version: capabilities.version,
    features: capabilities.features,
  );

  Future<void> connectTransport() async {
    if (!hasRemote) {
      _pushLog('ERR', '当前版本无远程通信');
      notifyListeners();
      return;
    }
    final ok = capabilities.has('wifi')
        ? await communication.connectMqtt()
        : await communication.connectBleFromInput();
    if (ok) await refreshStatus();
  }

  Future<void> disconnectTransport() => communication.disconnect();

  Future<void> refreshStatus() => sendCommand(buildGetStatusCommand());

  Future<void> setMode(WorkMode mode) async {
    pendingMode = mode;
    notifyListeners();
    await sendCommand(buildSetModeCommand(mode));
    pendingMode = null;
    notifyListeners();
  }

  void updateThreshold(String key, num value) {
    final definition = supportedThresholds.firstWhere((item) => item.key == key);
    final clamped = value.clamp(definition.min, definition.max);
    thresholds = thresholds.copyWithValue(key, clamped);
    final values = Map<String, num?>.from(telemetry.values);
    if (key == 'distance') {
      values['threshold_cm'] = clamped;
    }
    final nextTelemetry = telemetry.copyWith(values: values);
    telemetry = nextTelemetry.copyWith(alarm: calculateAlarm(nextTelemetry, thresholds));
    notifyListeners();
  }

  Future<void> applyThreshold(String key) async {
    pendingThreshold = key;
    notifyListeners();
    await sendCommand(buildSetThresholdCommand(thresholds[key] ?? 0));
    pendingThreshold = null;
    notifyListeners();
  }

  Future<void> sendCommand(Map<String, dynamic> command) async {
    pending = true;
    notifyListeners();
    try {
      if (!hasRemote) {
        throw StateError('当前版本无远程通信');
      }
      if (!communication.transport.isConnected) {
        throw StateError('通信未连接');
      }
      _pushLog('TX', command);
      await communication.transport.send(serializeCommand(command));
    } catch (error) {
      _pushLog('ERR', error.toString());
    } finally {
      pending = false;
      notifyListeners();
    }
  }

  void handleTelemetry(Object? payload) {
    final normalized = normalizeTelemetry(payload, capabilities, previous: telemetry, thresholds: thresholds);
    telemetry = normalized.telemetry;
    thresholds = normalizeThresholds(payload, previous: thresholds);
    lastSync = _formatTime(DateTime.now());
    _pushLog('RX', normalized.raw);
    notifyListeners();
  }

  String copyableLogs() => logs.map((item) => '[${item.direction}] ${item.payload}').join('\n');

  @override
  void dispose() {
    for (final subscription in _subscriptions) {
      unawaited(subscription.cancel());
    }
    _subscriptions.clear();
    communication.dispose();
    super.dispose();
  }

  static common.MqttConfig _commonMqttConfig(ConnectionConfig config) {
    return common.MqttConfig(
      host: config.host,
      port: config.port,
      clientId: buildClientId('flutter'),
      username: config.username,
      password: config.password,
      commandTopic: config.commandTopic,
      telemetryTopic: config.telemetryTopic,
      keepAliveSeconds: config.keepalive,
    );
  }

  String _formatTime(DateTime time) {
    String two(int value) => value.toString().padLeft(2, '0');
    return '${two(time.hour)}:${two(time.minute)}:${two(time.second)}';
  }

  String _formatValue(num? value, int precision, String unit) {
    if (value == null) return '-- $unit';
    return '${value.toStringAsFixed(precision)} $unit';
  }

  void _pushLog(String direction, Object payload) {
    if (!upperUiShowCommunicationLog) return;
    final text = payload is String ? payload : const JsonEncoder.withIndent('  ').convert(payload);
    logs.insert(0, LogEntry(id: DateTime.now().microsecondsSinceEpoch.toString(), direction: direction, payload: text));
    if (logs.length > 20) {
      logs.removeRange(20, logs.length);
    }
  }
}
