import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

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
        content: Text('阈值已下发'),
        duration: Duration(seconds: 1),
      ),
    );
  }

  void _selectMode(String mode) {
    widget.service.setMode(mode);
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text('模式已切换为 ${_modeLabel(mode)}'),
        duration: const Duration(seconds: 1),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return AdaptivePage(
      children: [
        AdaptiveSectionCard(
          title: '工作模式',
          child: Wrap(
            spacing: 8,
            runSpacing: 8,
            children: [
              ActionChip(
                label: const Text('自动'),
                onPressed: () => _selectMode('auto'),
              ),
              ActionChip(
                label: const Text('手动'),
                onPressed: () => _selectMode('manual'),
              ),
              ActionChip(
                label: const Text('阈值设置'),
                onPressed: () => _selectMode('threshold'),
              ),
            ],
          ),
        ),
        AdaptiveSectionCard(
          title: '阈值设置',
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              _ThresholdField(
                label: '温度 (°C)',
                ctrl: tempCtrl,
                min: 10,
                max: 60,
              ),
              _ThresholdField(
                label: '空气质量 (ppm)',
                ctrl: aqCtrl,
                min: 100,
                max: 2000,
              ),
              _ThresholdField(label: 'CO (ppm)', ctrl: coCtrl, min: 10, max: 500),
              const SizedBox(height: 12),
              FilledButton(onPressed: _save, child: const Text('保存阈值')),
            ],
          ),
        ),
      ],
    );
  }

  String _modeLabel(String mode) => switch (mode) {
        'auto' => '自动',
        'manual' => '手动',
        'threshold' => '阈值设置',
        _ => mode,
      };
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
    return AdaptiveFieldRow(
      label: label,
      child: TextField(
        controller: ctrl,
        keyboardType: TextInputType.number,
        decoration: InputDecoration(
          isDense: true,
          suffix: Text('$min-$max'),
        ),
      ),
    );
  }
}
