import 'dart:async';
import 'package:flutter/material.dart';
import 'core/config/mqtt_config.dart';
import 'core/transport/bluetooth_spp_transport.dart';
import 'core/transport/mqtt_transport.dart';
import 'core/transport/transport_service.dart';
import 'core/version/version_capabilities.dart';
import 'services/smzl_service.dart';
import 'features/dashboard/dashboard_page.dart';
import 'features/settings/settings_page.dart';

class Smzl001App extends StatelessWidget {
  const Smzl001App({super.key});
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'SMZL_001 上位机',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorSchemeSeed: const Color(0xFF1565C0),
        useMaterial3: true,
      ),
      home: const HomePage(),
    );
  }
}

class HomePage extends StatefulWidget {
  const HomePage({super.key});
  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  final capabilities = resolveUpperFeatures();
  final transport = ActiveTransportService();
  final _bleAddressController = TextEditingController();
  late final SmzlService service;
  StreamSubscription? _connSub;
  bool _connected = false;
  bool _busy = false;
  String _transportLabel = '未连接';
  String? _error;
  int _idx = 0;

  @override
  void initState() {
    super.initState();
    service = SmzlService(transport: transport, capabilities: capabilities);
    _connSub = transport.onConnectionChanged.listen(
      (c) => mounted ? setState(() => _connected = c) : null,
    );
  }

  @override
  void dispose() {
    _connSub?.cancel();
    _bleAddressController.dispose();
    service.dispose();
    transport.dispose();
    super.dispose();
  }

  Future<void> _connectMqtt() async =>
      _connect('MQTT', MqttTransport(defaultMqttConfig));
  Future<void> _connectBle() async => _connect(
    'BLE',
    BluetoothSppTransport(address: _bleAddressController.text.trim()),
  );

  Future<void> _connect(String label, TransportService nextTransport) async {
    setState(() {
      _busy = true;
      _error = null;
    });
    try {
      final ok = await transport.connectWith(nextTransport, const {});
      if (mounted) {
        setState(() {
          _transportLabel = ok ? label : '未连接';
          _error = ok ? null : '$label 连接失败';
        });
      }
      if (ok) service.requestStatus();
    } catch (error) {
      await transport.disconnect();
      if (mounted) {
        setState(() {
          _transportLabel = '未连接';
          _error = error.toString();
        });
      }
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  Future<void> _disconnect() async {
    await transport.disconnect();
    if (mounted) setState(() => _transportLabel = '未连接');
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('SMZL_001'),
        actions: [
          IconButton(
            icon: Icon(_connected ? Icons.link : Icons.link_off),
            onPressed: _connected ? _disconnect : null,
          ),
        ],
      ),
      body: Column(
        children: [
          Container(
            padding: const EdgeInsets.all(8),
            color: Theme.of(context).colorScheme.surfaceContainerHighest,
            child: Text(
              'V${capabilities.version}  ${capabilities.features.join(", ")}',
              style: Theme.of(context).textTheme.bodySmall,
              textAlign: TextAlign.center,
            ),
          ),
          _ConnectionPanel(
            connected: _connected,
            busy: _busy,
            label: _transportLabel,
            error: _error,
            mqttEnabled: capabilities.has('wifi'),
            bleEnabled: capabilities.has('ble'),
            bleAddressController: _bleAddressController,
            onConnectMqtt: _connectMqtt,
            onConnectBle: _connectBle,
            onDisconnect: _disconnect,
          ),
          Expanded(
            child: IndexedStack(
              index: _idx,
              children: [
                SmzlDashboard(service: service),
                SmzlSettings(service: service),
              ],
            ),
          ),
        ],
      ),
      bottomNavigationBar: NavigationBar(
        selectedIndex: _idx,
        onDestinationSelected: (i) => setState(() => _idx = i),
        destinations: const [
          NavigationDestination(
            icon: Icon(Icons.dashboard),
            label: 'Dashboard',
          ),
          NavigationDestination(icon: Icon(Icons.tune), label: 'Settings'),
        ],
      ),
    );
  }
}

class _ConnectionPanel extends StatelessWidget {
  const _ConnectionPanel({
    required this.connected,
    required this.busy,
    required this.label,
    required this.error,
    required this.mqttEnabled,
    required this.bleEnabled,
    required this.bleAddressController,
    required this.onConnectMqtt,
    required this.onConnectBle,
    required this.onDisconnect,
  });

  final bool connected;
  final bool busy;
  final String label;
  final String? error;
  final bool mqttEnabled;
  final bool bleEnabled;
  final TextEditingController bleAddressController;
  final VoidCallback onConnectMqtt;
  final VoidCallback onConnectBle;
  final VoidCallback onDisconnect;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(16, 10, 16, 12),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          Wrap(
            spacing: 10,
            runSpacing: 10,
            crossAxisAlignment: WrapCrossAlignment.center,
            children: [
              Chip(
                avatar: Icon(
                  connected ? Icons.check_circle : Icons.radio_button_unchecked,
                  size: 18,
                ),
                label: Text(label),
              ),
              FilledButton.icon(
                onPressed: !busy && mqttEnabled ? onConnectMqtt : null,
                icon: const Icon(Icons.cloud),
                label: const Text('MQTT'),
              ),
              FilledButton.tonalIcon(
                onPressed: !busy && bleEnabled ? onConnectBle : null,
                icon: const Icon(Icons.bluetooth),
                label: const Text('BLE'),
              ),
              OutlinedButton.icon(
                onPressed: connected && !busy ? onDisconnect : null,
                icon: const Icon(Icons.link_off),
                label: const Text('断开'),
              ),
            ],
          ),
          if (bleEnabled) ...[
            const SizedBox(height: 8),
            TextField(
              controller: bleAddressController,
              decoration: const InputDecoration(
                labelText: 'BLE MAC 地址（可选）',
                hintText: '留空自动连接已配对 JDY 设备',
                isDense: true,
                border: OutlineInputBorder(),
              ),
            ),
          ],
          if (error != null) ...[
            const SizedBox(height: 8),
            Text(
              error!,
              style: TextStyle(color: Theme.of(context).colorScheme.error),
            ),
          ],
        ],
      ),
    );
  }
}
