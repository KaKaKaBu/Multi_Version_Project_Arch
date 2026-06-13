import 'dart:async';
import 'dart:convert';

import 'package:flutter/foundation.dart';

import '../../core/config/debug_flags.dart';
import '../../core/config/mqtt_config.dart';
import '../../core/models/kqzl3_models.dart';
import '../../core/protocol/kqzl3_protocol.dart';
import '../../core/version/version_capabilities.dart';
import '../../services/bluetooth/bluetooth_spp_transport.dart';
import '../../services/mqtt/mqtt_transport.dart';
import '../../services/transport/upper_transport.dart';

class DashboardController extends ChangeNotifier {
  DashboardController({VersionCapabilities? capabilities})
      : capabilities = capabilities ?? resolveUpperFeatures(),
        telemetry = createDefaultTelemetry(capabilities ?? resolveUpperFeatures()),
        thresholds = createDefaultThresholds(capabilities ?? resolveUpperFeatures()),
        connection = boardMqttConfig;

  final VersionCapabilities capabilities;
  TelemetryState telemetry;
  ThresholdState thresholds;
  ConnectionConfig connection;
  bool pending = false;
  bool connecting = false;
  WorkMode? pendingMode;
  String? pendingThreshold;
  String transportState = 'disconnected';
  String lastSync = '';
  final logs = <LogEntry>[];
  UpperTransport? _transport;
  final _subscriptions = <StreamSubscription<dynamic>>[];

  List<SensorDefinition> get supportedSensors => getSupportedSensors(capabilities);
  List<SensorDefinition> get supportedThresholds => getSupportedThresholds(capabilities);
  List<ActuatorDefinition> get supportedActuators => getSupportedActuators();
  String get versionDescription => describeVersion(capabilities);
  String get enabledSensorLabels => supportedSensors.map((item) => item.label).join('/');
  bool get hasRemote => capabilities.has('remote');
  bool get alarmActive => telemetry.alarm != 0 || calculateAlarm(telemetry, thresholds, capabilities) != 0;
  String get modeLabel => telemetry.mode.label;
  String get lastSyncDisplay => lastSync.isEmpty ? '尚未同步' : lastSync;
  String get connectionLabel => transportState == 'connected'
      ? '已连接'
      : transportState == 'connecting' || transportState == 'scanning'
          ? '连接中'
          : '未连接';
  String get connectionPanelState => hasRemote ? transportState : 'unsupported';

  String get connectionTitle {
    if (capabilities.has('aliyun')) return '阿里云 MQTT';
    if (capabilities.has('wifi')) return 'WiFi MQTT';
    if (capabilities.has('ble')) return 'JDY-31 蓝牙';
    return '无远程通信';
  }

  String get connectionDescription {
    if (!hasRemote) return '当前固件版本不包含 MQTT/BLE 远程通信模块，仅显示本地默认状态。';
    if (capabilities.has('wifi')) {
      final endpoint = connection.wsEndpoint.isNotEmpty ? connection.wsEndpoint : '${connection.host}:${connection.port}';
      return '$endpoint · TX ${connection.commandTopic} · RX ${connection.telemetryTopic}';
    }
    if (capabilities.has('ble')) return '通过 JDY-31 透明串口收发 JSON 命令。';
    return '当前版本未配置可用通信方式。';
  }

  bool get canConnect => hasRemote;

  void updateConnection(ConnectionConfig value) {
    connection = value;
    notifyListeners();
  }

  Future<void> connectTransport() async {
    if (!hasRemote) {
      _pushLog('ERR', '当前版本无远程通信');
      notifyListeners();
      return;
    }
    await disconnectTransport();
    connecting = true;
    transportState = capabilities.has('ble') ? 'scanning' : 'connecting';
    notifyListeners();

    final transport = capabilities.has('wifi')
        ? MqttUpperTransport(config: connection, clientId: buildClientId('v${capabilities.version}'))
        : BluetoothSppTransport();
    _transport = transport;
    _subscriptions
      ..add(transport.telemetryStream.listen(handleTelemetry))
      ..add(transport.statusStream.listen((state) {
        transportState = state;
        connecting = state == 'connecting' || state == 'scanning';
        notifyListeners();
      }))
      ..add(transport.errorStream.listen((error) {
        connecting = false;
        _pushLog('ERR', error);
        notifyListeners();
      }));

    try {
      await transport.connect();
    } catch (error) {
      connecting = false;
      transportState = 'disconnected';
      _pushLog('ERR', error.toString());
      notifyListeners();
    }
  }

  Future<void> disconnectTransport() async {
    for (final subscription in _subscriptions) {
      await subscription.cancel();
    }
    _subscriptions.clear();
    final transport = _transport;
    _transport = null;
    if (transport != null) {
      try {
        await transport.dispose();
      } catch (_) {}
    }
    connecting = false;
    transportState = 'disconnected';
    notifyListeners();
  }

  Future<void> refreshStatus() => sendCommand(buildGetStatusCommand());

  Future<void> setMode(WorkMode mode) async {
    pendingMode = mode;
    notifyListeners();
    await sendCommand(buildSetModeCommand(mode));
    pendingMode = null;
    notifyListeners();
  }

  Future<void> setDevice(String device, bool state) async {
    if (telemetry.mode != WorkMode.manual) {
      await sendCommand(buildSetModeCommand(WorkMode.manual));
    }
    await sendCommand(buildSetDeviceCommand(device, state));
  }

  void updateThreshold(String sensor, num value) {
    thresholds = thresholds.copyWithValue(sensor, value);
    notifyListeners();
  }

  Future<void> applyThreshold(String sensor) async {
    pendingThreshold = sensor;
    notifyListeners();
    await sendCommand(buildSetThresholdCommand(sensor, thresholds[sensor] ?? 0));
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
      final transport = _transport;
      if (transport == null) {
        throw StateError('通信未连接');
      }
      _pushLog('TX', command);
      await transport.sendCommand(command);
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
    thresholds = normalizeThresholds(payload, capabilities, previous: thresholds);
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
    final transport = _transport;
    _transport = null;
    if (transport != null) {
      unawaited(transport.dispose());
    }
    super.dispose();
  }

  void _pushLog(String direction, Object? payload) {
    if (!upperUiShowCommunicationLog) return;
    final text = payload is String ? payload : jsonEncode(payload);
    logs.insert(0, LogEntry(id: DateTime.now().microsecondsSinceEpoch.toString(), direction: direction, payload: text));
    if (logs.length > 30) {
      logs.removeRange(30, logs.length);
    }
  }

  String _formatTime(DateTime date) {
    String pad(int value) => value.toString().padLeft(2, '0');
    return '${pad(date.hour)}:${pad(date.minute)}:${pad(date.second)}';
  }
}
