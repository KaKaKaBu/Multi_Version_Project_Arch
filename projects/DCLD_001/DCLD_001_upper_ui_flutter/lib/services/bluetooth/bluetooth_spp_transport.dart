import 'dart:async';

import 'package:flutter/services.dart';

import '../../core/protocol/dcld_protocol.dart';
import '../transport/upper_transport.dart';

class BluetoothSppTransport extends UpperTransport {
  BluetoothSppTransport({this.namePrefix = 'JDY', this.address, this.uuid});

  static const _methodChannel = MethodChannel('com.jbltech.dcld001/bluetooth_spp');
  static const _eventChannel = EventChannel('com.jbltech.dcld001/bluetooth_spp_events');

  final String namePrefix;
  final String? address;
  final String? uuid;
  final _telemetry = StreamController<Object?>.broadcast();
  final _status = StreamController<String>.broadcast();
  final _errors = StreamController<String>.broadcast();
  StreamSubscription<dynamic>? _events;

  @override
  Stream<Object?> get telemetryStream => _telemetry.stream;

  @override
  Stream<String> get statusStream => _status.stream;

  @override
  Stream<String> get errorStream => _errors.stream;

  @override
  Future<void> connect() async {
    _events ??= _eventChannel.receiveBroadcastStream().listen(_handleEvent, onError: (Object error) {
      _errors.add(error.toString());
    });
    _status.add('connecting');
    await _methodChannel.invokeMethod('connect', {
      'namePrefix': namePrefix,
      if (address != null && address!.isNotEmpty) 'address': address,
      if (uuid != null && uuid!.isNotEmpty) 'uuid': uuid,
    });
  }

  @override
  Future<void> sendCommand(Map<String, dynamic> command) async {
    await _methodChannel.invokeMethod('write', {'data': '${serializeCommand(command)}\n'});
  }

  @override
  Future<void> disconnect() async {
    await _methodChannel.invokeMethod('disconnect');
    _status.add('disconnected');
  }

  @override
  Future<void> dispose() async {
    await disconnect();
    await _events?.cancel();
    _events = null;
    await _telemetry.close();
    await _status.close();
    await _errors.close();
  }

  void _handleEvent(Object? event) {
    if (event is! Map) {
      return;
    }
    final type = event['type']?.toString();
    if (type == 'status') {
      _status.add(event['state']?.toString() ?? 'disconnected');
    } else if (type == 'data') {
      _telemetry.add(parseTelemetry(event['data']));
    } else if (type == 'error') {
      _errors.add(event['message']?.toString() ?? '未知蓝牙错误');
    }
  }
}
