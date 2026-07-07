import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

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
    final hasLight = widget.service.hasLight;
    final hasWeight = widget.service.hasWeight;
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
              _Item(label: '温度 (°C)', ctrl: temp),
              _Item(label: '湿度 (%)', ctrl: humidity),
              if (hasLight) _Item(label: '光照 (%)', ctrl: light),
              if (hasWeight) _Item(label: '重量 (kg)', ctrl: weight),
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

class _Item extends StatelessWidget {
  final String label;
  final TextEditingController ctrl;
  const _Item({required this.label, required this.ctrl});
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
