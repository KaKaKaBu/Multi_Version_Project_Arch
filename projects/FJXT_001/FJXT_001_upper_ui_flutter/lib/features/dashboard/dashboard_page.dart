import 'package:flutter/material.dart';
import '../../core/version/version_capabilities.dart';
import '../../services/fjxt_service.dart';
import '../../services/telemetry_model.dart';

class DashboardPage extends StatefulWidget {
  final FjxtService service;
  final VersionCapabilities capabilities;

  const DashboardPage({
    super.key,
    required this.service,
    required this.capabilities,
  });

  @override
  State<DashboardPage> createState() => _DashboardPageState();
}

class _DashboardPageState extends State<DashboardPage> {
  @override
  void initState() {
    super.initState();
    widget.service.requestStatus();
  }

  @override
  Widget build(BuildContext context) {
    return StreamBuilder<WindowTelemetry>(
      stream: widget.service.onData,
      builder: (context, snapshot) {
        final data = snapshot.data ?? widget.service.latestData;
        return ListView(
          padding: const EdgeInsets.all(16),
          children: [
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
              label: '防夹检测',
              value: data.pinch ? '检测到有物/手' : '未触发',
              active: data.pinch,
            ),
            _StateCard(
              icon: Icons.vertical_align_top,
              label: '打开到位',
              value: data.openLimit ? '已到位' : '未到位',
              active: data.openLimit,
            ),
            _StateCard(
              icon: Icons.vertical_align_bottom,
              label: '关闭到位',
              value: data.closeLimit ? '已到位' : '未到位',
              active: data.closeLimit,
            ),
            if (widget.capabilities.has('camera')) ...[
              const SizedBox(height: 12),
              _CameraPanel(data: data),
            ],
          ],
        );
      },
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
        leading: Icon(data.alarm ? Icons.warning_amber : Icons.verified, color: color),
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
    return Card(
      child: ListTile(
        leading: const Icon(Icons.videocam),
        title: const Text('ESP-CAM'),
        subtitle: Text(
          data.cameraStream.isEmpty
              ? '等待 ESP-CAM 上报局域网地址和 stream 链接'
              : '${data.cameraIp}\n${data.cameraStream}',
        ),
      ),
    );
  }
}
