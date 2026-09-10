import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import 'config/huawei_iotda_config.dart';
import 'config/mqtt_config.dart';
import 'config/version_capabilities.dart';
import 'features/camera/camera_page.dart';
import 'features/dashboard/dashboard_page.dart';
import 'features/settings/settings_page.dart';
import 'services/yyyh_service.dart';

class Yyyh001App extends StatelessWidget {
  const Yyyh001App({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'YYYH_001 上位机',
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
  late final YyyhService service;

  @override
  void initState() {
    super.initState();
    communication = CommunicationController(
      capabilities: capabilities,
      mqttConfig: defaultMqttConfig,
      huaweiIotdaConfig: defaultHuaweiIotdaConfig,
      bleNamePrefix: 'JDY',
    );
    service = YyyhService(
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
      title: 'YYYH_001 智能药盒',
      capabilities: capabilities,
      controller: communication,
      onConnected: service.requestStatus,
      pages: [
        UpperPageSpec(
          icon: Icons.medication,
          label: '监测',
          child: DashboardPage(service: service, capabilities: capabilities),
        ),
        UpperPageSpec(
          icon: Icons.tune,
          label: '控制',
          child: SettingsPage(service: service, capabilities: capabilities),
        ),
        UpperPageSpec(
          icon: Icons.videocam,
          label: '视频',
          visibleWhen: (capabilities) => capabilities.has('camera'),
          child: CameraPage(service: service),
        ),
      ],
    );
  }
}
