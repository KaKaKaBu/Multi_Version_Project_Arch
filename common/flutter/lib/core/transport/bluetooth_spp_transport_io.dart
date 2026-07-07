import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/services.dart';
import 'package:flutter_bluetooth_serial/flutter_bluetooth_serial.dart';

import 'transport_service.dart';

class BluetoothSppDevice {
  const BluetoothSppDevice({required this.name, required this.address});

  final String name;
  final String address;

  String get label => name.isEmpty ? address : '$name  $address';
}

class BluetoothSppTransport extends TransportService {
  BluetoothSppTransport({this.namePrefix = 'JDY', this.address});

  static const _windowsMethodChannel = MethodChannel(
    'com.jbltech.common/bluetooth_spp',
  );
  static const _windowsEventChannel = EventChannel(
    'com.jbltech.common/bluetooth_spp_events',
  );

  final String namePrefix;
  final String? address;
  final _messageController = StreamController<String>.broadcast();
  final _connectionController = StreamController<bool>.broadcast();
  BluetoothConnection? _connection;
  StreamSubscription<Uint8List>? _inputSub;
  StreamSubscription<dynamic>? _windowsEvents;
  String _buffer = '';
  bool _connected = false;

  @override
  Stream<String> get onMessage => _messageController.stream;

  @override
  Stream<bool> get onConnectionChanged => _connectionController.stream;

  @override
  bool get isConnected => _connected;

  @override
  Future<bool> connect(Map<String, String> config) async {
    if (Platform.isWindows) {
      return _connectWindows(config);
    }
    if (!Platform.isAndroid) {
      throw UnsupportedError('当前平台不支持蓝牙 SPP 通信，请使用 Android 或 Windows 桌面端运行。');
    }

    final targetAddress = await _resolveAddress();
    if (targetAddress == null) {
      throw StateError('未找到已配对的 $namePrefix 蓝牙设备，请先在系统蓝牙设置中配对 JDY-31');
    }
    final connection = await BluetoothConnection.toAddress(targetAddress);
    _connection = connection;
    _inputSub = connection.input?.listen(
      _handleBytes,
      onDone: () {
        _connected = false;
        _connectionController.add(false);
      },
    );
    _connected = connection.isConnected;
    _connectionController.add(_connected);
    return _connected;
  }

  @override
  Future<void> disconnect() async {
    if (Platform.isWindows) {
      await _windowsMethodChannel.invokeMethod('disconnect');
      _connected = false;
      _connectionController.add(false);
      return;
    }
    if (!Platform.isAndroid) {
      _connected = false;
      _connectionController.add(false);
      return;
    }

    await _inputSub?.cancel();
    _inputSub = null;
    await _connection?.close();
    _connection = null;
    _connected = false;
    _connectionController.add(false);
  }

  @override
  Future<void> send(String message) async {
    if (Platform.isWindows) {
      if (!_connected) return;
      await _windowsMethodChannel.invokeMethod('write', {'data': '$message\n'});
      return;
    }
    if (!Platform.isAndroid) return;

    final connection = _connection;
    if (connection == null || !connection.isConnected) return;
    connection.output.add(Uint8List.fromList(utf8.encode('$message\n')));
    await connection.output.allSent;
  }

Future<bool> _connectWindows(Map<String, String> config) async {
    _windowsEvents ??= _windowsEventChannel.receiveBroadcastStream().listen(
      _handleWindowsEvent,
      onError: (Object error) {
        _connected = false;
        _connectionController.add(false);
        _messageController.add('ERROR:$error');
      },
    );

    final response = await _windowsMethodChannel
        .invokeMapMethod<String, Object?>('connect', {
          'namePrefix': namePrefix,
          if (address != null && address!.isNotEmpty) 'address': address,
          if (config['port']?.isNotEmpty == true) 'port': config['port'],
          if (config['baudRate']?.isNotEmpty == true)
            'baudRate': int.tryParse(config['baudRate']!) ?? 9600,
        });
    _connected = response?['connected'] == true;
    _connectionController.add(_connected);
    return _connected;
  }

  Future<String?> _resolveAddress() async {
    if (address != null && address!.isNotEmpty) return address;

    final devices = await discoverBluetoothSppDevices(namePrefix: namePrefix);
    if (devices.isNotEmpty) return devices.first.address;

    final bonded = await FlutterBluetoothSerial.instance.getBondedDevices();
    for (final device in bonded) {
      final name = device.name ?? '';
      if (name.toLowerCase().contains(namePrefix.toLowerCase())) {
        return device.address;
      }
    }
    return null;
  }

  void _handleBytes(Uint8List bytes) {
    _buffer += utf8.decode(bytes, allowMalformed: true);
    final lines = _buffer.split('\n');
    _buffer = lines.removeLast();
    for (final line in lines) {
      final trimmed = line.trim();
      if (trimmed.isNotEmpty) _messageController.add(trimmed);
    }
  }

  void _handleWindowsEvent(Object? event) {
    if (event is! Map) return;
    final type = event['type']?.toString();
    if (type == 'status') {
      final state = event['state']?.toString();
      _connected = state == 'connected';
      _connectionController.add(_connected);
    } else if (type == 'data') {
      final data = event['data']?.toString().trim();
      if (data != null && data.isNotEmpty) _messageController.add(data);
    } else if (type == 'error') {
      final message = event['message']?.toString();
      if (message != null && message.isNotEmpty) {
        _messageController.add('ERROR:$message');
      }
    }
  }

  void dispose() {
    disconnect();
    _inputSub?.cancel();
    _windowsEvents?.cancel();
    _messageController.close();
    _connectionController.close();
  }
}

Future<List<BluetoothSppDevice>> discoverBluetoothSppDevices({
  String namePrefix = 'JDY',
  bool includeBonded = true,
}) async {
  if (!Platform.isAndroid) {
    return const <BluetoothSppDevice>[];
  }

  final prefix = namePrefix.toLowerCase();
  final byAddress = <String, BluetoothSppDevice>{};
  StreamSubscription<BluetoothDiscoveryResult>? sub;

  void addDevice(String? name, String address) {
    final displayName = name ?? '';
    if (prefix.isNotEmpty &&
        (displayName.isEmpty ||
            !displayName.toLowerCase().contains(prefix))) {
      return;
    }
    byAddress[address] = BluetoothSppDevice(
      name: displayName,
      address: address,
    );
  }

  try {
    await FlutterBluetoothSerial.instance.cancelDiscovery();
    final done = Completer<void>();
    sub = FlutterBluetoothSerial.instance.startDiscovery().listen(
      (result) => addDevice(result.device.name, result.device.address),
      onError: (_) {
        if (!done.isCompleted) done.complete();
      },
      onDone: () {
        if (!done.isCompleted) done.complete();
      },
    );

    await done.future.timeout(
      const Duration(seconds: 8),
      onTimeout: () {},
    );
  } finally {
    await sub?.cancel();
    await FlutterBluetoothSerial.instance.cancelDiscovery();
  }

  if (includeBonded) {
    final bonded = await FlutterBluetoothSerial.instance.getBondedDevices();
    for (final device in bonded) {
      addDevice(device.name, device.address);
    }
  }

  final devices = byAddress.values.toList()
    ..sort((a, b) => a.label.toLowerCase().compareTo(b.label.toLowerCase()));
  return devices;
}
