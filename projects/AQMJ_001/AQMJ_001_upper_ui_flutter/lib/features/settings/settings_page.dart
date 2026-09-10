import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import '../../services/aqmj_service.dart';

class SettingsPage extends StatefulWidget {
  const SettingsPage({
    super.key,
    required this.service,
    required this.capabilities,
  });

  final AqmjService service;
  final VersionCapabilities capabilities;

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  bool _armed = false;
  bool _home = true;
  bool _lamp = false;

  @override
  Widget build(BuildContext context) {
    return AdaptivePage(
      children: [
        CommonControlPage(
          title: '门禁控制',
          embedded: true,
          primaryActions: [
            DeviceCommandAction(
              icon: Icons.refresh,
              label: '刷新状态',
              onPressed: widget.service.requestStatus,
            ),
            DeviceCommandAction(
              icon: Icons.notifications_active,
              label: '模拟门铃',
              onPressed: widget.service.doorbell,
            ),
            DeviceCommandAction(
              icon: Icons.check_circle,
              label: '清除报警',
              onPressed: widget.service.clearAlarm,
            ),
            if (widget.capabilities.has('message'))
              DeviceCommandAction(
                icon: Icons.play_arrow,
                label: '播放留言',
                onPressed: widget.service.playMessage,
              ),
          ],
          secondaryActions: [
            if (widget.capabilities.has('message'))
              DeviceCommandAction(
                icon: Icons.fiber_manual_record,
                label: '开始录音',
                onPressed: widget.service.recordStart,
              ),
            if (widget.capabilities.has('message'))
              DeviceCommandAction(
                icon: Icons.stop,
                label: '停止录音',
                onPressed: widget.service.recordStop,
              ),
          ],
        ),
        AdaptiveSectionCard(
          title: '状态下发',
          child: Column(
            children: [
              SwitchListTile(
                value: _armed,
                title: const Text('设防模式'),
                subtitle: Text(_armed ? '门前逗留会触发提醒' : '撤防状态不播报逗留提醒'),
                contentPadding: EdgeInsets.zero,
                onChanged: (value) {
                  setState(() => _armed = value);
                  widget.service.setArmed(value);
                },
              ),
              SwitchListTile(
                value: _home,
                title: const Text('主人在家'),
                subtitle: Text(_home ? '门铃触发本地提醒' : '门铃触发外出语音播报'),
                contentPadding: EdgeInsets.zero,
                onChanged: (value) {
                  setState(() => _home = value);
                  widget.service.setHome(value);
                },
              ),
              if (widget.capabilities.has('light'))
                SwitchListTile(
                  value: _lamp,
                  title: const Text('感应灯'),
                  subtitle: Text(_lamp ? '手动开灯' : '手动关灯'),
                  contentPadding: EdgeInsets.zero,
                  onChanged: (value) {
                    setState(() => _lamp = value);
                    widget.service.setLamp(value);
                  },
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
                : widget.capabilities.has('ir')
                    ? '红外遥控'
                    : '本地按键';
    final extras = <String>[];
    if (widget.capabilities.has('light')) extras.add('光敏感应灯');
    if (widget.capabilities.has('message')) extras.add('语音留言');
    if (widget.capabilities.has('camera')) extras.add('视频监控');
    return 'V${widget.capabilities.version} 支持门铃、人体检测、设防撤防、门铃时间记录和$remote控制。'
        '${extras.isEmpty ? '' : ' 扩展：${extras.join('、')}。'}';
  }
}
