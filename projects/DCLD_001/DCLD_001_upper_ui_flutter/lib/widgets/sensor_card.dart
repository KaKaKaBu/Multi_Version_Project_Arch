import 'package:flutter/material.dart';

class SensorCard extends StatelessWidget {
  const SensorCard({
    super.key,
    required this.label,
    required this.value,
    required this.unit,
    required this.threshold,
    required this.precision,
    this.alarmWhenBelowThreshold = false,
  });

  final String label;
  final num? value;
  final String unit;
  final num? threshold;
  final int precision;
  final bool alarmWhenBelowThreshold;

  @override
  Widget build(BuildContext context) {
    final alarm = value != null && threshold != null && (alarmWhenBelowThreshold ? value! > 0 && value! < threshold! : value! > threshold!);
    final color = alarm ? const Color(0xffb42318) : const Color(0xff0f8f70);
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: alarm ? const Color(0xfffff3f0) : const Color(0xfff6fafc),
        borderRadius: BorderRadius.circular(18),
        border: Border.all(color: color.withValues(alpha: alarm ? 0.4 : 0.16)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Expanded(child: Text(label, style: const TextStyle(color: Color(0xff486581), fontWeight: FontWeight.w700))),
              Text(alarm ? '超限' : '正常', style: TextStyle(color: color, fontSize: 12, fontWeight: FontWeight.w800)),
            ],
          ),
          const SizedBox(height: 14),
          Text(_valueText(), style: TextStyle(color: color, fontSize: 30, fontWeight: FontWeight.w900)),
          const SizedBox(height: 6),
          Text('阈值 ${threshold ?? '--'} $unit', style: const TextStyle(color: Color(0xff697586), fontSize: 12)),
        ],
      ),
    );
  }

  String _valueText() {
    if (value == null) return '-- $unit';
    return '${value!.toStringAsFixed(precision)} $unit';
  }
}
