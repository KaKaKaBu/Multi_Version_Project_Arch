import 'dart:async';

import 'package:flutter/widgets.dart';

import '../config/huawei_iotda_config.dart';
import '../config/mqtt_config.dart';
import '../transport/bluetooth_spp_transport.dart';
import '../transport/huawei_iotda_transport.dart';
import '../transport/mqtt_transport.dart';
import '../transport/transport_service.dart';
import '../version/version_capabilities.dart';

class CommunicationController extends ChangeNotifier {
  CommunicationController({
    required this.capabilities,
    this.mqttConfig,
    this.huaweiIotdaConfig,
    this.bleNamePrefix = '',
    ActiveTransportService? transport,
  }) : transport = transport ?? ActiveTransportService() {
    _connectionSub = this.transport.onConnectionChanged.listen((connected) {
      _connected = connected;
      notifyListeners();
    });
  }

  final VersionCapabilities capabilities;
  final MqttConfig? mqttConfig;
  final HuaweiIotdaConfig? huaweiIotdaConfig;
  final String bleNamePrefix;
  final ActiveTransportService transport;
  final bleAddressController = TextEditingController();

  StreamSubscription<bool>? _connectionSub;
  bool _connected = false;
  bool _busy = false;
  String _label = '未连接';
  String? _error;
  bool _disposed = false;

  bool get connected => _connected;
  bool get busy => _busy;
  String get label => _label;
  String? get error => _error;
  bool get mqttEnabled => capabilities.has('wifi') && mqttConfig != null;
  bool get cloudEnabled => capabilities.has('cloud') && huaweiIotdaConfig != null;
  bool get bleEnabled => capabilities.has('ble');

  Future<bool> connectMqtt() {
    final config = mqttConfig;
    if (config == null) return _failUnavailable('MQTT 配置未设置');
    return _connect('MQTT', MqttTransport(config));
  }

  Future<bool> connectHuaweiCloud() {
    final config = huaweiIotdaConfig;
    if (config == null) return _failUnavailable('华为云配置未设置');
    return _connect('华为云', HuaweiIotdaTransport(config));
  }

  Future<bool> connectBleFromInput() {
    return _connect(
      'BLE',
      BluetoothSppTransport(
        namePrefix: bleNamePrefix,
        address: bleAddressController.text.trim(),
      ),
    );
  }

  Future<bool> connectBleDevice(BluetoothSppDevice device) {
    return _connect(
      device.name.isEmpty ? 'BLE' : 'BLE ${device.name}',
      BluetoothSppTransport(namePrefix: bleNamePrefix, address: device.address),
    );
  }

  Future<List<BluetoothSppDevice>> scanBleDevices() async {
    _setBusy(true, label: '搜索附近蓝牙设备...', clearError: true);
    try {
      final devices = await discoverBluetoothSppDevices(
        namePrefix: bleNamePrefix,
        includeBonded: false,
      );
      if (devices.isEmpty) {
        _setIdleError(
          '未搜索到附近蓝牙设备，请确认模块已上电、未被其他设备占用，并已开启蓝牙扫描权限。',
        );
      }
      return devices;
    } catch (error) {
      _setIdleError('蓝牙搜索失败：$error');
      return const <BluetoothSppDevice>[];
    } finally {
      if (!_disposed) {
        _busy = false;
        notifyListeners();
      }
    }
  }

  void cancelBleSelectionIfDisconnected() {
    if (!_connected) {
      _label = '未连接';
      notifyListeners();
    }
  }

  Future<bool> _connect(String label, TransportService nextTransport) async {
    _setBusy(true, clearError: true);
    try {
      final ok = await transport.connectWith(nextTransport, const {});
      _label = ok ? label : '未连接';
      _error = ok ? null : '$label 连接失败';
      notifyListeners();
      return ok;
    } catch (error) {
      await transport.disconnect();
      _connected = false;
      _label = '未连接';
      _error = error.toString();
      notifyListeners();
      return false;
    } finally {
      if (!_disposed) {
        _busy = false;
        notifyListeners();
      }
    }
  }

  Future<bool> _failUnavailable(String error) async {
    _label = '未连接';
    _error = error;
    notifyListeners();
    return false;
  }

  Future<void> disconnect() async {
    await transport.disconnect();
    _connected = false;
    _label = '未连接';
    notifyListeners();
  }

  void _setBusy(bool value, {String? label, bool clearError = false}) {
    _busy = value;
    if (label != null) _label = label;
    if (clearError) _error = null;
    notifyListeners();
  }

  void _setIdleError(String error) {
    _label = '未连接';
    _error = error;
    notifyListeners();
  }

  @override
  void dispose() {
    _disposed = true;
    _connectionSub?.cancel();
    bleAddressController.dispose();
    transport.dispose();
    super.dispose();
  }
}
