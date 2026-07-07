import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import '../../config/version_capabilities.dart';
import '../../services/kqzl2_service.dart';
import '../../services/telemetry_model.dart';

class DashboardPage extends StatelessWidget {
  const DashboardPage({
    super.key,
    required this.service,
    required this.capabilities,
  });

  final Kqzl2Service service;
  final VersionCapabilities capabilities;

  @override
  Widget build(BuildContext context) {
    return CommonDashboardPage<AirQualityTelemetry>(
      stream: service.onData,
      latestData: service.latestData,
      onInit: service.requestStatus,
      sectionsBuilder: (context, data) => [
        _StatusBanner(data: data, capabilities: capabilities),
        const SizedBox(height: 12),
        _SensorGrid(data: data, capabilities: capabilities),
        const SizedBox(height: 12),
        _ActuatorGrid(data: data),
      ],
    );
  }
}

class _StatusBanner extends StatelessWidget {
  const _StatusBanner({required this.data, required this.capabilities});

  final AirQualityTelemetry data;
  final VersionCapabilities capabilities;

  @override
  Widget build(BuildContext context) {
    final color = data.alarm ? Colors.red : Colors.green;
    return Card(
      color: color.withAlpha(30),
      child: ListTile(
        leading: Icon(
          data.alarm ? Icons.warning_amber : Icons.verified,
          color: color,
        ),
        title: Text(data.alarm ? '空气指标超限' : '空气状态正常'),
        subtitle: Text('${data.modeLabel} · ${describeKqzl2Version(capabilities)}'),
      ),
    );
  }
}

class _SensorGrid extends StatelessWidget {
  const _SensorGrid({required this.data, required this.capabilities});

  final AirQualityTelemetry data;
  final VersionCapabilities capabilities;

  @override
  Widget build(BuildContext context) {
    final sensors = airSensorCatalog
        .where((sensor) => sensorSupported(capabilities, sensor.feature))
        .toList();
    return AdaptiveSectionCard(
      title: '实时数据',
      child: AdaptiveMetricGrid(
        minTileWidth: 168,
        children: [
          for (final sensor in sensors)
            _MetricCard(
              icon: _sensorIcon(sensor.key),
              label: sensor.label,
              value: _formatSensorValue(data, sensor),
              active: _isSensorOverThreshold(data, sensor),
            ),
        ],
      ),
    );
  }

  static String _formatSensorValue(
    AirQualityTelemetry data,
    AirSensorSpec sensor,
  ) {
    final value = data.sensorValue(sensor.key);
    if (value == null) return '--';
    return '$value ${sensor.unit}';
  }

  static bool _isSensorOverThreshold(
    AirQualityTelemetry data,
    AirSensorSpec sensor,
  ) {
    final value = data.sensorValue(sensor.key);
    final threshold = data.thresholds[sensor.thresholdKey];
    return value != null && threshold != null && value > threshold;
  }

  static IconData _sensorIcon(String key) {
    return switch (key) {
      'temp' => Icons.thermostat,
      'humidity' => Icons.water_drop,
      'smoke' => Icons.smoke_free,
      'co' => Icons.gas_meter,
      _ => Icons.air,
    };
  }
}

class _ActuatorGrid extends StatelessWidget {
  const _ActuatorGrid({required this.data});

  final AirQualityTelemetry data;

  @override
  Widget build(BuildContext context) {
    return AdaptiveSectionCard(
      title: '执行器状态',
      child: AdaptiveMetricGrid(
        minTileWidth: 160,
        children: [
          for (final actuator in airActuatorCatalog)
            _MetricCard(
              icon: _actuatorIcon(actuator.key),
              label: actuator.label,
              value: data.actuatorState(actuator.key) ? '开启' : '关闭',
              active: data.actuatorState(actuator.key),
            ),
          _MetricCard(
            icon: Icons.settings,
            label: '模式',
            value: data.modeLabel,
            active: data.mode != 'auto',
          ),
        ],
      ),
    );
  }

  static IconData _actuatorIcon(String key) {
    return switch (key) {
      'buzzer' => Icons.notifications_active,
      'light' => Icons.lightbulb,
      _ => Icons.mode_fan_off,
    };
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
        padding: const EdgeInsets.all(14),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(icon, color: color),
            const SizedBox(height: 8),
            Text(label, maxLines: 1, overflow: TextOverflow.ellipsis),
            const SizedBox(height: 4),
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
