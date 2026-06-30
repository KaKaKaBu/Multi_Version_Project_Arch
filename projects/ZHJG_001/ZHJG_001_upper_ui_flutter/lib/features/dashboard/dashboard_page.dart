import 'package:flutter/material.dart';
import '../../core/version/version_capabilities.dart';
import '../../services/telemetry_model.dart';
import '../../services/zhjg_service.dart';

class DashboardPage extends StatefulWidget {
  final ZhjgService service;
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
    return StreamBuilder<ManholeTelemetry>(
      stream: widget.service.onData,
      builder: (context, snapshot) {
        final data = snapshot.data ?? widget.service.latestData;
        return ListView(
          padding: const EdgeInsets.all(16),
          children: [
            _AlarmBanner(data: data),
            const SizedBox(height: 12),
            _SensorCard(
              icon: Icons.local_fire_department,
              label: '沼气浓度',
              value: '${data.methanePpm} ppm',
              limit: '${data.methaneThresholdPpm} ppm',
              active: data.alarms.methane,
            ),
            _SensorCard(
              icon: Icons.water_drop,
              label: '水位',
              value: '${data.waterLevelPercent.toStringAsFixed(0)}%',
              limit: '${data.waterThresholdPercent.toStringAsFixed(0)}%',
              active: data.alarms.water,
            ),
            _SensorCard(
              icon: Icons.screen_rotation_alt,
              label: '井盖倾角',
              value: '${data.tiltDegree.toStringAsFixed(1)}°',
              limit: '${data.tiltThresholdDegree.toStringAsFixed(1)}°',
              active: data.alarms.tilt,
            ),
            const SizedBox(height: 12),
            _ModeRow(mode: data.mode),
            if (widget.capabilities.has('gps')) ...[
              const SizedBox(height: 12),
              _GpsPanel(gps: data.gps),
            ],
          ],
        );
      },
    );
  }
}

class _AlarmBanner extends StatelessWidget {
  final ManholeTelemetry data;

  const _AlarmBanner({required this.data});

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
        title: Text(data.alarm ? '异常告警' : '状态正常'),
        subtitle: Text(
          data.alarm
              ? '存在沼气、水位或倾斜异常'
              : '沼气、水位和井盖姿态均在阈值内',
        ),
      ),
    );
  }
}

class _SensorCard extends StatelessWidget {
  final IconData icon;
  final String label;
  final String value;
  final String limit;
  final bool active;

  const _SensorCard({
    required this.icon,
    required this.label,
    required this.value,
    required this.limit,
    required this.active,
  });

  @override
  Widget build(BuildContext context) {
    final color = active ? Colors.red : Colors.green;
    return Card(
      child: ListTile(
        leading: Icon(icon, color: color, size: 32),
        title: Text(label),
        subtitle: Text('上限 $limit'),
        trailing: Text(
          value,
          style: TextStyle(
            color: color,
            fontSize: 18,
            fontWeight: FontWeight.bold,
          ),
        ),
      ),
    );
  }
}

class _ModeRow extends StatelessWidget {
  final String mode;

  const _ModeRow({required this.mode});

  @override
  Widget build(BuildContext context) {
    return Wrap(
      spacing: 8,
      runSpacing: 8,
      children: [
        Chip(
          avatar: const Icon(Icons.settings, size: 18),
          label: Text(mode == 'threshold' ? '阈值设置模式' : '自动模式'),
        ),
      ],
    );
  }
}

class _GpsPanel extends StatelessWidget {
  final GpsFix gps;

  const _GpsPanel({required this.gps});

  @override
  Widget build(BuildContext context) {
    return Card(
      child: ListTile(
        leading: Icon(
          gps.valid ? Icons.location_on : Icons.location_off,
          color: gps.valid ? Colors.blue : Colors.grey,
        ),
        title: Text(gps.valid ? 'GPS 已定位' : 'GPS 未定位'),
        subtitle: Text(
          '${gps.latitude.toStringAsFixed(6)}, ${gps.longitude.toStringAsFixed(6)}',
        ),
      ),
    );
  }
}
