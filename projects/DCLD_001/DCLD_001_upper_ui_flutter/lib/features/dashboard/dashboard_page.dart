import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import 'package:mvp_flutter_common/core/config/debug_flags.dart';
import 'package:mvp_flutter_common/core/ui/adaptive_layout.dart';
import 'package:mvp_flutter_common/core/ui/common_home_shell.dart';
import 'package:mvp_flutter_common/core/ui/mjpeg_stream_view.dart';
import '../../core/version/version_capabilities.dart';
import '../../widgets/sensor_card.dart';
import '../../widgets/threshold_editor.dart';
import 'dashboard_controller.dart';

class DcldHomePage extends StatefulWidget {
  const DcldHomePage({super.key});

  @override
  State<DcldHomePage> createState() => _DcldHomePageState();
}

class _DcldHomePageState extends State<DcldHomePage> {
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
        return CommonHomeShell(
          title: 'DCLD_001 倒车雷达',
          capabilities: controller.commonCapabilities,
          controller: controller.communication,
          onConnected: controller.refreshStatus,
          pages: [
            UpperPageSpec(
              icon: Icons.sensors,
              label: '监测',
              child: _DcldMonitorPage(controller: controller),
            ),
            UpperPageSpec(
              icon: Icons.tune,
              label: '控制',
              child: _DcldControlPage(controller: controller),
            ),
          ],
        );
      },
    );
  }
}

class _DcldMonitorPage extends StatelessWidget {
  const _DcldMonitorPage({required this.controller});

  final DashboardController controller;

  @override
  Widget build(BuildContext context) {
    return AdaptivePage(
      children: [
        _HeroCard(controller: controller),
        _MetricSection(controller: controller),
        if (controller.cameraEnabled) _CameraSection(controller: controller),
        if (upperUiShowCommunicationLog) _LogSection(controller: controller),
      ],
    );
  }
}

class _DcldControlPage extends StatelessWidget {
  const _DcldControlPage({required this.controller});

  final DashboardController controller;

  @override
  Widget build(BuildContext context) {
    return AdaptivePage(
      children: [
        _ModeSection(controller: controller),
        _ThresholdSection(controller: controller),
      ],
    );
  }
}

class DashboardPage extends StatelessWidget {
  const DashboardPage({super.key});

  @override
  Widget build(BuildContext context) {
    return const DcldHomePage();
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
                constraints: const BoxConstraints(maxWidth: 560),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'DCLD_001 倒车雷达控制台',
                      style: Theme.of(context).textTheme.headlineSmall
                          ?.copyWith(
                            fontWeight: FontWeight.w900,
                            color: const Color(0xff102a43),
                          ),
                    ),
                    const SizedBox(height: 8),
                    Text(
                      controller.versionDescription,
                      style: const TextStyle(
                        color: Color(0xff486581),
                        height: 1.4,
                      ),
                    ),
                  ],
                ),
              ),
              Wrap(
                spacing: 8,
                runSpacing: 8,
                children: [
                  _Chip(
                    label: controller.alarmActive ? '距离报警' : '安全距离',
                    tone: controller.alarmActive ? _Tone.danger : _Tone.success,
                  ),
                  _Chip(label: controller.modeLabel, tone: _Tone.neutral),
                  _Chip(
                    label: controller.connectionLabel,
                    tone: controller.transportState == 'connected'
                        ? _Tone.success
                        : _Tone.neutral,
                  ),
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
                  _Metric(value: controller.distanceDisplay, label: '当前距离'),
                  _Metric(value: controller.thresholdDisplay, label: '报警阈值'),
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
              FilledButton(
                onPressed: controller.pending ? null : controller.refreshStatus,
                child: const Text('刷新状态'),
              ),
            ],
          ),
        ],
      ),
    );
  }
}

class _MetricSection extends StatelessWidget {
  const _MetricSection({required this.controller});

  final DashboardController controller;

  @override
  Widget build(BuildContext context) {
    return _Card(
      title: '实时数据',
      trailing: _Chip(
        label: controller.alarmActive ? '低于阈值' : '距离安全',
        tone: controller.alarmActive ? _Tone.danger : _Tone.success,
      ),
      child: LayoutBuilder(
        builder: (context, constraints) {
          final columns = constraints.maxWidth > 760
              ? 3
              : constraints.maxWidth > 460
              ? 2
              : 1;
          return GridView.count(
            crossAxisCount: columns,
            shrinkWrap: true,
            physics: const NeverScrollableScrollPhysics(),
            crossAxisSpacing: 12,
            mainAxisSpacing: 12,
            childAspectRatio: columns == 1 ? 2.5 : 1.75,
            children: [
              for (final metric in controller.supportedMetrics)
                SensorCard(
                  label: metric.label,
                  value: controller.telemetry[metric.key],
                  unit: metric.unit,
                  threshold: metric.thresholdKey == null
                      ? null
                      : controller.thresholds[metric.thresholdKey!],
                  precision: metric.precision,
                  alarmWhenBelowThreshold: metric.thresholdKey != null,
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
              onSelected: controller.pendingMode == null
                  ? (_) => controller.setMode(mode)
                  : null,
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
      trailing: const _Chip(label: '距离越近越危险', tone: _Tone.neutral),
      child: Column(
        children: [
          for (final threshold in controller.supportedThresholds)
            ThresholdEditor(
              label: threshold.label,
              unit: threshold.unit,
              step: threshold.step,
              value:
                  controller.thresholds[threshold.key] ??
                  threshold.defaultValue,
              pending: controller.pendingThreshold == threshold.key,
              onChanged: (value) =>
                  controller.updateThreshold(threshold.key, value),
              onApply: () => controller.applyThreshold(threshold.key),
            ),
          const Align(
            alignment: Alignment.centerLeft,
            child: Text(
              '设备端 Key1 切换自动/阈值设置，Key2/Key3 调整阈值；App 同步后会下发 set_threshold。',
              style: TextStyle(color: Color(0xff697586), fontSize: 12),
            ),
          ),
        ],
      ),
    );
  }
}

class _CameraSection extends StatelessWidget {
  const _CameraSection({required this.controller});

  final DashboardController controller;

  @override
  Widget build(BuildContext context) {
    final url = controller.cameraUrl;
    return _Card(
      title: 'ESP32-CAM 视频',
      trailing: const _Chip(label: '外部流地址', tone: _Tone.neutral),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            url.isEmpty ? '等待设备通过 telemetry.camera_url 上报视频流地址。' : url,
            style: const TextStyle(
              color: Color(0xff486581),
              fontFamily: 'monospace',
            ),
          ),
          const SizedBox(height: 12),
          MjpegStreamView(url: url),
          const SizedBox(height: 12),
          Wrap(
            spacing: 12,
            runSpacing: 12,
            children: [
              OutlinedButton(
                onPressed: url.isEmpty
                    ? null
                    : () {
                        Clipboard.setData(ClipboardData(text: url));
                        ScaffoldMessenger.of(context).showSnackBar(
                          const SnackBar(content: Text('摄像头地址已复制')),
                        );
                      },
                child: const Text('复制地址'),
              ),
              const Text(
                'Android 内嵌 WebView 播放 HTTP MJPEG；需手机与摄像头处于同一局域网。',
                style: TextStyle(color: Color(0xff697586)),
              ),
            ],
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
                Clipboard.setData(
                  ClipboardData(text: controller.copyableLogs()),
                );
                ScaffoldMessenger.of(
                  context,
                ).showSnackBar(const SnackBar(content: Text('日志已复制')));
              },
        child: const Text('复制'),
      ),
      child: controller.logs.isEmpty
          ? const Text(
              '暂无日志，连接设备后会显示 TX/RX JSON。',
              style: TextStyle(color: Color(0xff697586)),
            )
          : Column(
              children: [
                for (final item in controller.logs)
                  Container(
                    margin: const EdgeInsets.only(bottom: 8),
                    padding: const EdgeInsets.all(10),
                    decoration: BoxDecoration(
                      color: const Color(0xfff6fafc),
                      borderRadius: BorderRadius.circular(12),
                    ),
                    child: Row(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        SizedBox(
                          width: 42,
                          child: Text(
                            item.direction,
                            style: TextStyle(
                              color: item.direction == 'TX'
                                  ? const Color(0xff0f8f70)
                                  : const Color(0xff2563eb),
                              fontWeight: FontWeight.w900,
                            ),
                          ),
                        ),
                        Expanded(
                          child: Text(
                            item.payload,
                            style: const TextStyle(
                              fontFamily: 'monospace',
                              fontSize: 12,
                            ),
                          ),
                        ),
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
        boxShadow: const [
          BoxShadow(
            color: Color(0x14104d5e),
            blurRadius: 24,
            offset: Offset(0, 10),
          ),
        ],
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
                Text(
                  title!,
                  style: Theme.of(context).textTheme.titleLarge?.copyWith(
                    fontWeight: FontWeight.w900,
                    color: const Color(0xff102a43),
                  ),
                ),
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
      decoration: BoxDecoration(
        color: const Color(0xfff6fafc),
        borderRadius: BorderRadius.circular(18),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Text(
            value,
            maxLines: 2,
            overflow: TextOverflow.ellipsis,
            style: const TextStyle(
              color: Color(0xff2563eb),
              fontSize: 20,
              fontWeight: FontWeight.w900,
            ),
          ),
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
      decoration: BoxDecoration(
        color: color.withValues(alpha: 0.12),
        borderRadius: BorderRadius.circular(99),
      ),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
        child: Text(
          label,
          style: TextStyle(
            color: color,
            fontSize: 12,
            fontWeight: FontWeight.w800,
          ),
        ),
      ),
    );
  }
}
