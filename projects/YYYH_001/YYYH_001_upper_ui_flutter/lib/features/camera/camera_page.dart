import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import '../../services/telemetry_model.dart';
import '../../services/yyyh_service.dart';

class CameraPage extends StatelessWidget {
  const CameraPage({super.key, required this.service});

  final YyyhService service;

  @override
  Widget build(BuildContext context) {
    return CommonDashboardPage<YyyhTelemetry>(
      stream: service.onData,
      latestData: service.latestData,
      onInit: service.requestStatus,
      sectionsBuilder: (context, data) => [
        AdaptiveSectionCard(
          title: '视频监控',
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              MjpegStreamView(url: data.mjpegUrl, height: 260),
              const SizedBox(height: 12),
              Text(
                '视频地址由 ESP32-CAM 上报，STM32 只负责药盒状态和控制命令。',
                style: Theme.of(context).textTheme.bodySmall,
              ),
            ],
          ),
        ),
      ],
    );
  }
}
