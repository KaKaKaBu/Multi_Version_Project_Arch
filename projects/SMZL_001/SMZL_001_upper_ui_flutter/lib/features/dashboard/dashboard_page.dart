import 'package:flutter/material.dart';
import '../../services/smzl_service.dart';

class SmzlDashboard extends StatefulWidget {
  final SmzlService service;
  const SmzlDashboard({super.key, required this.service});

  @override
  State<SmzlDashboard> createState() => _SmzlDashboardState();
}

class _SmzlDashboardState extends State<SmzlDashboard> {
  @override
  void initState() {
    super.initState();
    widget.service.requestStatus();
  }

  Color _hrColor(int hr, int upper, int lower) =>
      (hr > upper || hr < lower) ? Colors.red : Colors.green;

  Color _spo2Color(int s, int lower) => s < lower ? Colors.red : Colors.blue;

  Color _tempColor(double t, double upper, double lower) =>
      (t > upper || t < lower) ? Colors.red : Colors.orange;

  @override
  Widget build(BuildContext context) {
    return StreamBuilder(
      stream: widget.service.onData,
      builder: (context, snapshot) {
        final d = snapshot.data ?? widget.service.latestData;
        return ListView(
          padding: const EdgeInsets.all(16),
          children: [
            _Card(
              icon: Icons.flip,
              label: 'Turnover',
              value: '${d.turnoverCount}次',
              color: Colors.purple,
              trailing: TextButton(
                onPressed: () => widget.service.clearTurnover(),
                child: const Text('Clear'),
              ),
            ),
            _Card(
              icon: Icons.favorite,
              label: 'Heart Rate',
              value: '${d.heartRate} bpm',
              color: _hrColor(d.heartRate, d.hrUpper, d.hrLower),
            ),
            _Card(
              icon: Icons.bloodtype,
              label: 'SpO2',
              value: '${d.spo2} %',
              color: _spo2Color(d.spo2, d.spo2Lower),
            ),
            _Card(
              icon: Icons.thermostat,
              label: 'Temperature',
              value: '${d.temperature.toStringAsFixed(1)} °C',
              color: _tempColor(d.temperature, d.tempUpper, d.tempLower),
            ),
            const SizedBox(height: 12),
            _StatusChip(
              label: 'Alarm',
              active: d.alarm == 1,
              color: d.alarm == 1 ? Colors.red : Colors.grey,
            ),
          ],
        );
      },
    );
  }
}

class _Card extends StatelessWidget {
  final IconData icon;
  final String label;
  final String value;
  final Color color;
  final Widget? trailing;
  const _Card({
    required this.icon,
    required this.label,
    required this.value,
    required this.color,
    this.trailing,
  });
  @override
  Widget build(BuildContext context) {
    return Card(
      child: ListTile(
        leading: Icon(icon, color: color, size: 32),
        title: Text(label),
        trailing:
            trailing ??
            Text(
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
      label: Text('$label: ${active ? "ON" : "OFF"}'),
    );
  }
}
