import '../../core/models/kqzl3_models.dart';
import '../transport/upper_transport.dart';

class MqttUpperTransport extends UpperTransport {
  MqttUpperTransport({required this.config, required this.clientId});

  final ConnectionConfig config;
  final String clientId;

  @override
  Stream<Object?> get telemetryStream => const Stream.empty();

  @override
  Stream<String> get statusStream => const Stream.empty();

  @override
  Stream<String> get errorStream => const Stream.empty();

  @override
  Future<void> connect() async {
    throw UnsupportedError('当前平台暂不支持 MQTT 连接');
  }

  @override
  Future<void> sendCommand(Map<String, dynamic> command) async {
    throw UnsupportedError('当前平台暂不支持 MQTT 连接');
  }

  @override
  Future<void> disconnect() async {}
}
