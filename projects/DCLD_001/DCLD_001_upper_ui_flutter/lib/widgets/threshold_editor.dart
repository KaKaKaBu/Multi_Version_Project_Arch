import 'package:flutter/material.dart';

class ThresholdEditor extends StatelessWidget {
  const ThresholdEditor({
    super.key,
    required this.label,
    required this.unit,
    required this.step,
    required this.value,
    required this.onChanged,
    required this.onApply,
    this.pending = false,
  });

  final String label;
  final String unit;
  final num step;
  final num value;
  final ValueChanged<num> onChanged;
  final VoidCallback onApply;
  final bool pending;

  @override
  Widget build(BuildContext context) {
    final controller = TextEditingController(text: value.toString());
    return Container(
      margin: const EdgeInsets.only(bottom: 12),
      padding: const EdgeInsets.all(14),
      decoration: BoxDecoration(color: const Color(0xfff6fafc), borderRadius: BorderRadius.circular(16)),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Expanded(child: Text(label, style: const TextStyle(fontWeight: FontWeight.w800, color: Color(0xff102a43)))),
              Text('步进 ±$step $unit', style: const TextStyle(color: Color(0xff697586), fontSize: 12)),
            ],
          ),
          const SizedBox(height: 10),
          Row(
            children: [
              IconButton.filledTonal(onPressed: () => onChanged(value - step), icon: const Icon(Icons.remove)),
              const SizedBox(width: 8),
              Expanded(
                child: TextField(
                  controller: controller,
                  keyboardType: TextInputType.number,
                  textAlign: TextAlign.center,
                  decoration: InputDecoration(suffixText: unit, isDense: true, border: const OutlineInputBorder()),
                  onSubmitted: (text) => onChanged(num.tryParse(text) ?? value),
                ),
              ),
              const SizedBox(width: 8),
              IconButton.filledTonal(onPressed: () => onChanged(value + step), icon: const Icon(Icons.add)),
              const SizedBox(width: 8),
              FilledButton(onPressed: pending ? null : onApply, child: Text(pending ? '同步中' : '同步')),
            ],
          ),
        ],
      ),
    );
  }
}
