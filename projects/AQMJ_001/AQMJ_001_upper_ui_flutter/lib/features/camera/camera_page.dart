import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import '../../services/aqmj_service.dart';
import '../../services/telemetry_model.dart';

class CameraPage extends StatelessWidget {
  const CameraPage({super.key, required this.service});

  final AqmjService service;

  @override
  Widget build(BuildContext context) {
    return CommonDashboardPage<AqmjTelemetry>(
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
                '视频地址由摄像头模块上报，STM32 负责门禁状态、报警和控制命令。',
                style: Theme.of(context).textTheme.bodySmall,
              ),
            ],
          ),
        ),
      ],
    );
  }
}
