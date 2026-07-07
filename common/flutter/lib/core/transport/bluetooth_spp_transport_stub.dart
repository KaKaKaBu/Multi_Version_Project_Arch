import 'transport_service.dart';

class BluetoothSppDevice {
  const BluetoothSppDevice({required this.name, required this.address});

  final String name;
  final String address;

  String get label => name.isEmpty ? address : '$name  $address';
}

class BluetoothSppTransport extends TransportService {
  BluetoothSppTransport({this.namePrefix = 'JDY', this.address});

  final String namePrefix;
  final String? address;

  @override
  Stream<String> get onMessage => const Stream<String>.empty();

  @override
  Stream<bool> get onConnectionChanged => const Stream<bool>.empty();

  @override
  bool get isConnected => false;

  @override
  Future<bool> connect(Map<String, String> config) async {
    throw UnsupportedError('当前平台不支持蓝牙 SPP 通信，请使用 Android 或 Windows 桌面端运行。');
  }

  @override
  Future<void> disconnect() async {}

  @override
  Future<void> send(String message) async {}
}

Future<List<BluetoothSppDevice>> discoverBluetoothSppDevices({
  String namePrefix = 'JDY',
  bool includeBonded = true,
}) async =>
    const <BluetoothSppDevice>[];
