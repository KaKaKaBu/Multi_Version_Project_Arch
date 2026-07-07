import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import '../../config/version_capabilities.dart';
import '../../services/kqzl2_service.dart';
import '../../services/telemetry_model.dart';

class SettingsPage extends StatefulWidget {
  const SettingsPage({
    super.key,
    required this.service,
    required this.capabilities,
  });

  final Kqzl2Service service;
  final VersionCapabilities capabilities;

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  late final Map<String, TextEditingController> _thresholdControllers;

  @override
  void initState() {
    super.initState();
    _thresholdControllers = {
      for (final sensor in airSensorCatalog)
        sensor.thresholdKey: TextEditingController(
          text: sensor.defaultThreshold.toString(),
        ),
    };
  }

  @override
  void dispose() {
    for (final controller in _thresholdControllers.values) {
      controller.dispose();
    }
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final sensors = airSensorCatalog
        .where((sensor) => sensorSupported(widget.capabilities, sensor.feature))
        .toList();

    return AdaptivePage(
      children: [
        CommonControlPage(
          title: '工作模式',
          primaryActions: [
            DeviceCommandAction(
              icon: Icons.autorenew,
              label: '自动模式',
              onPressed: () => widget.service.setMode('auto'),
            ),
            DeviceCommandAction(
              icon: Icons.touch_app,
              label: '手动模式',
              onPressed: () => widget.service.setMode('manual'),
            ),
            DeviceCommandAction(
              icon: Icons.tune,
              label: '阈值模式',
              onPressed: () => widget.service.setMode('threshold'),
            ),
            DeviceCommandAction(
              icon: Icons.refresh,
              label: '刷新状态',
              onPressed: widget.service.requestStatus,
            ),
          ],
        ),
        AdaptiveSectionCard(
          title: '远程控制',
          child: AdaptiveButtonGrid(
            children: [
              for (final actuator in airActuatorCatalog) ...[
                FilledButton.icon(
                  onPressed: () =>
                      widget.service.setDevice(actuator.commandDevice, true),
                  icon: Icon(_actuatorIcon(actuator.key)),
                  label: Text('${actuator.label}开'),
                ),
                OutlinedButton.icon(
                  onPressed: () =>
                      widget.service.setDevice(actuator.commandDevice, false),
                  icon: Icon(_actuatorIcon(actuator.key)),
                  label: Text('${actuator.label}关'),
                ),
              ],
            ],
          ),
        ),
        AdaptiveSectionCard(
          title: '阈值设置',
          child: Column(
            children: [
              for (final sensor in sensors)
                AdaptiveFieldRow(
                  label: '${sensor.label} 阈值',
                  child: Row(
                    children: [
                      Expanded(
                        child: TextField(
                          controller: _thresholdControllers[sensor.thresholdKey],
                          keyboardType: TextInputType.number,
                          decoration: InputDecoration(
                            suffixText: sensor.unit,
                            border: const OutlineInputBorder(),
                            isDense: true,
                          ),
                        ),
                      ),
                      const SizedBox(width: 8),
                      FilledButton(
                        onPressed: () => _applyThreshold(sensor),
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
            'V14 支持温湿度、烟雾、PM2.5、CO、排风、蜂鸣器、灯光和华为云App远程控制。命令通过 MQTT/BLE 透明 JSON 协议下发。',
            style: Theme.of(context).textTheme.bodySmall,
          ),
        ),
      ],
    );
  }

  void _applyThreshold(AirSensorSpec sensor) {
    final raw = _thresholdControllers[sensor.thresholdKey]?.text.trim() ?? '';
    final value = int.tryParse(raw);
    if (value == null || value < 0) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('请输入有效阈值')),
      );
      return;
    }
    widget.service.setThreshold(sensor.thresholdKey, value);
  }

  static IconData _actuatorIcon(String key) {
    return switch (key) {
      'buzzer' => Icons.notifications_active,
      'light' => Icons.lightbulb,
      _ => Icons.air,
    };
  }
}
