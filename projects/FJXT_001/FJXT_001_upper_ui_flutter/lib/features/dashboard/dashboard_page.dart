import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import '../../services/fjxt_service.dart';
import '../../services/telemetry_model.dart';

class DashboardPage extends StatelessWidget {
  final FjxtService service;
  final VersionCapabilities capabilities;

  const DashboardPage({
    super.key,
    required this.service,
    required this.capabilities,
  });

  @override
  Widget build(BuildContext context) {
    return CommonDashboardPage<WindowTelemetry>(
      stream: service.onData,
      latestData: service.latestData,
      onInit: service.requestStatus,
      sectionsBuilder: (context, data) => [
        _StatusBanner(data: data),
        const SizedBox(height: 12),
        _StateCard(
          icon: Icons.window,
          label: '车窗状态',
          value: data.state,
          active: data.state == 'Opening' || data.state == 'Closing',
        ),
        _StateCard(
          icon: Icons.pan_tool,
          label: '红外对管防夹',
          value: data.pinch ? '检测到有物/手' : '未触发',
          active: data.pinch,
        ),
        if (capabilities.has('camera')) ...[
          const SizedBox(height: 12),
          _CameraPanel(data: data),
        ],
      ],
    );
  }
}

class _StatusBanner extends StatelessWidget {
  final WindowTelemetry data;

  const _StatusBanner({required this.data});

  @override
  Widget build(BuildContext context) {
    final color = data.alarm ? Colors.red : Colors.green;
    return Card(
      color: color.withAlpha(28),
      child: ListTile(
        leading: Icon(
          data.alarm ? Icons.warning_amber : Icons.verified,
          color: color,
        ),
        title: Text(data.alarm ? '声光提醒中' : '系统待命'),
        subtitle: Text(data.pinch ? '防夹已触发，正在反向打开' : '车窗控制状态正常'),
      ),
    );
  }
}

class _StateCard extends StatelessWidget {
  final IconData icon;
  final String label;
  final String value;
  final bool active;

  const _StateCard({
    required this.icon,
    required this.label,
    required this.value,
    required this.active,
  });

  @override
  Widget build(BuildContext context) {
    final color = active ? Theme.of(context).colorScheme.primary : Colors.grey;
    return Card(
      child: ListTile(
        leading: Icon(icon, color: color, size: 30),
        title: Text(label),
        trailing: Text(
          value,
          style: TextStyle(color: color, fontWeight: FontWeight.bold),
        ),
      ),
    );
  }
}

class _CameraPanel extends StatelessWidget {
  final WindowTelemetry data;

  const _CameraPanel({required this.data});

  @override
  Widget build(BuildContext context) {
    final stream = data.cameraStream;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const ListTile(
              contentPadding: EdgeInsets.zero,
              leading: Icon(Icons.videocam),
              title: Text('ESP-CAM 视频监控'),
              subtitle: Text('Android 内嵌播放 HTTP MJPEG，需手机与摄像头同一局域网'),
            ),
            Text(
              stream.isEmpty ? '等待 ESP-CAM 上报局域网地址和 stream 链接' : '${data.cameraIp}\n$stream',
              style: const TextStyle(fontFamily: 'monospace'),
            ),
            const SizedBox(height: 12),
            MjpegStreamView(url: stream),
            const SizedBox(height: 12),
            OutlinedButton.icon(
              onPressed: stream.isEmpty
                  ? null
                  : () {
                      Clipboard.setData(ClipboardData(text: stream));
                      ScaffoldMessenger.of(context).showSnackBar(
                        const SnackBar(content: Text('摄像头地址已复制')),
                      );
                    },
              icon: const Icon(Icons.copy),
              label: const Text('复制地址'),
            ),
          ],
        ),
      ),
    );
  }
}
