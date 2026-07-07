import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import '../../services/wxcs_service.dart';

class DashboardPage extends StatefulWidget {
  final WxcsService service;
  const DashboardPage({super.key, required this.service});

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
    return StreamBuilder(
      stream: widget.service.onData,
      builder: (context, snapshot) {
        final data = snapshot.data ?? widget.service.latestData;
        return AdaptivePage(
          children: [
            _ModeChip(mode: data.mode),
            const SizedBox(height: 12),
            _SensorCard(
              icon: Icons.thermostat,
              label: '温度',
              value: '${data.temperature.toStringAsFixed(1)} °C',
              color: _tempColor(data.temperature, data.tempThreshold),
            ),
            _SensorCard(
              icon: Icons.air,
              label: '空气质量',
              value: '${data.airQualityPpm} ppm',
              color: _aqColor(data.airQualityPpm, data.aqThreshold),
            ),
            _SensorCard(
              icon: Icons.cloud,
              label: 'CO',
              value: '${data.coPpm} ppm',
              color: _coColor(data.coPpm, data.coThreshold),
            ),
            const SizedBox(height: 12),
            Row(
              children: [
                Expanded(
                  child: _StatusChip(
                    label: '风扇',
                    active: data.fan == 1,
                    color: data.fan == 1 ? Colors.green : Colors.grey,
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: _StatusChip(
                    label: '报警',
                    active: data.alarm == 1,
                    color: data.alarm == 1 ? Colors.red : Colors.grey,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: () =>
                        widget.service.setControl(fan: data.fan == 1 ? 0 : 1),
                    icon: Icon(data.fan == 1 ? Icons.stop : Icons.play_arrow),
                    label: Text(data.fan == 1 ? '关闭风扇' : '启动风扇'),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: () => widget.service.setControl(
                      alarm: data.alarm == 1 ? 0 : 1,
                    ),
                    icon: Icon(
                      data.alarm == 1
                          ? Icons.notifications_off
                          : Icons.notifications_active,
                    ),
                    label: Text(data.alarm == 1 ? '关闭报警' : '启动报警'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: data.alarm == 1
                          ? Colors.red
                          : Colors.orange,
                      foregroundColor: Colors.white,
                    ),
                  ),
                ),
              ],
            ),
          ],
        );
      },
    );
  }

  Color _tempColor(double temp, int threshold) =>
      temp > threshold ? Colors.red : Colors.blue;

  Color _aqColor(int ppm, int threshold) =>
      ppm > threshold ? Colors.orange : Colors.green;

  Color _coColor(int ppm, int threshold) =>
      ppm > threshold ? Colors.deepOrange : Colors.green;
}

class _ModeChip extends StatelessWidget {
  final String mode;
  const _ModeChip({required this.mode});

  @override
  Widget build(BuildContext context) {
    return Chip(
      avatar: const Icon(Icons.settings, size: 18),
      label: Text(
        switch (mode.toLowerCase()) {
          'auto' => '自动模式',
          'manual' => '手动模式',
          'threshold' => '阈值设置',
          _ => mode,
        },
        style: const TextStyle(fontWeight: FontWeight.bold),
      ),
    );
  }
}

class _SensorCard extends StatelessWidget {
  final IconData icon;
  final String label;
  final String value;
  final Color color;

  const _SensorCard({
    required this.icon,
    required this.label,
    required this.value,
    required this.color,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: ListTile(
        leading: Icon(icon, color: color, size: 32),
        title: Text(label),
        trailing: Text(
          value,
          style: TextStyle(
            fontSize: 18,
            color: color,
            fontWeight: FontWeight.bold,
          ),
        ),
      ),
    );
  }
}

class _StatusChip extends StatelessWidget {
  final String label;
  final bool active;
  final Color color;

  const _StatusChip({
    required this.label,
    required this.active,
    required this.color,
  });

  @override
  Widget build(BuildContext context) {
    return Chip(
      backgroundColor: color.withAlpha(40),
      side: BorderSide(color: color),
      label: Text('$label：${active ? "开启" : "关闭"}'),
    );
  }
}
