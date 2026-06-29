import 'package:flutter/material.dart';
import '../../services/smzl_service.dart';

class SmzlSettings extends StatefulWidget {
  final SmzlService service;
  const SmzlSettings({super.key, required this.service});

  @override
  State<SmzlSettings> createState() => _SmzlSettingsState();
}

class _SmzlSettingsState extends State<SmzlSettings> {
  late TextEditingController hrU, hrL, sU, sL, tU, tL;

  @override
  void initState() {
    super.initState();
    final d = widget.service.latestData;
    hrU = TextEditingController(text: d.hrUpper.toString());
    hrL = TextEditingController(text: d.hrLower.toString());
    sU = TextEditingController(text: d.spo2Upper.toString());
    sL = TextEditingController(text: d.spo2Lower.toString());
    tU = TextEditingController(text: d.tempUpper.toString());
    tL = TextEditingController(text: d.tempLower.toString());
  }

  @override
  void dispose() {
    hrU.dispose();
    hrL.dispose();
    sU.dispose();
    sL.dispose();
    tU.dispose();
    tL.dispose();
    super.dispose();
  }

  void _save() {
    widget.service.setThreshold(
      hrU: int.tryParse(hrU.text),
      hrL: int.tryParse(hrL.text),
      sU: int.tryParse(sU.text),
      sL: int.tryParse(sL.text),
      tU: double.tryParse(tU.text),
      tL: double.tryParse(tL.text),
    );
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
        content: Text('Thresholds sent'),
        duration: Duration(seconds: 1),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        Text(
          'Threshold Settings',
          style: Theme.of(context).textTheme.titleMedium,
        ),
        const SizedBox(height: 8),
        _Field(label: 'HR Upper (bpm)', ctrl: hrU),
        _Field(label: 'HR Lower (bpm)', ctrl: hrL),
        _Field(label: 'SpO2 Upper (%)', ctrl: sU),
        _Field(label: 'SpO2 Lower (%)', ctrl: sL),
        _Field(label: 'Temp Upper (°C)', ctrl: tU),
        _Field(label: 'Temp Lower (°C)', ctrl: tL),
        const SizedBox(height: 16),
        ElevatedButton(onPressed: _save, child: const Text('Save Thresholds')),
      ],
    );
  }
}

class _Field extends StatelessWidget {
  final String label;
  final TextEditingController ctrl;
  const _Field({required this.label, required this.ctrl});
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
