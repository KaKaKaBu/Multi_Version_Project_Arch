import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import '../../config/version_capabilities.dart';
import '../../services/aqmj_service.dart';
import '../../services/telemetry_model.dart';

class DashboardPage extends StatelessWidget {
  const DashboardPage({
    super.key,
    required this.service,
    required this.capabilities,
  });

  final AqmjService service;
  final VersionCapabilities capabilities;

  @override
  Widget build(BuildContext context) {
    return CommonDashboardPage<AqmjTelemetry>(
      stream: service.onData,
      latestData: service.latestData,
      onInit: service.requestStatus,
      sectionsBuilder: (context, data) => [
        _StatusBanner(data: data, capabilities: capabilities),
        _DoorGrid(data: data, capabilities: capabilities),
        if (capabilities.has('light') || capabilities.has('message'))
          _ExtensionGrid(data: data, capabilities: capabilities),
      ],
    );
  }
}

class _StatusBanner extends StatelessWidget {
  const _StatusBanner({required this.data, required this.capabilities});

  final AqmjTelemetry data;
  final VersionCapabilities capabilities;

  @override
  Widget build(BuildContext context) {
    final color = data.alarm ? Colors.red : Colors.green;
    final title = data.alarm
        ? '门前逗留报警'
        : data.presence
            ? '门前有人'
            : '门禁状态正常';
    return Card(
      color: color.withAlpha(30),
      margin: const EdgeInsets.only(bottom: 16),
      child: ListTile(
        leading: Icon(
          data.alarm ? Icons.warning_amber : Icons.verified_user,
          color: color,
        ),
        title: Text(title),
        subtitle: Text(describeAqmjVersion(capabilities)),
        trailing: Chip(label: Text(data.armed ? '设防' : '撤防')),
      ),
    );
  }
}

class _DoorGrid extends StatelessWidget {
  const _DoorGrid({required this.data, required this.capabilities});

  final AqmjTelemetry data;
  final VersionCapabilities capabilities;

  @override
  Widget build(BuildContext context) {
    return AdaptiveSectionCard(
      title: '门禁状态',
      child: AdaptiveMetricGrid(
        minTileWidth: 150,
        minTileHeight: 110,
        children: [
          _MetricCard(
            icon: Icons.home,
            label: '主人状态',
            value: data.home ? '在家' : '外出',
            active: data.home,
          ),
          _MetricCard(
            icon: Icons.security,
            label: '安全模式',
            value: data.armed ? '设防中' : '已撤防',
            active: data.armed,
          ),
          _MetricCard(
            icon: Icons.sensor_occupied,
            label: '人体检测',
            value: data.presence ? '有人' : '无人',
            active: data.presence,
          ),
          _MetricCard(
            icon: Icons.notifications,
            label: '最近门铃',
            value: data.lastCallText,
            active: data.lastCallHour != 0 || data.lastCallMinute != 0,
          ),
        ],
      ),
    );
  }
}

class _ExtensionGrid extends StatelessWidget {
  const _ExtensionGrid({required this.data, required this.capabilities});

  final AqmjTelemetry data;
  final VersionCapabilities capabilities;

  @override
  Widget build(BuildContext context) {
    return AdaptiveSectionCard(
      title: '扩展状态',
      child: AdaptiveMetricGrid(
        minTileWidth: 150,
        minTileHeight: 110,
        children: [
          if (capabilities.has('light'))
            _MetricCard(
              icon: Icons.light_mode,
              label: '光照值',
              value: '${data.light}',
              active: data.light < 35,
            ),
          if (capabilities.has('light'))
            _MetricCard(
              icon: Icons.lightbulb,
              label: '感应灯',
              value: data.lamp ? '已开启' : '已关闭',
              active: data.lamp,
            ),
          if (capabilities.has('message'))
            _MetricCard(
              icon: Icons.voicemail,
              label: '访客留言',
              value: data.message ? '有留言' : '无留言',
              active: data.message,
            ),
          if (capabilities.has('camera'))
            _MetricCard(
              icon: Icons.videocam,
              label: '视频监控',
              value: data.video ? '在线' : '未上报',
              active: data.video,
            ),
        ],
      ),
    );
  }
}

class _MetricCard extends StatelessWidget {
  const _MetricCard({
    required this.icon,
    required this.label,
    required this.value,
    required this.active,
  });

  final IconData icon;
  final String label;
  final String value;
  final bool active;

  @override
  Widget build(BuildContext context) {
    final color = active
        ? Theme.of(context).colorScheme.primary
        : Theme.of(context).colorScheme.onSurfaceVariant;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(icon, color: color, size: 22),
            const SizedBox(height: 6),
            Text(label, maxLines: 1, overflow: TextOverflow.ellipsis),
            const SizedBox(height: 3),
            Text(
              value,
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
