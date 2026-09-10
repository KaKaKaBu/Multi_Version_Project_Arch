import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import '../../config/version_capabilities.dart';
import '../../services/telemetry_model.dart';
import '../../services/yyyh_service.dart';

class DashboardPage extends StatelessWidget {
  const DashboardPage({
    super.key,
    required this.service,
    required this.capabilities,
  });

  final YyyhService service;
  final VersionCapabilities capabilities;

  @override
  Widget build(BuildContext context) {
    return CommonDashboardPage<YyyhTelemetry>(
      stream: service.onData,
      latestData: service.latestData,
      onInit: service.requestStatus,
      sectionsBuilder: (context, data) => [
        _StatusBanner(data: data, capabilities: capabilities),
        _MedicineGrid(data: data),
        if (capabilities.has('hx711') || capabilities.has('dht11'))
          _SensorGrid(data: data, capabilities: capabilities),
        _TimerSummary(data: data),
      ],
    );
  }
}

class _StatusBanner extends StatelessWidget {
  const _StatusBanner({required this.data, required this.capabilities});

  final YyyhTelemetry data;
  final VersionCapabilities capabilities;

  @override
  Widget build(BuildContext context) {
    final color = data.alarm || data.lowMedicine ? Colors.red : Colors.green;
    final title = data.alarm
        ? '吃药提醒中'
        : data.lowMedicine
            ? '药量不足'
            : '药盒状态正常';
    return Card(
      color: color.withAlpha(30),
      margin: const EdgeInsets.only(bottom: 16),
      child: ListTile(
        leading: Icon(
          data.alarm ? Icons.notifications_active : Icons.verified,
          color: color,
        ),
        title: Text(title),
        subtitle: Text(describeYyyhVersion(capabilities)),
        trailing: Chip(label: Text(data.boxOpen ? '药盒已打开' : '药盒已关闭')),
      ),
    );
  }
}

class _MedicineGrid extends StatelessWidget {
  const _MedicineGrid({required this.data});

  final YyyhTelemetry data;

  @override
  Widget build(BuildContext context) {
    return AdaptiveSectionCard(
      title: '药品库存',
      child: AdaptiveMetricGrid(
        minTileWidth: 150,
        minTileHeight: 110,
        children: [
          for (var index = 1; index <= 3; index++)
            _MetricCard(
              icon: Icons.medication_liquid,
              label: '药品 $index',
              value: '${data.medicineCount(index)} 份',
              active: data.medicineCount(index) <= 0,
            ),
          _MetricCard(
            icon: Icons.sensor_occupied,
            label: '取药检测',
            value: data.taken ? '已取药' : '未触发',
            active: data.taken,
          ),
        ],
      ),
    );
  }
}

class _SensorGrid extends StatelessWidget {
  const _SensorGrid({required this.data, required this.capabilities});

  final YyyhTelemetry data;
  final VersionCapabilities capabilities;

  @override
  Widget build(BuildContext context) {
    return AdaptiveSectionCard(
      title: '扩展数据',
      child: AdaptiveMetricGrid(
        minTileWidth: 150,
        minTileHeight: 110,
        children: [
          if (capabilities.has('hx711'))
            _MetricCard(
              icon: Icons.scale,
              label: '剩余重量',
              value: data.weightG == null
                  ? '--'
                  : '${data.weightG!.toStringAsFixed(1)} g',
              active: data.lowMedicine,
            ),
          if (capabilities.has('dht11'))
            _MetricCard(
              icon: Icons.thermostat,
              label: '温度',
              value: data.temperature == null
                  ? '--'
                  : '${data.temperature!.toStringAsFixed(1)} C',
              active: false,
            ),
          if (capabilities.has('dht11'))
            _MetricCard(
              icon: Icons.water_drop,
              label: '湿度',
              value: data.humidity == null
                  ? '--'
                  : '${data.humidity!.toStringAsFixed(1)} %',
              active: false,
            ),
        ],
      ),
    );
  }
}

class _TimerSummary extends StatelessWidget {
  const _TimerSummary({required this.data});

  final YyyhTelemetry data;

  @override
  Widget build(BuildContext context) {
    final timer = data.selectedTimer;
    return AdaptiveSectionCard(
      title: '当前定时',
      child: AdaptiveMetricGrid(
        minTileWidth: 150,
        minTileHeight: 110,
        children: [
          _MetricCard(
            icon: Icons.alarm,
            label: '定时组',
            value: '第 ${timer.index} 组',
            active: timer.enabled,
          ),
          _MetricCard(
            icon: Icons.schedule,
            label: '时间',
            value: timer.timeText,
            active: timer.enabled,
          ),
          _MetricCard(
            icon: Icons.inventory,
            label: '剂量',
            value: '${timer.dose} 份',
            active: false,
          ),
          _MetricCard(
            icon: Icons.category,
            label: '药品分类',
            value: '药品 ${timer.category + 1}',
            active: false,
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
