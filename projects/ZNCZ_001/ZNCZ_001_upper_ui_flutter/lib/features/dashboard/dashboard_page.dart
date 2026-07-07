import 'package:flutter/material.dart';

import '../../config/version_capabilities.dart';
import '../../services/telemetry_model.dart';
import '../../services/zncz_service.dart';

class DashboardPage extends StatefulWidget {
  const DashboardPage({
    super.key,
    required this.service,
    required this.capabilities,
  });

  final ZnczService service;
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
    return StreamBuilder<SocketTelemetry>(
      stream: widget.service.onData,
      builder: (context, snapshot) {
        final data = snapshot.data ?? widget.service.latestData;
        return ListView(
          padding: const EdgeInsets.all(16),
          children: [
            _StatusBanner(data: data),
            const SizedBox(height: 12),
            _MetricGrid(data: data),
          ],
        );
      },
    );
  }
}

class _StatusBanner extends StatelessWidget {
  const _StatusBanner({required this.data});

  final SocketTelemetry data;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final color = data.relayOn ? scheme.primary : scheme.outline;
    return Card(
      color: data.relayOn ? scheme.primaryContainer : scheme.surfaceContainerHigh,
      child: Padding(
        padding: const EdgeInsets.all(18),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Wrap(
              spacing: 10,
              runSpacing: 10,
              crossAxisAlignment: WrapCrossAlignment.center,
              children: [
                Icon(data.relayOn ? Icons.power : Icons.power_off, color: color),
                Text(
                  data.relayOn ? '继电器已开启' : '继电器已关闭',
                  style: Theme.of(context).textTheme.titleLarge?.copyWith(
                    color: color,
                    fontWeight: FontWeight.w700,
                  ),
                ),
                Chip(
                  avatar: Icon(
                    data.timerMode ? Icons.timer : Icons.touch_app,
                    size: 18,
                  ),
                  label: Text(data.timerMode ? '定时模式' : '手动模式'),
                ),
                Chip(
                  avatar: Icon(
                    data.wifiConnected ? Icons.wifi : Icons.wifi_off,
                    size: 18,
                  ),
                  label: Text(data.wifiConnected ? 'WiFi 在线' : data.wifi),
                ),
              ],
            ),
            const SizedBox(height: 18),
            Text(
              data.time,
              style: Theme.of(context).textTheme.displaySmall?.copyWith(
                fontWeight: FontWeight.w700,
                letterSpacing: 0,
              ),
            ),
            const SizedBox(height: 6),
            Text('ON ${data.onTime}   OFF ${data.offTime}'),
          ],
        ),
      ),
    );
  }
}

class _MetricGrid extends StatelessWidget {
  const _MetricGrid({required this.data});

  final SocketTelemetry data;

  @override
  Widget build(BuildContext context) {
    final items = <_MetricItem>[
      _MetricItem(
        icon: Icons.power_settings_new,
        label: '继电器',
        value: data.relayOn ? 'ON' : 'OFF',
        active: data.relayOn,
      ),
      _MetricItem(
        icon: data.timerMode ? Icons.timer : Icons.touch_app,
        label: '工作模式',
        value: data.timerMode ? '定时' : '手动',
        active: data.timerMode,
      ),
      _MetricItem(
        icon: Icons.schedule,
        label: '当前时间',
        value: data.time,
        active: data.time != '--:--:--',
      ),
      _MetricItem(
        icon: Icons.play_arrow,
        label: '开启时间',
        value: data.onTime,
        active: true,
      ),
      _MetricItem(
        icon: Icons.stop,
        label: '关闭时间',
        value: data.offTime,
        active: true,
      ),
      _MetricItem(
        icon: data.wifiConnected ? Icons.wifi : Icons.wifi_off,
        label: '设备 WiFi',
        value: data.wifiConnected ? 'connected' : data.wifi,
        active: data.wifiConnected,
      ),
    ];

    return LayoutBuilder(
      builder: (context, constraints) {
        final columns = constraints.maxWidth >= 900
            ? 3
            : constraints.maxWidth >= 560
            ? 2
            : 1;
        return GridView.builder(
          shrinkWrap: true,
          physics: const NeverScrollableScrollPhysics(),
          gridDelegate: SliverGridDelegateWithFixedCrossAxisCount(
            crossAxisCount: columns,
            crossAxisSpacing: 12,
            mainAxisSpacing: 12,
            childAspectRatio: columns == 1 ? 3.2 : 2.15,
          ),
          itemCount: items.length,
          itemBuilder: (context, index) => _MetricCard(item: items[index]),
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
        : Theme.of(context).colorScheme.outline;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(14),
        child: Row(
          children: [
            Icon(item.icon, color: color),
            const SizedBox(width: 12),
            Expanded(
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(item.label, style: Theme.of(context).textTheme.bodySmall),
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
          ],
        ),
      ),
    );
  }
}
