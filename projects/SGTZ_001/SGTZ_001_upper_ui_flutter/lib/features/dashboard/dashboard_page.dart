import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../../services/sgtz_service.dart';
import '../../services/telemetry_model.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

class DashboardPage extends StatefulWidget {
  const DashboardPage({
    super.key,
    required this.service,
    required this.capabilities,
  });

  final SgtzService service;
  final VersionCapabilities capabilities;

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
    return StreamBuilder<ScaleTelemetry>(
      stream: widget.service.onData,
      builder: (context, snapshot) {
        final data = snapshot.data ?? widget.service.latestData;
        return AdaptivePage(
          children: [
            _StatusBanner(data: data, capabilities: widget.capabilities),
            const SizedBox(height: 12),
            _MetricGrid(data: data, capabilities: widget.capabilities),
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
  const _StatusBanner({required this.data, required this.capabilities});

  final ScaleTelemetry data;
  final VersionCapabilities capabilities;

  @override
  Widget build(BuildContext context) {
    final warning =
        data.alarmOn ||
        data.bmiLabel == '过轻' ||
        data.bmiLabel == '过重' ||
        (capabilities.has('fan') && data.fanOn);
    final color = warning ? Colors.orange : Colors.green;
    final title = capabilities.has('fan')
        ? (data.fanOn ? '风扇运行中' : '风扇待机')
        : (data.locked ? '数据已锁定' : '实时测量中');
    final subtitle = capabilities.has('fan')
        ? '${data.manualMode ? "手动" : "自动"}模式，压力 ${data.weightG}g'
        : '体重 ${data.weightKg.toStringAsFixed(1)}kg，BMI ${data.bmiX10 > 0 ? data.bmi.toStringAsFixed(1) : "--"} ${data.bmiLabel}';

    return Card(
      color: color.withAlpha(28),
      child: ListTile(
        leading: Icon(
          warning ? Icons.info_outline : Icons.verified,
          color: color,
        ),
        title: Text(title),
        subtitle: Text(subtitle),
      ),
    );
  }
}

class _MetricGrid extends StatelessWidget {
  const _MetricGrid({required this.data, required this.capabilities});

  final ScaleTelemetry data;
  final VersionCapabilities capabilities;

  @override
  Widget build(BuildContext context) {
    final items = <_MetricItem>[
      if (capabilities.has('weight') || capabilities.has('pressure'))
        _MetricItem(
          icon: Icons.monitor_weight,
          label: capabilities.has('pressure') ? '压力' : '体重',
          value: capabilities.has('pressure')
              ? '${data.weightG} g'
              : '${data.weightKg.toStringAsFixed(1)} kg',
          active: data.weightG > 0,
        ),
      if (capabilities.has('height'))
        _MetricItem(
          icon: Icons.height,
          label: '身高',
          value: data.heightCm > 0
              ? '${data.heightM.toStringAsFixed(2)} m'
              : '--',
          active: data.heightCm > 0,
        ),
      if (capabilities.has('bmi'))
        _MetricItem(
          icon: Icons.accessibility_new,
          label: 'BMI',
          value: data.bmiX10 > 0
              ? '${data.bmi.toStringAsFixed(1)} ${data.bmiLabel}'
              : '--',
          active: data.bmiX10 > 0,
        ),
      if (capabilities.has('fan'))
        _MetricItem(
          icon: Icons.air,
          label: '风扇',
          value: data.fanOn ? '开启' : '关闭',
          active: data.fanOn,
        ),
      if (capabilities.has('fan'))
        _MetricItem(
          icon: Icons.toggle_on,
          label: '模式',
          value: data.manualMode ? '手动' : '自动',
          active: data.manualMode,
        ),
      if (capabilities.has('threshold'))
        _MetricItem(
          icon: Icons.speed,
          label: '报警阈值',
          value: '${data.thresholdG} g',
          active: data.alarmOn,
        ),
      if (capabilities.has('alarm'))
        _MetricItem(
          icon: Icons.notifications_active,
          label: '报警',
          value: data.alarmOn ? '报警中' : '正常',
          active: data.alarmOn,
        ),
      _MetricItem(
        icon: Icons.lock,
        label: '锁定',
        value: data.locked ? 'Lock' : '实时',
        active: data.locked,
      ),
      if (capabilities.has('voice'))
        _MetricItem(
          icon: Icons.record_voice_over,
          label: '语音',
          value: data.voicePaused ? '暂停' : '启用',
          active: !data.voicePaused,
        ),
    ];

    return LayoutBuilder(
      builder: (context, constraints) {
        return AdaptiveMetricGrid(
          minTileWidth: constraints.maxWidth < 420 ? 240 : 180,
          singleColumnAspectRatio: 2.9,
          multiColumnAspectRatio: constraints.maxWidth < 520 ? 1.55 : 1.9,
          children: [
            for (final item in items) _MetricCard(item: item),
          ],
        );
      },
    );
  }
}

class _MetricItem {
  const _MetricItem({
    required this.icon,
    required this.label,
    required this.value,
    required this.active,
  });

  final IconData icon;
  final String label;
  final String value;
  final bool active;
}

class _MetricCard extends StatelessWidget {
  const _MetricCard({required this.item});

  final _MetricItem item;

  @override
  Widget build(BuildContext context) {
    final color = item.active
        ? Theme.of(context).colorScheme.primary
        : Colors.grey;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(14),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(item.icon, color: color),
            const SizedBox(height: 8),
            Text(
              item.label,
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
              style: Theme.of(context).textTheme.bodyMedium,
            ),
            const SizedBox(height: 4),
            Text(
              item.value,
              maxLines: 2,
              overflow: TextOverflow.ellipsis,
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                color: color,
                fontWeight: FontWeight.bold,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class _CameraPanel extends StatelessWidget {
  const _CameraPanel({required this.data});

  final ScaleTelemetry data;

  @override
  Widget build(BuildContext context) {
    final stream = data.camera.streamUrl;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const ListTile(
              contentPadding: EdgeInsets.zero,
              leading: Icon(Icons.videocam),
              title: Text('视频监控'),
              subtitle: Text('V6 外部 ESP32-CAM 通过 MQTT 上报局域网 MJPEG 地址'),
            ),
            SelectableText(
              stream.isEmpty
                  ? '等待摄像头上报 cam_ip / mjpeg_url'
                  : '${data.camera.deviceId}\n${data.camera.ip}\n$stream',
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
                      ScaffoldMessenger.of(
                        context,
                      ).showSnackBar(const SnackBar(content: Text('摄像头地址已复制')));
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
