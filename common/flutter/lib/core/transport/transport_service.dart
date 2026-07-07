import 'dart:async';

abstract class TransportService {
  Stream<String> get onMessage;
  Stream<bool> get onConnectionChanged;

  Future<bool> connect(Map<String, String> config);
  Future<void> disconnect();
  Future<void> send(String message);
  bool get isConnected;
}

class ActiveTransportService extends TransportService {
  final _messageController = StreamController<String>.broadcast();
  final _connectionController = StreamController<bool>.broadcast();
  TransportService? _active;
  StreamSubscription<String>? _messageSub;
  StreamSubscription<bool>? _connectionSub;
  bool _disposed = false;

  @override
  Stream<String> get onMessage => _messageController.stream;

  @override
  Stream<bool> get onConnectionChanged => _connectionController.stream;

  @override
  bool get isConnected => _active?.isConnected ?? false;

  Future<bool> connectWith(
    TransportService transport,
    Map<String, String> config,
  ) async {
    await disconnect();
    _active = transport;
    _messageSub = transport.onMessage.listen(_messageController.add);
    _connectionSub = transport.onConnectionChanged.listen(
      _connectionController.add,
    );
    return transport.connect(config);
  }

  @override
  Future<bool> connect(Map<String, String> config) async =>
      _active?.connect(config) ?? false;

  @override
  Future<void> disconnect() async {
    await _messageSub?.cancel();
    await _connectionSub?.cancel();
    _messageSub = null;
    _connectionSub = null;
    await _active?.disconnect();
    _active = null;
    if (!_disposed && !_connectionController.isClosed) {
      _connectionController.add(false);
    }
  }

  @override
  Future<void> send(String message) async {
    final transport = _active;
    if (transport == null || !transport.isConnected) return;
    await transport.send(message);
  }

  void dispose() {
    _disposed = true;
    _messageSub?.cancel();
    _connectionSub?.cancel();
    _active?.disconnect();
    _active = null;
    if (!_messageController.isClosed) _messageController.close();
    if (!_connectionController.isClosed) _connectionController.close();
  }
}

class MockTransport extends TransportService {
  final _messageController = StreamController<String>.broadcast();
  final _connectionController = StreamController<bool>.broadcast();
  bool _connected = false;

  @override
  Stream<String> get onMessage => _messageController.stream;

  @override
  Stream<bool> get onConnectionChanged => _connectionController.stream;

  @override
  bool get isConnected => _connected;

  @override
  Future<bool> connect(Map<String, String> config) async {
    _connected = true;
    _connectionController.add(true);
    return true;
  }

  @override
  Future<void> disconnect() async {
    _connected = false;
    _connectionController.add(false);
  }

  @override
  Future<void> send(String message) async {}

  void feedMessage(String json) => _messageController.add(json);

  void dispose() {
    _messageController.close();
    _connectionController.close();
  }
}
