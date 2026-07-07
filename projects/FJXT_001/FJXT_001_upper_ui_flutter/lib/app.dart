import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import 'config/huawei_iotda_config.dart';
import 'config/mqtt_config.dart';
import 'features/dashboard/dashboard_page.dart';
import 'features/settings/settings_page.dart';
import 'services/fjxt_service.dart';

class Fjxt001App extends StatelessWidget {
  const Fjxt001App({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'FJXT_001 上位机',
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
  final capabilities = resolveUpperFeatures();
  late final CommunicationController communication;
  late final FjxtService service;

  @override
  void initState() {
    super.initState();
    communication = CommunicationController(
      capabilities: capabilities,
      mqttConfig: defaultMqttConfig,
      huaweiIotdaConfig: defaultHuaweiIotdaConfig,
    );
    service = FjxtService(
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
      title: 'FJXT_001 车窗防夹',
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
          label: '控制',
          child: SettingsPage(service: service),
        ),
      ],
    );
  }
}
