import 'dart:async';

abstract class UpperTransport {
  Stream<Object?> get telemetryStream;
  Stream<String> get statusStream;
  Stream<String> get errorStream;

  Future<void> connect();
  Future<void> sendCommand(Map<String, dynamic> command);
  Future<void> disconnect();

  Future<void> dispose() async {
    await disconnect();
  }
}
