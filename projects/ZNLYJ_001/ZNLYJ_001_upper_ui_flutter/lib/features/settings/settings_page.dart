import 'package:flutter/material.dart';
import '../../services/znlyj_service.dart';

class ZnlyjSettings extends StatefulWidget {
  final ZnlyjService service;
  const ZnlyjSettings({super.key, required this.service});

  @override
  State<ZnlyjSettings> createState() => _ZnlyjSettingsState();
}

class _ZnlyjSettingsState extends State<ZnlyjSettings> {
  late TextEditingController temp, humidity, light, weight;

  @override
  void initState() {
    super.initState();
    final d = widget.service.latestData;
    temp = TextEditingController(text: d.tempThreshold.toStringAsFixed(1));
    humidity = TextEditingController(
      text: d.humidityThreshold.toStringAsFixed(1),
    );
    light = TextEditingController(text: d.lightThreshold.toStringAsFixed(1));
    weight = TextEditingController(text: d.weightThreshold.toStringAsFixed(1));
  }

  @override
  void dispose() {
    temp.dispose();
    humidity.dispose();
    light.dispose();
    weight.dispose();
    super.dispose();
  }

  void _save() {
    widget.service.setThreshold(
      temp: double.tryParse(temp.text),
      humidity: double.tryParse(humidity.text),
      light: double.tryParse(light.text),
      weight: double.tryParse(weight.text),
    );
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
        content: Text('Thresholds sent'),
        duration: Duration(seconds: 1),
      ),
    );
  }

  void _selectMode(String mode) {
    widget.service.setMode(mode);
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text('Mode: $mode'),
        duration: const Duration(seconds: 1),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final hasLight = widget.service.hasLight;
    final hasWeight = widget.service.hasWeight;
    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        Text('Mode', style: Theme.of(context).textTheme.titleMedium),
        const SizedBox(height: 8),
        Wrap(
          spacing: 8,
          children: [
            ActionChip(
              label: const Text('AUTO'),
              onPressed: () => _selectMode('auto'),
            ),
            ActionChip(
              label: const Text('MANUAL'),
              onPressed: () => _selectMode('manual'),
            ),
            ActionChip(
              label: const Text('THRESHOLD'),
              onPressed: () => _selectMode('threshold'),
            ),
          ],
        ),
        const SizedBox(height: 16),
        Text('Thresholds', style: Theme.of(context).textTheme.titleMedium),
        const SizedBox(height: 8),
        _Item(label: 'Temperature (°C)', ctrl: temp),
        _Item(label: 'Humidity (%)', ctrl: humidity),
        if (hasLight) _Item(label: 'Light (%)', ctrl: light),
        if (hasWeight) _Item(label: 'Weight (kg)', ctrl: weight),
        const SizedBox(height: 16),
        ElevatedButton(onPressed: _save, child: const Text('Save Thresholds')),
      ],
    );
  }
}

class _Item extends StatelessWidget {
  final String label;
  final TextEditingController ctrl;
  const _Item({required this.label, required this.ctrl});
  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
        child: Row(
          children: [
            SizedBox(width: 150, child: Text(label)),
            Expanded(
              child: TextField(
                controller: ctrl,
                keyboardType: TextInputType.number,
                decoration: const InputDecoration(isDense: true),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
