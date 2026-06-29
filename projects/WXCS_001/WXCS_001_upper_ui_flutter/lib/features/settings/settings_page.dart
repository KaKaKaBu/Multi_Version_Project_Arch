import 'package:flutter/material.dart';
import '../../services/wxcs_service.dart';

class SettingsPage extends StatefulWidget {
  final WxcsService service;
  const SettingsPage({super.key, required this.service});

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  late TextEditingController tempCtrl;
  late TextEditingController aqCtrl;
  late TextEditingController coCtrl;

  @override
  void initState() {
    super.initState();
    final d = widget.service.latestData;
    tempCtrl = TextEditingController(text: d.tempThreshold.toString());
    aqCtrl = TextEditingController(text: d.aqThreshold.toString());
    coCtrl = TextEditingController(text: d.coThreshold.toString());
  }

  @override
  void dispose() {
    tempCtrl.dispose();
    aqCtrl.dispose();
    coCtrl.dispose();
    super.dispose();
  }

  void _save() {
    widget.service.setThreshold(
      temp: int.tryParse(tempCtrl.text),
      aq: int.tryParse(aqCtrl.text),
      co: int.tryParse(coCtrl.text),
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
        content: Text('Mode set to $mode'),
        duration: const Duration(seconds: 1),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
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
        const SizedBox(height: 24),
        Text('Thresholds', style: Theme.of(context).textTheme.titleMedium),
        const SizedBox(height: 8),
        _ThresholdField(
          label: 'Temperature (°C)',
          ctrl: tempCtrl,
          min: 10,
          max: 60,
        ),
        _ThresholdField(
          label: 'Air Quality (ppm)',
          ctrl: aqCtrl,
          min: 100,
          max: 2000,
        ),
        _ThresholdField(label: 'CO (ppm)', ctrl: coCtrl, min: 10, max: 500),
        const SizedBox(height: 16),
        ElevatedButton(onPressed: _save, child: const Text('Save Thresholds')),
      ],
    );
  }
}

class _ThresholdField extends StatelessWidget {
  final String label;
  final TextEditingController ctrl;
  final int min;
  final int max;

  const _ThresholdField({
    required this.label,
    required this.ctrl,
    required this.min,
    required this.max,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
        child: Row(
          children: [
            SizedBox(width: 140, child: Text(label)),
            Expanded(
              child: TextField(
                controller: ctrl,
                keyboardType: TextInputType.number,
                decoration: InputDecoration(
                  isDense: true,
                  suffix: Text('$min-$max'),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
