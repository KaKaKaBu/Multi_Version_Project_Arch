import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import 'config/mqtt_config.dart';
import 'config/version_capabilities.dart';
import 'features/dashboard/dashboard_page.dart';
import 'features/settings/settings_page.dart';
import 'services/zhjg_service.dart';

class Zhjg001App extends StatelessWidget {
  const Zhjg001App({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'ZHJG_001 上位机',
      debugShowCheckedModeBanner: false,
      theme: MvpAppTheme.light(),
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
  late final VersionCapabilities capabilities;
  late final CommunicationController communication;
  late final ZhjgService service;

  @override
  void initState() {
    super.initState();
    capabilities = resolveProjectUpperFeatures();
    communication = CommunicationController(
      capabilities: capabilities,
      mqttConfig: defaultMqttConfig,
      bleNamePrefix: 'JDY',
    );
    service = ZhjgService(
      transport: communication.transport,
      capabilities: capabilities,
    );
  }

  @override
  void dispose() {
    service.dispose();
    communication.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return CommonHomeShell(
      title: 'ZHJG_001 智能井盖',
      capabilities: capabilities,
      controller: communication,
      onConnected: service.requestStatus,
      pages: [
        UpperPageSpec(
          icon: Icons.monitor_heart,
          label: '监测',
          child: DashboardPage(service: service, capabilities: capabilities),
        ),
        UpperPageSpec(
          icon: Icons.tune,
          label: '阈值',
          child: SettingsPage(service: service),
        ),
      ],
    );
  }
}
