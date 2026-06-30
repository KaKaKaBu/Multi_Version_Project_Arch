import 'dart:async';
import 'dart:convert';

import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

import '../config/mqtt_config.dart';
import 'transport_service.dart';

class MqttTransport extends TransportService {
  MqttTransport(this.config);

  final MqttConfig config;
  final _messageController = StreamController<String>.broadcast();
  final _connectionController = StreamController<bool>.broadcast();
  MqttServerClient? _client;
  StreamSubscription<List<MqttReceivedMessage<MqttMessage>>>? _subscription;

  @override
  Stream<String> get onMessage => _messageController.stream;

  @override
  Stream<bool> get onConnectionChanged => _connectionController.stream;

  @override
  bool get isConnected =>
      _client?.connectionStatus?.state == MqttConnectionState.connected;

  @override
  Future<bool> connect(Map<String, String> _) async {
    final client = MqttServerClient.withPort(
      config.host,
      config.clientId,
      config.port,
    );
    client.keepAlivePeriod = config.keepAliveSeconds;
    client.logging(on: false);
    client.connectionMessage = MqttConnectMessage()
        .withClientIdentifier(config.clientId)
        .startClean()
        .withWillQos(MqttQos.atMostOnce);
    client.onConnected = () => _connectionController.add(true);
    client.onDisconnected = () => _connectionController.add(false);

    await client.connect(config.username, config.password);
    if (client.connectionStatus?.state != MqttConnectionState.connected) {
      client.disconnect();
      _connectionController.add(false);
      return false;
    }

    client.subscribe(config.telemetryTopic, MqttQos.atMostOnce);
    _subscription = client.updates?.listen(_handleMessages);
    _client = client;
    _connectionController.add(true);
    return true;
  }

  @override
  Future<void> disconnect() async {
    await _subscription?.cancel();
    _subscription = null;
    _client?.disconnect();
    _client = null;
    _connectionController.add(false);
  }

  @override
  Future<void> send(String message) async {
    final client = _client;
    if (client == null || !isConnected) return;
    final payload = MqttClientPayloadBuilder()..addUTF8String(message);
    client.publishMessage(
      config.commandTopic,
      MqttQos.atMostOnce,
      payload.payload!,
    );
  }

  void _handleMessages(List<MqttReceivedMessage<MqttMessage>> messages) {
    for (final message in messages) {
      if (message.topic != config.telemetryTopic ||
          message.payload is! MqttPublishMessage) {
        continue;
      }
      final payload = message.payload as MqttPublishMessage;
      final text = utf8.decode(payload.payload.message);
      _messageController.add(text);
    }
  }

  void dispose() {
    disconnect();
    _messageController.close();
    _connectionController.close();
  }
}
