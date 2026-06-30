import 'package:flutter/material.dart';
import '../../services/zhjg_service.dart';

class SettingsPage extends StatefulWidget {
  final ZhjgService service;

  const SettingsPage({super.key, required this.service});

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  late TextEditingController methaneCtrl;
  late TextEditingController waterCtrl;
  late TextEditingController tiltCtrl;

  @override
  void initState() {
    super.initState();
    final data = widget.service.latestData;
    methaneCtrl = TextEditingController(text: data.methaneThresholdPpm.toString());
    waterCtrl = TextEditingController(
      text: data.waterThresholdPercent.toStringAsFixed(0),
    );
    tiltCtrl = TextEditingController(
      text: data.tiltThresholdDegree.toStringAsFixed(0),
    );
  }

  @override
  void dispose() {
    methaneCtrl.dispose();
    waterCtrl.dispose();
    tiltCtrl.dispose();
    super.dispose();
  }

  void _save() {
    widget.service.setThreshold(
      methanePpm: int.tryParse(methaneCtrl.text),
      waterPercent: double.tryParse(waterCtrl.text),
      tiltDegree: double.tryParse(tiltCtrl.text),
    );
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('阈值已发送'), duration: Duration(seconds: 1)),
    );
  }

  void _selectMode(String mode) {
    widget.service.setMode(mode);
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('模式已切换: $mode'), duration: const Duration(seconds: 1)),
    );
  }

  @override
  Widget build(BuildContext context) {
    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        Text('运行模式', style: Theme.of(context).textTheme.titleMedium),
        const SizedBox(height: 8),
        Wrap(
          spacing: 8,
          runSpacing: 8,
          children: [
            ActionChip(
              avatar: const Icon(Icons.auto_mode),
              label: const Text('自动模式'),
              onPressed: () => _selectMode('auto'),
            ),
            ActionChip(
              avatar: const Icon(Icons.tune),
              label: const Text('阈值设置模式'),
              onPressed: () => _selectMode('threshold'),
            ),
          ],
        ),
        const SizedBox(height: 24),
        Text('告警阈值', style: Theme.of(context).textTheme.titleMedium),
        const SizedBox(height: 8),
        _ThresholdField(
          label: '沼气浓度',
          unit: 'ppm',
          ctrl: methaneCtrl,
          min: 100,
          max: 5000,
        ),
        _ThresholdField(
          label: '水位上限',
          unit: '%',
          ctrl: waterCtrl,
          min: 5,
          max: 100,
        ),
        _ThresholdField(
          label: '倾角上限',
          unit: '°',
          ctrl: tiltCtrl,
          min: 1,
          max: 60,
        ),
        const SizedBox(height: 16),
        FilledButton.icon(
          onPressed: _save,
          icon: const Icon(Icons.save),
          label: const Text('保存阈值'),
        ),
      ],
    );
  }
}

class _ThresholdField extends StatelessWidget {
  final String label;
  final String unit;
  final TextEditingController ctrl;
  final int min;
  final int max;

  const _ThresholdField({
    required this.label,
    required this.unit,
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
            SizedBox(width: 120, child: Text(label)),
            Expanded(
              child: TextField(
                controller: ctrl,
                keyboardType: TextInputType.number,
                decoration: InputDecoration(
                  isDense: true,
                  suffixText: unit,
                  helperText: '$min-$max',
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
