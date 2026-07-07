import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import 'config/mqtt_config.dart';
import 'config/version_capabilities.dart';
import 'features/dashboard/dashboard_page.dart';
import 'features/settings/settings_page.dart';
import 'services/sgtz_service.dart';

class Sgtz001App extends StatelessWidget {
  const Sgtz001App({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'SGTZ_001 上位机',
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
  late final SgtzService service;

  @override
  void initState() {
    super.initState();
    communication = CommunicationController(
      capabilities: capabilities,
      mqttConfig: defaultMqttConfig,
    );
    service = SgtzService(
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
      title: 'SGTZ_001 电子秤',
      capabilities: capabilities,
      controller: communication,
      onConnected: service.requestStatus,
      pages: [
        UpperPageSpec(
          icon: Icons.monitor_weight,
          label: '监测',
          child: DashboardPage(service: service, capabilities: capabilities),
        ),
        UpperPageSpec(
          icon: Icons.tune,
          label: '控制',
          child: SettingsPage(service: service, capabilities: capabilities),
        ),
      ],
    );
  }
}
