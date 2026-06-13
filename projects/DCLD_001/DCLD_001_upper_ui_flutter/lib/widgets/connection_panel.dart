import 'package:flutter/material.dart';

class ConnectionPanel extends StatelessWidget {
  const ConnectionPanel({
    super.key,
    required this.title,
    required this.description,
    required this.state,
    required this.child,
  });

  final String title;
  final String description;
  final String state;
  final Widget child;

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: const Color(0xfff6fafc),
        borderRadius: BorderRadius.circular(18),
        border: Border.all(color: _stateColor(state).withValues(alpha: 0.3)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Wrap(
            alignment: WrapAlignment.spaceBetween,
            crossAxisAlignment: WrapCrossAlignment.center,
            spacing: 12,
            runSpacing: 8,
            children: [
              Text(title, style: Theme.of(context).textTheme.titleMedium?.copyWith(fontWeight: FontWeight.w800)),
              _Pill(label: _stateLabel(state), color: _stateColor(state)),
            ],
          ),
          const SizedBox(height: 8),
          Text(description, style: const TextStyle(color: Color(0xff486581))),
          const SizedBox(height: 16),
          child,
        ],
      ),
    );
  }

  static String _stateLabel(String state) {
    return switch (state) {
      'connected' => '已连接',
      'connecting' => '连接中',
      'scanning' => '扫描中',
      'unsupported' => '不支持',
      _ => '未连接',
    };
  }

  static Color _stateColor(String state) {
    return switch (state) {
      'connected' => const Color(0xff0f8f70),
      'connecting' || 'scanning' => const Color(0xffb7791f),
      'unsupported' => const Color(0xff697586),
      _ => const Color(0xff697586),
    };
  }
}

class _Pill extends StatelessWidget {
  const _Pill({required this.label, required this.color});

  final String label;
  final Color color;

  @override
  Widget build(BuildContext context) {
    return DecoratedBox(
      decoration: BoxDecoration(color: color.withValues(alpha: 0.12), borderRadius: BorderRadius.circular(99)),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
        child: Text(label, style: TextStyle(color: color, fontWeight: FontWeight.w700, fontSize: 12)),
      ),
    );
  }
}
