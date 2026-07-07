import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';
import 'package:flutter/services.dart';

import '../../services/telemetry_model.dart';
import '../../services/zncz_service.dart';

class SettingsPage extends StatefulWidget {
  const SettingsPage({
    super.key,
    required this.service,
    required this.capabilities,
  });

  final ZnczService service;
  final VersionCapabilities capabilities;

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  final _onTimeController = TextEditingController(text: '08:00:00');
  final _offTimeController = TextEditingController(text: '22:00:00');
  DateTime _selectedDateTime = DateTime.now();

  @override
  void dispose() {
    _onTimeController.dispose();
    _offTimeController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return StreamBuilder<SocketTelemetry>(
      stream: widget.service.onData,
      builder: (context, snapshot) {
        final data = snapshot.data ?? widget.service.latestData;
        _syncControllers(data);
        return ListView(
          padding: const EdgeInsets.all(16),
          children: [
            _ControlCard(
              data: data,
              onRefresh: () => _send(widget.service.requestStatus),
              onRelayOn: () => _send(() => widget.service.setRelay(true)),
              onRelayOff: () => _send(() => widget.service.setRelay(false)),
            ),
            const SizedBox(height: 12),
            _ScheduleCard(
              onTimeController: _onTimeController,
              offTimeController: _offTimeController,
              onSubmitOn: _submitOnTime,
              onSubmitOff: _submitOffTime,
            ),
            const SizedBox(height: 12),
            _TimeSyncCard(
              selectedDateTime: _selectedDateTime,
              onPickDate: _pickDate,
              onPickTime: _pickTime,
              onUseNow: () {
                setState(() => _selectedDateTime = DateTime.now());
              },
              onSubmit: () => _send(
                () => widget.service.setDeviceTime(_selectedDateTime),
              ),
            ),
            if (upperUiShowCommunicationLog) ...[
              const SizedBox(height: 12),
              _LogCard(service: widget.service),
            ],
          ],
        );
      },
    );
  }

  void _syncControllers(SocketTelemetry data) {
    if (!_onTimeController.selection.isValid &&
        _onTimeController.text != data.onTime) {
      _onTimeController.text = data.onTime;
    }
    if (!_offTimeController.selection.isValid &&
        _offTimeController.text != data.offTime) {
      _offTimeController.text = data.offTime;
    }
  }

  Future<void> _submitOnTime() async {
    final time = _parseTime(_onTimeController.text);
    if (time == null) return;
    await _send(() => widget.service.setOnTime(time));
  }

  Future<void> _submitOffTime() async {
    final time = _parseTime(_offTimeController.text);
    if (time == null) return;
    await _send(() => widget.service.setOffTime(time));
  }

  TimeParts? _parseTime(String text) {
    final parsed = TimeParts.parse(text);
    if (parsed == null) {
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(const SnackBar(content: Text('时间格式应为 HH:MM:SS')));
    }
    return parsed;
  }

  Future<void> _pickDate() async {
    final picked = await showDatePicker(
      context: context,
      initialDate: _selectedDateTime,
      firstDate: DateTime(2024),
      lastDate: DateTime(2099),
    );
    if (picked == null) return;
    setState(() {
      _selectedDateTime = DateTime(
        picked.year,
        picked.month,
        picked.day,
        _selectedDateTime.hour,
        _selectedDateTime.minute,
        _selectedDateTime.second,
      );
    });
  }

  Future<void> _pickTime() async {
    final picked = await showTimePicker(
      context: context,
      initialTime: TimeOfDay.fromDateTime(_selectedDateTime),
    );
    if (picked == null) return;
    setState(() {
      _selectedDateTime = DateTime(
        _selectedDateTime.year,
        _selectedDateTime.month,
        _selectedDateTime.day,
        picked.hour,
        picked.minute,
        0,
      );
    });
  }

  Future<void> _send(Future<void> Function() action) async {
    if (!widget.service.isConnected) {
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(const SnackBar(content: Text('MQTT 未连接')));
      return;
    }
    await action();
    if (!mounted) return;
    ScaffoldMessenger.of(
      context,
    ).showSnackBar(const SnackBar(content: Text('命令已发送')));
  }
}

class _ControlCard extends StatelessWidget {
  const _ControlCard({
    required this.data,
    required this.onRefresh,
    required this.onRelayOn,
    required this.onRelayOff,
  });

  final SocketTelemetry data;
  final VoidCallback onRefresh;
  final VoidCallback onRelayOn;
  final VoidCallback onRelayOff;

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            ListTile(
              contentPadding: EdgeInsets.zero,
              leading: const Icon(Icons.power),
              title: const Text('手动控制'),
              subtitle: Text(data.relayOn ? '当前继电器 ON' : '当前继电器 OFF'),
            ),
            Wrap(
              spacing: 10,
              runSpacing: 10,
              children: [
                FilledButton.icon(
                  onPressed: onRelayOn,
                  icon: const Icon(Icons.power_settings_new),
                  label: const Text('继电器开启'),
                ),
                FilledButton.tonalIcon(
                  onPressed: onRelayOff,
                  icon: const Icon(Icons.power_off),
                  label: const Text('继电器关闭'),
                ),
                OutlinedButton.icon(
                  onPressed: onRefresh,
                  icon: const Icon(Icons.refresh),
                  label: const Text('刷新状态'),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}

class _ScheduleCard extends StatelessWidget {
  const _ScheduleCard({
    required this.onTimeController,
    required this.offTimeController,
    required this.onSubmitOn,
    required this.onSubmitOff,
  });

  final TextEditingController onTimeController;
  final TextEditingController offTimeController;
  final VoidCallback onSubmitOn;
  final VoidCallback onSubmitOff;

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            const ListTile(
              contentPadding: EdgeInsets.zero,
              leading: Icon(Icons.timer),
              title: Text('定时配置'),
              subtitle: Text('下发 set_on_time / set_off_time 到设备 RAM 配置'),
            ),
            LayoutBuilder(
              builder: (context, constraints) {
                final wide = constraints.maxWidth >= 680;
                final fields = [
                  _TimeField(
                    controller: onTimeController,
                    label: '开启时间',
                    onSubmit: onSubmitOn,
                  ),
                  _TimeField(
                    controller: offTimeController,
                    label: '关闭时间',
                    onSubmit: onSubmitOff,
                  ),
                ];
                return wide
                    ? Row(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Expanded(child: fields[0]),
                          const SizedBox(width: 12),
                          Expanded(child: fields[1]),
                        ],
                      )
                    : Column(children: fields);
              },
            ),
          ],
        ),
      ),
    );
  }
}

class _TimeField extends StatelessWidget {
  const _TimeField({
    required this.controller,
    required this.label,
    required this.onSubmit,
  });

  final TextEditingController controller;
  final String label;
  final VoidCallback onSubmit;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 12),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          TextField(
            controller: controller,
            decoration: InputDecoration(
              labelText: label,
              hintText: '08:00:00',
              border: const OutlineInputBorder(),
            ),
            keyboardType: TextInputType.datetime,
          ),
          const SizedBox(height: 8),
          OutlinedButton.icon(
            onPressed: onSubmit,
            icon: const Icon(Icons.save),
            label: Text('同步$label'),
          ),
        ],
      ),
    );
  }
}

class _TimeSyncCard extends StatelessWidget {
  const _TimeSyncCard({
    required this.selectedDateTime,
    required this.onPickDate,
    required this.onPickTime,
    required this.onUseNow,
    required this.onSubmit,
  });

  final DateTime selectedDateTime;
  final VoidCallback onPickDate;
  final VoidCallback onPickTime;
  final VoidCallback onUseNow;
  final VoidCallback onSubmit;

  @override
  Widget build(BuildContext context) {
    final text =
        '${selectedDateTime.year}-${_pad(selectedDateTime.month)}-${_pad(selectedDateTime.day)} '
        '${_pad(selectedDateTime.hour)}:${_pad(selectedDateTime.minute)}:${_pad(selectedDateTime.second)}';
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            const ListTile(
              contentPadding: EdgeInsets.zero,
              leading: Icon(Icons.update),
              title: Text('RTC 校时'),
              subtitle: Text('发送 set_time 写入 DS1302'),
            ),
            SelectableText(text),
            const SizedBox(height: 12),
            Wrap(
              spacing: 10,
              runSpacing: 10,
              children: [
                OutlinedButton.icon(
                  onPressed: onPickDate,
                  icon: const Icon(Icons.calendar_month),
                  label: const Text('选择日期'),
                ),
                OutlinedButton.icon(
                  onPressed: onPickTime,
                  icon: const Icon(Icons.schedule),
                  label: const Text('选择时间'),
                ),
                OutlinedButton.icon(
                  onPressed: onUseNow,
                  icon: const Icon(Icons.access_time),
                  label: const Text('使用当前'),
                ),
                FilledButton.icon(
                  onPressed: onSubmit,
                  icon: const Icon(Icons.send),
                  label: const Text('发送校时'),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}

class _LogCard extends StatelessWidget {
  const _LogCard({required this.service});

  final ZnczService service;

  @override
  Widget build(BuildContext context) {
    return StreamBuilder<List<CommandLogEntry>>(
      stream: service.onLogs,
      builder: (context, snapshot) {
        final logs = snapshot.data ?? service.logs;
        return Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                Row(
                  children: [
                    const Expanded(
                      child: ListTile(
                        contentPadding: EdgeInsets.zero,
                        leading: Icon(Icons.terminal),
                        title: Text('通信日志'),
                      ),
                    ),
                    IconButton(
                      onPressed: logs.isEmpty ? null : () => _copyLogs(context, logs),
                      icon: const Icon(Icons.copy),
                      tooltip: '复制日志',
                    ),
                  ],
                ),
                if (logs.isEmpty)
                  const Text('暂无 TX/RX')
                else
                  ...logs.map((entry) => _LogLine(entry: entry)),
              ],
            ),
          ),
        );
      },
    );
  }

  void _copyLogs(BuildContext context, List<CommandLogEntry> logs) {
    final text = logs
        .map((item) => '${item.direction} ${_formatClock(item.timestamp)} ${item.payload}')
        .join('\n');
    Clipboard.setData(ClipboardData(text: text));
    ScaffoldMessenger.of(
      context,
    ).showSnackBar(const SnackBar(content: Text('日志已复制')));
  }
}

class _LogLine extends StatelessWidget {
  const _LogLine({required this.entry});

  final CommandLogEntry entry;

  @override
  Widget build(BuildContext context) {
    final isTx = entry.direction == 'TX';
    final color = isTx ? Colors.blue : Colors.orange;
    return Padding(
      padding: const EdgeInsets.only(bottom: 8),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 34,
            child: Text(
              entry.direction,
              style: TextStyle(color: color, fontWeight: FontWeight.bold),
            ),
          ),
          Expanded(
            child: SelectableText(
              '${_formatClock(entry.timestamp)}  ${entry.payload}',
              style: const TextStyle(fontFamily: 'monospace'),
            ),
          ),
        ],
      ),
    );
  }
}

String _pad(int value) => value.toString().padLeft(2, '0');

String _formatClock(DateTime time) =>
    '${_pad(time.hour)}:${_pad(time.minute)}:${_pad(time.second)}';
