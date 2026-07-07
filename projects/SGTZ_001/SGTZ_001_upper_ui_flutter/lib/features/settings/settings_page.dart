import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import '../../services/sgtz_service.dart';

class SettingsPage extends StatefulWidget {
  const SettingsPage({
    super.key,
    required this.service,
    required this.capabilities,
  });

  final SgtzService service;
  final VersionCapabilities capabilities;

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  final _thresholdController = TextEditingController(text: '3000');

  @override
  void dispose() {
    _thresholdController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final capabilities = widget.capabilities;
    final commands = <Widget>[
      _CommandButton(
        icon: Icons.refresh,
        label: '刷新状态',
        onPressed: widget.service.requestStatus,
      ),
      if (capabilities.has('bmi') || capabilities.has('voice'))
        _CommandButton(
          icon: Icons.lock,
          label: '锁定/计算',
          onPressed: widget.service.lock,
        ),
      if (capabilities.has('bmi') || capabilities.has('voice'))
        _CommandButton(
          icon: Icons.lock_open,
          label: '重新测量',
          onPressed: widget.service.unlock,
        ),
      if (capabilities.has('weight') || capabilities.has('pressure'))
        _CommandButton(
          icon: Icons.restart_alt,
          label: '去皮',
          onPressed: widget.service.tare,
        ),
      if (capabilities.has('fan'))
        _CommandButton(
          icon: Icons.air,
          label: '风扇开启',
          onPressed: widget.service.fanOn,
        ),
      if (capabilities.has('fan'))
        _CommandButton(
          icon: Icons.air_outlined,
          label: '风扇关闭',
          onPressed: widget.service.fanOff,
        ),
      if (capabilities.has('voice'))
        _CommandButton(
          icon: Icons.volume_off,
          label: '语音暂停/恢复',
          onPressed: widget.service.voiceToggle,
        ),
    ];

    return AdaptivePage(
      children: [
        AdaptiveSectionCard(
          title: '设备控制',
          child: AdaptiveButtonGrid(children: commands),
        ),
        if (capabilities.has('threshold')) ...[
          AdaptiveSectionCard(
            title: '报警阈值',
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                TextField(
                  controller: _thresholdController,
                  keyboardType: TextInputType.number,
                  decoration: const InputDecoration(
                    labelText: '阈值 g',
                    helperText: 'SGTZ-002 范围 0-5000g',
                    border: OutlineInputBorder(),
                  ),
                ),
                const SizedBox(height: 12),
                FilledButton.icon(
                  onPressed: _setThreshold,
                  icon: const Icon(Icons.save),
                  label: const Text('下发阈值'),
                ),
              ],
            ),
          ),
        ],
        AdaptiveSectionCard(
          child: Text(
            '命令按 MQTT/BLE 透传 JSON 下发。当前固件已支持 lock、unlock、tare；风扇、阈值和语音命令用于 APP 版本协议扩展。',
            style: Theme.of(context).textTheme.bodySmall,
          ),
        ),
      ],
    );
  }

  void _setThreshold() {
    final value = int.tryParse(_thresholdController.text.trim());
    if (value == null || value < 0 || value > 5000) {
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(const SnackBar(content: Text('请输入 0-5000g 阈值')));
      return;
    }
    widget.service.setThreshold(value);
  }
}

class _CommandButton extends StatelessWidget {
  const _CommandButton({
    required this.icon,
    required this.label,
    required this.onPressed,
  });

  final IconData icon;
  final String label;
  final VoidCallback onPressed;

  @override
  Widget build(BuildContext context) {
    return FilledButton.icon(
      onPressed: onPressed,
      icon: Icon(icon),
      label: Text(
        label,
        textAlign: TextAlign.center,
        maxLines: 2,
        overflow: TextOverflow.ellipsis,
      ),
    );
  }
}
