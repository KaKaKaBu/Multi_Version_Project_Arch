import 'dart:async';
import 'dart:convert';

import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

import '../../core/models/dcld_models.dart';
import '../../core/protocol/dcld_protocol.dart';
import '../transport/upper_transport.dart';

class MqttUpperTransport extends UpperTransport {
  MqttUpperTransport({required this.config, required this.clientId});

  final ConnectionConfig config;
  final String clientId;
  final _telemetry = StreamController<Object?>.broadcast();
  final _status = StreamController<String>.broadcast();
  final _errors = StreamController<String>.broadcast();
  MqttServerClient? _client;
  StreamSubscription<List<MqttReceivedMessage<MqttMessage>>>? _messages;

  @override
  Stream<Object?> get telemetryStream => _telemetry.stream;

  @override
  Stream<String> get statusStream => _status.stream;

  @override
  Stream<String> get errorStream => _errors.stream;

  @override
  Future<void> connect() async {
    _status.add('connecting');
    final uri = Uri.tryParse(config.wsEndpoint);
    final useWebSocket = uri != null && uri.scheme.startsWith('ws');
    final host = useWebSocket ? uri.host : config.host;
    final port = useWebSocket ? uri.port : config.port;
    final client = MqttServerClient.withPort(host, clientId, port == 0 ? config.port : port);
    client.keepAlivePeriod = config.keepalive;
    client.logging(on: false);
    client.secure = config.useTLS || uri?.scheme == 'wss';
    client.useWebSocket = useWebSocket;
    if (useWebSocket) {
      client.websocketProtocols = MqttClientConstants.protocolsSingleDefault;
    }
    client.connectionMessage = MqttConnectMessage()
        .withClientIdentifier(clientId)
        .startClean()
        .withWillQos(MqttQos.atMostOnce);
    client.onConnected = () => _status.add('connected');
    client.onDisconnected = () => _status.add('disconnected');

    try {
      await client.connect(
        config.username.isEmpty ? null : config.username,
        config.password.isEmpty ? null : config.password,
      );
      if (client.connectionStatus?.state != MqttConnectionState.connected) {
        throw StateError(client.connectionStatus?.returnCode.toString() ?? 'MQTT 连接失败');
      }
      client.subscribe(config.telemetryTopic, MqttQos.atMostOnce);
      _messages = client.updates?.listen(_handleMessages);
      _client = client;
    } catch (error) {
      client.disconnect();
      _errors.add(error.toString());
      _status.add('disconnected');
      rethrow;
    }
  }

  @override
  Future<void> sendCommand(Map<String, dynamic> command) async {
    final client = _client;
    if (client == null || client.connectionStatus?.state != MqttConnectionState.connected) {
      throw StateError('MQTT 未连接');
    }
    final payload = MqttClientPayloadBuilder()..addUTF8String(serializeCommand(command));
    client.publishMessage(config.commandTopic, MqttQos.atMostOnce, payload.payload!);
  }

  @override
  Future<void> disconnect() async {
    await _messages?.cancel();
    _messages = null;
    _client?.disconnect();
    _client = null;
    _status.add('disconnected');
  }

  @override
  Future<void> dispose() async {
    await disconnect();
    await _telemetry.close();
    await _status.close();
    await _errors.close();
  }

  void _handleMessages(List<MqttReceivedMessage<MqttMessage>> messages) {
    for (final message in messages) {
      if (message.topic != config.telemetryTopic || message.payload is! MqttPublishMessage) {
        continue;
      }
      final payload = message.payload as MqttPublishMessage;
      final text = utf8.decode(payload.payload.message);
      _telemetry.add(parseTelemetry(text));
    }
  }
}
