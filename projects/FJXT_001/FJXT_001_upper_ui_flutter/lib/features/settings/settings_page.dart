import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import '../../services/fjxt_service.dart';

class SettingsPage extends StatelessWidget {
  final FjxtService service;

  const SettingsPage({super.key, required this.service});

  @override
  Widget build(BuildContext context) {
    return CommonControlPage(
      title: '车窗控制',
      primaryActions: [
        DeviceCommandAction(
          icon: Icons.keyboard_double_arrow_up,
          label: '一键打开',
          onPressed: service.openFull,
        ),
        DeviceCommandAction(
          icon: Icons.keyboard_double_arrow_down,
          label: '一键关闭',
          onPressed: service.closeFull,
        ),
        DeviceCommandAction(
          icon: Icons.keyboard_arrow_up,
          label: '开一点停',
          onPressed: service.openStep,
        ),
        DeviceCommandAction(
          icon: Icons.keyboard_arrow_down,
          label: '关一点停',
          onPressed: service.closeStep,
        ),
      ],
      secondaryActions: [
        DeviceCommandAction(
          icon: Icons.stop_circle,
          label: '停止',
          onPressed: service.stop,
        ),
        DeviceCommandAction(
          icon: Icons.refresh,
          label: '刷新状态',
          onPressed: service.requestStatus,
        ),
      ],
    );
  }
}
