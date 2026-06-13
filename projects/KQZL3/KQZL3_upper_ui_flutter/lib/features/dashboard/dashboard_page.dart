import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../../core/config/debug_flags.dart';
import '../../core/version/version_capabilities.dart';
import '../../widgets/connection_panel.dart';
import '../../widgets/device_switch.dart';
import '../../widgets/sensor_card.dart';
import '../../widgets/threshold_editor.dart';
import 'dashboard_controller.dart';

class DashboardPage extends StatefulWidget {
  const DashboardPage({super.key});

  @override
  State<DashboardPage> createState() => _DashboardPageState();
}

class _DashboardPageState extends State<DashboardPage> {
  late final DashboardController controller;

  @override
  void initState() {
    super.initState();
    controller = DashboardController();
  }

  @override
  void dispose() {
    controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return ListenableBuilder(
      listenable: controller,
      builder: (context, _) {
        return Scaffold(
          body: Container(
            decoration: const BoxDecoration(
              gradient: LinearGradient(
                begin: Alignment.topCenter,
                end: Alignment.bottomCenter,
                colors: [Color(0xffe1f5ef), Color(0xffeef4f7)],
                stops: [0, 0.35],
              ),
            ),
            child: SafeArea(
              child: SingleChildScrollView(
                padding: const EdgeInsets.all(16),
                child: Center(
                  child: ConstrainedBox(
                    constraints: const BoxConstraints(maxWidth: 980),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.stretch,
                      children: [
                        _HeroCard(controller: controller),
                        _ConnectionCard(controller: controller),
                        _SensorSection(controller: controller),
                        _ModeSection(controller: controller),
                        _DeviceSection(controller: controller),
                        _ThresholdSection(controller: controller),
                        if (upperUiShowCommunicationLog) _LogSection(controller: controller),
                      ],
                    ),
                  ),
                ),
              ),
            ),
          ),
        );
      },
    );
  }
}

class _HeroCard extends StatelessWidget {
  const _HeroCard({required this.controller});

  final DashboardController controller;

  @override
  Widget build(BuildContext context) {
    return _Card(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Wrap(
            alignment: WrapAlignment.spaceBetween,
            crossAxisAlignment: WrapCrossAlignment.center,
            runSpacing: 12,
            children: [
              ConstrainedBox(
                constraints: const BoxConstraints(maxWidth: 520),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text('KQZL3 空气质量控制台', style: Theme.of(context).textTheme.headlineSmall?.copyWith(fontWeight: FontWeight.w900, color: const Color(0xff102a43))),
                    const SizedBox(height: 8),
                    Text(controller.versionDescription, style: const TextStyle(color: Color(0xff486581), height: 1.4)),
                  ],
                ),
              ),
              Wrap(
                spacing: 8,
                runSpacing: 8,
                children: [
                  _Chip(label: controller.alarmActive ? '报警' : '正常', tone: controller.alarmActive ? _Tone.danger : _Tone.success),
                  _Chip(label: controller.modeLabel, tone: _Tone.neutral),
                  _Chip(label: controller.connectionLabel, tone: controller.transportState == 'connected' ? _Tone.success : _Tone.neutral),
                ],
              ),
            ],
          ),
          const SizedBox(height: 20),
          LayoutBuilder(
            builder: (context, constraints) {
              final columns = constraints.maxWidth > 720 ? 3 : 1;
              return GridView.count(
                crossAxisCount: columns,
                shrinkWrap: true,
                crossAxisSpacing: 12,
                mainAxisSpacing: 12,
                physics: const NeverScrollableScrollPhysics(),
                childAspectRatio: columns == 1 ? 2.8 : 1.6,
                children: [
                  _Metric(value: controller.telemetry['pm25']?.toString() ?? '--', label: 'PM2.5 μg/m³'),
                  _Metric(value: controller.enabledSensorLabels, label: '当前版本传感器'),
                  _Metric(value: controller.lastSyncDisplay, label: '最近同步'),
                ],
              );
            },
          ),
          const SizedBox(height: 16),
          Wrap(
            spacing: 12,
            runSpacing: 12,
            children: [
              FilledButton(onPressed: controller.pending ? null : controller.refreshStatus, child: const Text('刷新状态')),
            ],
          ),
        ],
      ),
    );
  }
}

class _ConnectionCard extends StatelessWidget {
  const _ConnectionCard({required this.controller});

  final DashboardController controller;

  @override
  Widget build(BuildContext context) {
    return _Card(
      title: '通信连接',
      child: ConnectionPanel(
        title: controller.connectionTitle,
        description: controller.connectionDescription,
        state: controller.connectionPanelState,
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            if (controller.capabilities.has('wifi')) _MqttForm(controller: controller),
            if (controller.capabilities.has('ble'))
              const Padding(
                padding: EdgeInsets.only(bottom: 12),
                child: Text('Android App 真机可连接已配对 JDY-31 经典蓝牙 SPP 设备，使用透明串口 JSON 协议。', style: TextStyle(color: Color(0xff486581))),
              ),
            Wrap(
              spacing: 12,
              runSpacing: 12,
              children: [
                FilledButton(
                  onPressed: controller.canConnect && !controller.connecting ? controller.connectTransport : null,
                  child: Text(controller.connecting ? '连接中' : '连接'),
                ),
                OutlinedButton(onPressed: controller.disconnectTransport, child: const Text('断开')),
              ],
            ),
          ],
        ),
      ),
    );
  }
}

class _MqttForm extends StatelessWidget {
  const _MqttForm({required this.controller});

  final DashboardController controller;

  @override
  Widget build(BuildContext context) {
    final config = controller.connection;
    return Column(
      children: [
        _TextFieldRow(
          children: [
            _ConfigField(label: 'MQTT WebSocket Endpoint', initialValue: config.wsEndpoint, onChanged: (value) => controller.updateConnection(config.copyWith(wsEndpoint: value))),
          ],
        ),
        _TextFieldRow(
          children: [
            _ConfigField(label: 'Broker Host', initialValue: config.host, onChanged: (value) => controller.updateConnection(controller.connection.copyWith(host: value))),
            _ConfigField(label: 'Port', initialValue: config.port.toString(), keyboardType: TextInputType.number, onChanged: (value) => controller.updateConnection(controller.connection.copyWith(port: int.tryParse(value) ?? config.port))),
          ],
        ),
        _TextFieldRow(
          children: [
            _ConfigField(label: '命令 Topic', initialValue: config.commandTopic, onChanged: (value) => controller.updateConnection(controller.connection.copyWith(commandTopic: value))),
            _ConfigField(label: '遥测 Topic', initialValue: config.telemetryTopic, onChanged: (value) => controller.updateConnection(controller.connection.copyWith(telemetryTopic: value))),
          ],
        ),
        const SizedBox(height: 12),
      ],
    );
  }
}

class _TextFieldRow extends StatelessWidget {
  const _TextFieldRow({required this.children});

  final List<Widget> children;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 10),
      child: LayoutBuilder(
        builder: (context, constraints) {
          if (children.length == 1 || constraints.maxWidth < 620) {
            return Column(children: children.map((child) => Padding(padding: const EdgeInsets.only(bottom: 10), child: child)).toList());
          }
          return Row(children: children.map((child) => Expanded(child: Padding(padding: const EdgeInsets.only(right: 10), child: child))).toList());
        },
      ),
    );
  }
}

class _ConfigField extends StatefulWidget {
  const _ConfigField({required this.label, required this.initialValue, required this.onChanged, this.keyboardType});

  final String label;
  final String initialValue;
  final ValueChanged<String> onChanged;
  final TextInputType? keyboardType;

  @override
  State<_ConfigField> createState() => _ConfigFieldState();
}

class _ConfigFieldState extends State<_ConfigField> {
  late final TextEditingController textController;

  @override
  void initState() {
    super.initState();
    textController = TextEditingController(text: widget.initialValue);
  }

  @override
  void dispose() {
    textController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return TextField(
      controller: textController,
      keyboardType: widget.keyboardType,
      decoration: InputDecoration(labelText: widget.label, isDense: true, border: const OutlineInputBorder()),
      onChanged: widget.onChanged,
    );
  }
}

class _SensorSection extends StatelessWidget {
  const _SensorSection({required this.controller});

  final DashboardController controller;

  @override
  Widget build(BuildContext context) {
    return _Card(
      title: '实时数据',
      trailing: _Chip(label: controller.alarmActive ? '阈值超限' : '空气状态正常', tone: controller.alarmActive ? _Tone.danger : _Tone.success),
      child: LayoutBuilder(
        builder: (context, constraints) {
          final columns = constraints.maxWidth > 760 ? 3 : constraints.maxWidth > 460 ? 2 : 1;
          return GridView.count(
            crossAxisCount: columns,
            shrinkWrap: true,
            physics: const NeverScrollableScrollPhysics(),
            crossAxisSpacing: 12,
            mainAxisSpacing: 12,
            childAspectRatio: columns == 1 ? 2.5 : 1.75,
            children: [
              for (final sensor in controller.supportedSensors)
                SensorCard(
                  label: sensor.label,
                  value: controller.telemetry[sensor.key],
                  unit: sensor.unit,
                  threshold: controller.thresholds[sensor.thresholdKey],
                  precision: sensor.precision,
                ),
            ],
          );
        },
      ),
    );
  }
}

class _ModeSection extends StatelessWidget {
  const _ModeSection({required this.controller});

  final DashboardController controller;

  @override
  Widget build(BuildContext context) {
    return _Card(
      title: '工作模式',
      trailing: _Chip(label: controller.modeLabel, tone: _Tone.neutral),
      child: Wrap(
        spacing: 12,
        runSpacing: 12,
        children: [
          for (final mode in modeOptions)
            ChoiceChip(
              label: Text(mode.label),
              selected: controller.telemetry.mode == mode,
              onSelected: controller.pendingMode == null ? (_) => controller.setMode(mode) : null,
            ),
        ],
      ),
    );
  }
}

class _DeviceSection extends StatelessWidget {
  const _DeviceSection({required this.controller});

  final DashboardController controller;

  @override
  Widget build(BuildContext context) {
    return _Card(
      title: '远程控制',
      trailing: const _Chip(label: '手动模式优先', tone: _Tone.neutral),
      child: Column(
        children: [
          for (final device in controller.supportedActuators)
            DeviceSwitch(
              label: device.label,
              value: (controller.telemetry[device.key] ?? 0) != 0,
              disabled: controller.pending,
              onChanged: (state) => controller.setDevice(device.key, state),
            ),
        ],
      ),
    );
  }
}

class _ThresholdSection extends StatelessWidget {
  const _ThresholdSection({required this.controller});

  final DashboardController controller;

  @override
  Widget build(BuildContext context) {
    return _Card(
      title: '阈值设置',
      trailing: const _Chip(label: '自动报警依据', tone: _Tone.neutral),
      child: Column(
        children: [
          for (final sensor in controller.supportedThresholds)
            ThresholdEditor(
              label: sensor.label,
              unit: sensor.unit,
              step: sensor.step,
              value: controller.thresholds[sensor.thresholdKey] ?? sensor.defaultThreshold,
              pending: controller.pendingThreshold == sensor.thresholdKey,
              onChanged: (value) => controller.updateThreshold(sensor.thresholdKey, value),
              onApply: () => controller.applyThreshold(sensor.thresholdKey),
            ),
        ],
      ),
    );
  }
}

class _LogSection extends StatelessWidget {
  const _LogSection({required this.controller});

  final DashboardController controller;

  @override
  Widget build(BuildContext context) {
    return _Card(
      title: '通信日志',
      trailing: OutlinedButton(
        onPressed: controller.logs.isEmpty
            ? null
            : () {
                Clipboard.setData(ClipboardData(text: controller.copyableLogs()));
                ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text('日志已复制')));
              },
        child: const Text('复制'),
      ),
      child: controller.logs.isEmpty
          ? const Text('暂无日志，连接设备后会显示 TX/RX JSON。', style: TextStyle(color: Color(0xff697586)))
          : Column(
              children: [
                for (final item in controller.logs)
                  Container(
                    margin: const EdgeInsets.only(bottom: 8),
                    padding: const EdgeInsets.all(10),
                    decoration: BoxDecoration(color: const Color(0xfff6fafc), borderRadius: BorderRadius.circular(12)),
                    child: Row(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        SizedBox(width: 42, child: Text(item.direction, style: TextStyle(color: item.direction == 'TX' ? const Color(0xff0f8f70) : const Color(0xff2563eb), fontWeight: FontWeight.w900))),
                        Expanded(child: Text(item.payload, style: const TextStyle(fontFamily: 'monospace', fontSize: 12))),
                      ],
                    ),
                  ),
              ],
            ),
    );
  }
}

class _Card extends StatelessWidget {
  const _Card({this.title, this.trailing, required this.child});

  final String? title;
  final Widget? trailing;
  final Widget child;

  @override
  Widget build(BuildContext context) {
    return Container(
      margin: const EdgeInsets.only(bottom: 16),
      padding: const EdgeInsets.all(18),
      decoration: BoxDecoration(
        color: Colors.white.withValues(alpha: 0.96),
        borderRadius: BorderRadius.circular(22),
        boxShadow: const [BoxShadow(color: Color(0x14104d5e), blurRadius: 24, offset: Offset(0, 10))],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          if (title != null) ...[
            Wrap(
              alignment: WrapAlignment.spaceBetween,
              crossAxisAlignment: WrapCrossAlignment.center,
              spacing: 12,
              runSpacing: 8,
              children: [
                Text(title!, style: Theme.of(context).textTheme.titleLarge?.copyWith(fontWeight: FontWeight.w900, color: const Color(0xff102a43))),
                ?trailing,
              ],
            ),
            const SizedBox(height: 14),
          ],
          child,
        ],
      ),
    );
  }
}

class _Metric extends StatelessWidget {
  const _Metric({required this.value, required this.label});

  final String value;
  final String label;

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(color: const Color(0xfff6fafc), borderRadius: BorderRadius.circular(18)),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Text(value, maxLines: 2, overflow: TextOverflow.ellipsis, style: const TextStyle(color: Color(0xff0f8f70), fontSize: 20, fontWeight: FontWeight.w900)),
          const SizedBox(height: 6),
          Text(label, style: const TextStyle(color: Color(0xff697586))),
        ],
      ),
    );
  }
}

enum _Tone { success, danger, neutral }

class _Chip extends StatelessWidget {
  const _Chip({required this.label, required this.tone});

  final String label;
  final _Tone tone;

  @override
  Widget build(BuildContext context) {
    final color = switch (tone) {
      _Tone.success => const Color(0xff0f8f70),
      _Tone.danger => const Color(0xffb42318),
      _Tone.neutral => const Color(0xff334e68),
    };
    return DecoratedBox(
      decoration: BoxDecoration(color: color.withValues(alpha: 0.12), borderRadius: BorderRadius.circular(99)),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
        child: Text(label, style: TextStyle(color: color, fontSize: 12, fontWeight: FontWeight.w800)),
      ),
    );
  }
}
