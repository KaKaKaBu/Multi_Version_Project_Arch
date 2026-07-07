import 'dart:async';
import 'dart:convert';

import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

import '../config/huawei_iotda_config.dart';
import 'transport_service.dart';

class HuaweiIotdaTransport extends TransportService {
  HuaweiIotdaTransport(this.config);

  final HuaweiIotdaConfig config;
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
    final credential = config.createCredential();
    final client = MqttServerClient.withPort(
      config.websocketUrl,
      credential.clientId,
      config.port,
    );
    client.useWebSocket = true;
    client.websocketProtocols = MqttClientConstants.protocolsSingleDefault;
    client.keepAlivePeriod = config.keepAliveSeconds;
    client.connectTimeoutPeriod = 10000;
    client.logging(on: true);
    client.connectionMessage = MqttConnectMessage()
        .withClientIdentifier(credential.clientId)
        .startClean()
        .withWillQos(MqttQos.atMostOnce);
    client.onConnected = () => _connectionController.add(true);
    client.onDisconnected = () => _connectionController.add(false);

    try {
      await client.connect(credential.username, credential.password);
    } catch (error) {
      client.disconnect();
      _connectionController.add(false);
      throw StateError(
        '华为云 MQTT 连接异常: $error; '
        'url=${config.websocketUrl}, clientId=${credential.clientId}, '
        'username=${credential.username}, sub=${config.appCustomSubscribeTopic}, '
        'pub=${config.appCustomPublishTopic}',
      );
    }
    if (client.connectionStatus?.state != MqttConnectionState.connected) {
      final status = client.connectionStatus;
      client.disconnect();
      _connectionController.add(false);
      throw StateError(
        '华为云 MQTT 连接失败: returnCode=${status?.returnCode}, '
        'state=${status?.state}; url=${config.websocketUrl}, '
        'clientId=${credential.clientId}, username=${credential.username}',
      );
    }

    client.subscribe(config.appCustomSubscribeTopic, MqttQos.atMostOnce);
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

    final payload = MqttClientPayloadBuilder()
      ..addUTF8String(jsonEncode({'message': _decodeObject(message)}));
    client.publishMessage(
      config.appCustomPublishTopic,
      MqttQos.atLeastOnce,
      payload.payload!,
    );
  }

  void _handleMessages(List<MqttReceivedMessage<MqttMessage>> messages) {
    for (final message in messages) {
      if (message.payload is! MqttPublishMessage) continue;
      final payload = message.payload as MqttPublishMessage;
      final text = utf8.decode(payload.payload.message);
      final properties = <String, dynamic>{};
      _collectProperties(_decodeValue(text), properties);
      if (properties.isEmpty) {
        _messageController.add(text);
      } else {
        _messageController.add(jsonEncode({
          'type': 'telemetry',
          'data': properties,
        }));
      }
    }
  }

  Map<String, dynamic> _decodeObject(String text) {
    try {
      final decoded = jsonDecode(text);
      if (decoded is Map<String, dynamic>) return decoded;
      if (decoded is Map) return Map<String, dynamic>.from(decoded);
    } catch (_) {}
    return {'raw': text};
  }

  Object? _decodeValue(Object? value) {
    if (value is String) {
      try {
        return jsonDecode(value.trim());
      } catch (_) {
        return value;
      }
    }
    return value;
  }

  void _collectProperties(Object? value, Map<String, dynamic> data) {
    value = _decodeValue(value);
    if (value == null) return;

    if (value is List) {
      for (final item in value) {
        _collectProperties(item, data);
      }
      return;
    }

    if (value is! Map) return;
    final map = Map<String, dynamic>.from(value);

    for (final key in const ['message', 'params', 'properties']) {
      final child = _decodeValue(map[key]);
      if (child is Map) data.addAll(Map<String, dynamic>.from(child));
    }

    final services = map['services'];
    if (services is List) {
      for (final service in services) {
        _collectProperties(service, data);
      }
    }

    for (final entry in map.entries) {
      if (entry.key == 'message' ||
          entry.key == 'params' ||
          entry.key == 'properties' ||
          entry.key == 'services') {
        continue;
      }
      _collectProperties(entry.value, data);
    }
  }

  void dispose() {
    disconnect();
    _messageController.close();
    _connectionController.close();
  }
}
