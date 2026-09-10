import 'package:flutter/material.dart';

import 'adaptive_layout.dart';

class DeviceCommandAction {
  const DeviceCommandAction({
    required this.icon,
    required this.label,
    required this.onPressed,
  });

  final IconData icon;
  final String label;
  final VoidCallback onPressed;
}

class CommonControlPage extends StatelessWidget {
  const CommonControlPage({
    super.key,
    required this.title,
    required this.primaryActions,
    this.secondaryActions = const <DeviceCommandAction>[],
    this.embedded = false,
  });

  final String title;
  final List<DeviceCommandAction> primaryActions;
  final List<DeviceCommandAction> secondaryActions;
  final bool embedded;

  @override
  Widget build(BuildContext context) {
    final card = AdaptiveSectionCard(
      title: title,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          AdaptiveButtonGrid(
            children: primaryActions
                .map(
                  (action) => FilledButton.icon(
                    onPressed: action.onPressed,
                    icon: Icon(action.icon),
                    label: Text(action.label, textAlign: TextAlign.center),
                  ),
                )
                .toList(),
          ),
          for (final action in secondaryActions) ...[
            const SizedBox(height: 12),
            OutlinedButton.icon(
              onPressed: action.onPressed,
              icon: Icon(action.icon),
              label: Text(action.label),
            ),
          ],
        ],
      ),
    );

    if (embedded) return card;
    return AdaptivePage(children: [card]);
  }
}
