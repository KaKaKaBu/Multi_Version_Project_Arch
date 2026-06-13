import 'package:flutter/material.dart';

class DeviceSwitch extends StatelessWidget {
  const DeviceSwitch({
    super.key,
    required this.label,
    required this.value,
    required this.onChanged,
    this.disabled = false,
  });

  final String label;
  final bool value;
  final bool disabled;
  final ValueChanged<bool> onChanged;

  @override
  Widget build(BuildContext context) {
    return Container(
      margin: const EdgeInsets.only(bottom: 12),
      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
      decoration: BoxDecoration(color: const Color(0xfff6fafc), borderRadius: BorderRadius.circular(16)),
      child: Row(
        children: [
          Expanded(child: Text(label, style: const TextStyle(fontWeight: FontWeight.w700, color: Color(0xff102a43)))),
          Text(value ? 'ON' : 'OFF', style: TextStyle(color: value ? const Color(0xff0f8f70) : const Color(0xff697586), fontWeight: FontWeight.w800)),
          const SizedBox(width: 10),
          Switch(value: value, onChanged: disabled ? null : onChanged, activeThumbColor: const Color(0xff0f8f70)),
        ],
      ),
    );
  }
}
