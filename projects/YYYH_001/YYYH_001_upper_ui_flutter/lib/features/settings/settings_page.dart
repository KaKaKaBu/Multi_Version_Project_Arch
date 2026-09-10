import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import '../../services/telemetry_model.dart';
import '../../services/yyyh_service.dart';

class SettingsPage extends StatefulWidget {
  const SettingsPage({
    super.key,
    required this.service,
    required this.capabilities,
  });

  final YyyhService service;
  final VersionCapabilities capabilities;

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  final _hourController = TextEditingController(text: '8');
  final _minuteController = TextEditingController(text: '0');
  final _doseController = TextEditingController(text: '1');
  final _countControllers = <int, TextEditingController>{
    1: TextEditingController(text: '10'),
    2: TextEditingController(text: '10'),
    3: TextEditingController(text: '10'),
  };

  int _timerIndex = 1;
  int _category = 0;
  bool _enabled = true;

  @override
  void dispose() {
    _hourController.dispose();
    _minuteController.dispose();
    _doseController.dispose();
    for (final controller in _countControllers.values) {
      controller.dispose();
    }
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AdaptivePage(
      children: [
        CommonControlPage(
          title: '药盒控制',
          embedded: true,
          primaryActions: [
            DeviceCommandAction(
              icon: Icons.refresh,
              label: '刷新状态',
              onPressed: widget.service.requestStatus,
            ),
            DeviceCommandAction(
              icon: Icons.lock_open,
              label: '打开药盒',
              onPressed: widget.service.openBox,
            ),
            DeviceCommandAction(
              icon: Icons.lock,
              label: '关闭药盒',
              onPressed: widget.service.closeBox,
            ),
            DeviceCommandAction(
              icon: Icons.check_circle,
              label: '确认取药',
              onPressed: widget.service.ackTake,
            ),
            DeviceCommandAction(
              icon: Icons.access_time,
              label: '同步时间',
              onPressed: () => widget.service.setTime(DateTime.now()),
            ),
          ],
        ),
        AdaptiveSectionCard(
          title: '定时设置',
          child: Column(
            children: [
              AdaptiveFieldRow(
                label: '定时组',
                child: SegmentedButton<int>(
                  segments: const [
                    ButtonSegment(value: 1, label: Text('1')),
                    ButtonSegment(value: 2, label: Text('2')),
                    ButtonSegment(value: 3, label: Text('3')),
                  ],
                  selected: {_timerIndex},
                  onSelectionChanged: (values) {
                    setState(() => _timerIndex = values.first);
                  },
                ),
              ),
              AdaptiveFieldRow(
                label: '启用状态',
                child: SwitchListTile(
                  value: _enabled,
                  title: Text(_enabled ? '启用' : '关闭'),
                  contentPadding: EdgeInsets.zero,
                  onChanged: (value) => setState(() => _enabled = value),
                ),
              ),
              AdaptiveFieldRow(
                label: '提醒时间',
                child: Row(
                  children: [
                    Expanded(
                      child: _NumberField(
                        controller: _hourController,
                        suffixText: '时',
                      ),
                    ),
                    const SizedBox(width: 8),
                    Expanded(
                      child: _NumberField(
                        controller: _minuteController,
                        suffixText: '分',
                      ),
                    ),
                  ],
                ),
              ),
              AdaptiveFieldRow(
                label: '剂量/分类',
                child: Row(
                  children: [
                    Expanded(
                      child: _NumberField(
                        controller: _doseController,
                        suffixText: '份',
                      ),
                    ),
                    const SizedBox(width: 8),
                    Expanded(
                      child: DropdownButtonFormField<int>(
                        initialValue: _category,
                        decoration: const InputDecoration(
                          border: OutlineInputBorder(),
                          isDense: true,
                        ),
                        items: const [
                          DropdownMenuItem(value: 0, child: Text('药品 1')),
                          DropdownMenuItem(value: 1, child: Text('药品 2')),
                          DropdownMenuItem(value: 2, child: Text('药品 3')),
                        ],
                        onChanged: (value) {
                          if (value != null) setState(() => _category = value);
                        },
                      ),
                    ),
                  ],
                ),
              ),
              Align(
                alignment: Alignment.centerRight,
                child: FilledButton.icon(
                  onPressed: _applyTimer,
                  icon: const Icon(Icons.send),
                  label: const Text('下发定时'),
                ),
              ),
            ],
          ),
        ),
        AdaptiveSectionCard(
          title: '药量设置',
          child: Column(
            children: [
              for (var index = 1; index <= 3; index++)
                AdaptiveFieldRow(
                  label: '药品 $index 数量',
                  child: Row(
                    children: [
                      Expanded(
                        child: _NumberField(
                          controller: _countControllers[index]!,
                          suffixText: '份',
                        ),
                      ),
                      const SizedBox(width: 8),
                      FilledButton(
                        onPressed: () => _applyCount(index),
                        child: const Text('下发'),
                      ),
                    ],
                  ),
                ),
            ],
          ),
        ),
        AdaptiveSectionCard(
          child: Text(
            _versionHelpText(),
            style: Theme.of(context).textTheme.bodySmall,
          ),
        ),
      ],
    );
  }

  String _versionHelpText() {
    final remote = widget.capabilities.has('cloud')
        ? '华为云App'
        : widget.capabilities.has('wifi')
            ? 'WiFi App'
            : widget.capabilities.has('ble')
                ? '蓝牙App'
                : '本地按键';
    return 'V${widget.capabilities.version} 支持 3 组吃药定时、药品分类、药量设置、取药检测和$remote控制。命令通过透明 JSON 协议下发。';
  }

  void _applyTimer() {
    final hour = int.tryParse(_hourController.text.trim());
    final minute = int.tryParse(_minuteController.text.trim());
    final dose = int.tryParse(_doseController.text.trim());
    if (hour == null ||
        hour < 0 ||
        hour > 23 ||
        minute == null ||
        minute < 0 ||
        minute > 59 ||
        dose == null ||
        dose <= 0) {
      _showError('请输入有效的时间和剂量');
      return;
    }
    widget.service.setTimer(
      MedicineTimer(
        index: _timerIndex,
        enabled: _enabled,
        hour: hour,
        minute: minute,
        dose: dose,
        category: _category,
      ),
    );
  }

  void _applyCount(int index) {
    final value = int.tryParse(_countControllers[index]!.text.trim());
    if (value == null || value < 0) {
      _showError('请输入有效药量');
      return;
    }
    widget.service.setCount(index, value);
  }

  void _showError(String message) {
    ScaffoldMessenger.of(
      context,
    ).showSnackBar(SnackBar(content: Text(message)));
  }
}

class _NumberField extends StatelessWidget {
  const _NumberField({required this.controller, required this.suffixText});

  final TextEditingController controller;
  final String suffixText;

  @override
  Widget build(BuildContext context) {
    return TextField(
      controller: controller,
      keyboardType: TextInputType.number,
      decoration: InputDecoration(
        suffixText: suffixText,
        border: const OutlineInputBorder(),
        isDense: true,
      ),
    );
  }
}
