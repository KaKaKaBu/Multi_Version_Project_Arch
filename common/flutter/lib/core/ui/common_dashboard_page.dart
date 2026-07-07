import 'package:flutter/material.dart';

import 'adaptive_layout.dart';

class CommonDashboardPage<T> extends StatefulWidget {
  const CommonDashboardPage({
    super.key,
    required this.stream,
    required this.latestData,
    required this.sectionsBuilder,
    this.onInit,
  });

  final Stream<T> stream;
  final T latestData;
  final VoidCallback? onInit;
  final List<Widget> Function(BuildContext context, T data) sectionsBuilder;

  @override
  State<CommonDashboardPage<T>> createState() => _CommonDashboardPageState<T>();
}

class _CommonDashboardPageState<T> extends State<CommonDashboardPage<T>> {
  @override
  void initState() {
    super.initState();
    widget.onInit?.call();
  }

  @override
  Widget build(BuildContext context) {
    return StreamBuilder<T>(
      stream: widget.stream,
      initialData: widget.latestData,
      builder: (context, snapshot) {
        final data = snapshot.data ?? widget.latestData;
        return AdaptivePage(
          children: widget.sectionsBuilder(context, data),
        );
      },
    );
  }
}
