import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import 'config/mqtt_config.dart';
import 'config/version_capabilities.dart';
import 'features/dashboard/dashboard_page.dart';
import 'features/settings/settings_page.dart';
import 'services/smzl_service.dart';

class Smzl001App extends StatelessWidget {
  const Smzl001App({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'SMZL_001 上位机',
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
  final capabilities = resolveProjectUpperFeatures();
  late final CommunicationController communication;
  late final SmzlService service;

  @override
  void initState() {
    super.initState();
    communication = CommunicationController(
      capabilities: capabilities,
      mqttConfig: defaultMqttConfig,
    );
    service = SmzlService(
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
      title: 'SMZL_001 睡眠监测',
      capabilities: capabilities,
      controller: communication,
      onConnected: service.requestStatus,
      pages: [
        UpperPageSpec(
          icon: Icons.monitor_heart,
          label: '监测',
          child: SmzlDashboard(service: service),
        ),
        UpperPageSpec(
          icon: Icons.tune,
          label: '控制',
          child: SmzlSettings(service: service),
        ),
      ],
    );
  }
}
