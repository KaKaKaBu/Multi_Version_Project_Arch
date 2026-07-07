import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

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
        content: Text('阈值已下发'),
        duration: Duration(seconds: 1),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return AdaptivePage(
      children: [
        AdaptiveSectionCard(
          title: '阈值设置',
          child: Column(
            children: [
              _Field(label: '心率上限 (bpm)', ctrl: hrU),
              _Field(label: '心率下限 (bpm)', ctrl: hrL),
              _Field(label: '血氧上限 (%)', ctrl: sU),
              _Field(label: '血氧下限 (%)', ctrl: sL),
              _Field(label: '体温上限 (°C)', ctrl: tU),
              _Field(label: '体温下限 (°C)', ctrl: tL),
              const SizedBox(height: 12),
              FilledButton(onPressed: _save, child: const Text('保存阈值')),
            ],
          ),
        ),
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
    return AdaptiveFieldRow(
      label: label,
      child: TextField(
        controller: ctrl,
        keyboardType: TextInputType.number,
        decoration: const InputDecoration(isDense: true),
      ),
    );
  }
}
