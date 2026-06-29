import 'package:flutter/material.dart';
import '../../services/znlyj_service.dart';

class ZnlyjDashboard extends StatefulWidget {
  final ZnlyjService service;
  const ZnlyjDashboard({super.key, required this.service});

  @override
  State<ZnlyjDashboard> createState() => _ZnlyjDashboardState();
}

class _ZnlyjDashboardState extends State<ZnlyjDashboard> {
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
        final d = snapshot.data ?? widget.service.latestData;
        final hasLight = widget.service.hasLight;
        final hasWeight = widget.service.hasWeight;
        return ListView(
          padding: const EdgeInsets.all(16),
          children: [
            _StatusChip(
              label: 'Rack',
              active: d.rackOpen == 1,
              color: d.rackOpen == 1 ? Colors.green : Colors.grey,
            ),
            const SizedBox(height: 8),
            _Card(
              icon: Icons.thermostat,
              label: 'Temperature',
              value: '${d.temperature.toStringAsFixed(1)} °C',
              color: Colors.orange,
            ),
            _Card(
              icon: Icons.water_drop,
              label: 'Humidity',
              value: '${d.humidity.toStringAsFixed(1)} %',
              color: Colors.blue,
            ),
            if (hasLight)
              _Card(
                icon: Icons.light_mode,
                label: 'Light',
                value: '${d.light.toStringAsFixed(1)} %',
                color: Colors.amber,
              ),
            _Card(
              icon: Icons.person,
              label: 'Human',
              value: d.humanPresent == 1 ? 'YES' : 'NO',
              color: d.humanPresent == 1 ? Colors.green : Colors.grey,
            ),
            _Card(
              icon: Icons.checkroom,
              label: 'Clothes',
              value: d.clothesPresent == 1 ? 'YES' : 'NO',
              color: d.clothesPresent == 1 ? Colors.blue : Colors.grey,
            ),
            if (hasWeight)
              _Card(
                icon: Icons.monitor_weight,
                label: 'Weight',
                value: '${d.weightKg.toStringAsFixed(1)} kg',
                color: d.weightOverload == 1 ? Colors.red : Colors.green,
              ),
            const SizedBox(height: 12),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: () => widget.service.setRack(d.rackOpen != 1),
                    icon: Icon(
                      d.rackOpen == 1
                          ? Icons.arrow_upward
                          : Icons.arrow_downward,
                    ),
                    label: Text(d.rackOpen == 1 ? 'Close Rack' : 'Open Rack'),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 8),
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
  const _Card({
    required this.icon,
    required this.label,
    required this.value,
    required this.color,
  });
  @override
  Widget build(BuildContext context) {
    return Card(
      child: ListTile(
        leading: Icon(icon, color: color, size: 28),
        title: Text(label),
        trailing: Text(
          value,
          style: TextStyle(
            fontSize: 16,
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
      label: Text(
        '$label: ${active ? "ON" : "OFF"}',
        style: const TextStyle(fontWeight: FontWeight.bold),
      ),
    );
  }
}
