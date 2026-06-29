import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter_bluetooth_serial/flutter_bluetooth_serial.dart';

import 'transport_service.dart';

class BluetoothSppTransport extends TransportService {
  BluetoothSppTransport({this.namePrefix = 'JDY', this.address});

  final String namePrefix;
  final String? address;
  final _messageController = StreamController<String>.broadcast();
  final _connectionController = StreamController<bool>.broadcast();
  BluetoothConnection? _connection;
  StreamSubscription<Uint8List>? _inputSub;
  String _buffer = '';
  bool _connected = false;

  @override
  Stream<String> get onMessage => _messageController.stream;

  @override
  Stream<bool> get onConnectionChanged => _connectionController.stream;

  @override
  bool get isConnected => _connected;

  @override
  Future<bool> connect(Map<String, String> _) async {
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
    await _inputSub?.cancel();
    _inputSub = null;
    await _connection?.close();
    _connection = null;
    _connected = false;
    _connectionController.add(false);
  }

  @override
  Future<void> send(String message) async {
    final connection = _connection;
    if (connection == null || !connection.isConnected) return;
    connection.output.add(Uint8List.fromList(utf8.encode('$message\n')));
    await connection.output.allSent;
  }

  Future<String?> _resolveAddress() async {
    if (address != null && address!.isNotEmpty) return address;
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

  void dispose() {
    disconnect();
    _messageController.close();
    _connectionController.close();
  }
}
