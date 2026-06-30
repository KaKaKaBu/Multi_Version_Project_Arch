import 'package:flutter/material.dart';
import '../../services/fjxt_service.dart';

class SettingsPage extends StatelessWidget {
  final FjxtService service;

  const SettingsPage({super.key, required this.service});

  @override
  Widget build(BuildContext context) {
    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        Text('车窗控制', style: Theme.of(context).textTheme.titleMedium),
        const SizedBox(height: 12),
        GridView.count(
          crossAxisCount: 2,
          mainAxisSpacing: 12,
          crossAxisSpacing: 12,
          shrinkWrap: true,
          physics: const NeverScrollableScrollPhysics(),
          childAspectRatio: 2.4,
          children: [
            _CommandButton(
              icon: Icons.keyboard_double_arrow_up,
              label: '一键打开',
              onPressed: service.openFull,
            ),
            _CommandButton(
              icon: Icons.keyboard_double_arrow_down,
              label: '一键关闭',
              onPressed: service.closeFull,
            ),
            _CommandButton(
              icon: Icons.keyboard_arrow_up,
              label: '开一点停',
              onPressed: service.openStep,
            ),
            _CommandButton(
              icon: Icons.keyboard_arrow_down,
              label: '关一点停',
              onPressed: service.closeStep,
            ),
          ],
        ),
        const SizedBox(height: 12),
        OutlinedButton.icon(
          onPressed: service.stop,
          icon: const Icon(Icons.stop_circle),
          label: const Text('停止'),
        ),
        const SizedBox(height: 12),
        OutlinedButton.icon(
          onPressed: service.requestStatus,
          icon: const Icon(Icons.refresh),
          label: const Text('刷新状态'),
        ),
      ],
    );
  }
}

class _CommandButton extends StatelessWidget {
  final IconData icon;
  final String label;
  final VoidCallback onPressed;

  const _CommandButton({
    required this.icon,
    required this.label,
    required this.onPressed,
  });

  @override
  Widget build(BuildContext context) {
    return FilledButton.icon(
      onPressed: onPressed,
      icon: Icon(icon),
      label: Text(label),
    );
  }
}
