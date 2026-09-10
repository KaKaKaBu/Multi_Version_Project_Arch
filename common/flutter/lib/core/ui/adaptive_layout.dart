import 'package:flutter/material.dart';

class AdaptivePage extends StatelessWidget {
  const AdaptivePage({
    super.key,
    required this.children,
    this.padding = const EdgeInsets.all(16),
    this.maxWidth = 980,
  });

  final List<Widget> children;
  final EdgeInsetsGeometry padding;
  final double maxWidth;

  @override
  Widget build(BuildContext context) {
    return LayoutBuilder(
      builder: (context, constraints) {
        return ListView(
          padding: padding,
          children: [
            Center(
              child: ConstrainedBox(
                constraints: BoxConstraints(maxWidth: maxWidth),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: children,
                ),
              ),
            ),
          ],
        );
      },
    );
  }
}

class AdaptiveSectionCard extends StatelessWidget {
  const AdaptiveSectionCard({
    super.key,
    this.title,
    this.trailing,
    required this.child,
    this.margin = const EdgeInsets.only(bottom: 16),
    this.padding = const EdgeInsets.all(16),
  });

  final String? title;
  final Widget? trailing;
  final Widget child;
  final EdgeInsetsGeometry margin;
  final EdgeInsetsGeometry padding;

  @override
  Widget build(BuildContext context) {
    return Card(
      margin: margin,
      child: Padding(
        padding: padding,
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            if (title != null || trailing != null) ...[
              Wrap(
                alignment: WrapAlignment.spaceBetween,
                crossAxisAlignment: WrapCrossAlignment.center,
                spacing: 12,
                runSpacing: 8,
                children: [
                  if (title != null)
                    Text(
                      title!,
                      style: Theme.of(context).textTheme.titleMedium?.copyWith(
                        fontWeight: FontWeight.w700,
                      ),
                    ),
                  if (trailing != null) trailing!,
                ],
              ),
              const SizedBox(height: 12),
            ],
            child,
          ],
        ),
      ),
    );
  }
}

class AdaptiveMetricGrid extends StatelessWidget {
  const AdaptiveMetricGrid({
    super.key,
    required this.children,
    this.minTileWidth = 168,
    this.minTileHeight = 104,
    this.spacing = 12,
    this.singleColumnAspectRatio = 2.8,
    this.multiColumnAspectRatio = 1.75,
  });

  final List<Widget> children;
  final double minTileWidth;
  final double minTileHeight;
  final double spacing;
  final double singleColumnAspectRatio;
  final double multiColumnAspectRatio;

  @override
  Widget build(BuildContext context) {
    if (children.isEmpty) return const SizedBox.shrink();

    return LayoutBuilder(
      builder: (context, constraints) {
        final width = constraints.maxWidth.isFinite
            ? constraints.maxWidth
            : MediaQuery.sizeOf(context).width;
        final columns = (width / minTileWidth).floor().clamp(1, 4).toInt();
        final totalSpacing = spacing * (columns - 1);
        final tileWidth = (width - totalSpacing) / columns;
        final preferredAspectRatio = columns == 1
            ? singleColumnAspectRatio
            : multiColumnAspectRatio;
        final preferredHeight = tileWidth / preferredAspectRatio;
        final tileHeight = preferredHeight < minTileHeight
            ? minTileHeight
            : preferredHeight;

        return GridView.count(
          crossAxisCount: columns,
          crossAxisSpacing: spacing,
          mainAxisSpacing: spacing,
          shrinkWrap: true,
          physics: const NeverScrollableScrollPhysics(),
          childAspectRatio: tileWidth / tileHeight,
          children: children,
        );
      },
    );
  }
}

class AdaptiveButtonGrid extends StatelessWidget {
  const AdaptiveButtonGrid({
    super.key,
    required this.children,
    this.minTileWidth = 150,
    this.minTileHeight = 52,
    this.spacing = 12,
  });

  final List<Widget> children;
  final double minTileWidth;
  final double minTileHeight;
  final double spacing;

  @override
  Widget build(BuildContext context) {
    return AdaptiveMetricGrid(
      minTileWidth: minTileWidth,
      minTileHeight: minTileHeight,
      spacing: spacing,
      singleColumnAspectRatio: 4.2,
      multiColumnAspectRatio: 2.7,
      children: children,
    );
  }
}

class AdaptiveFieldRow extends StatelessWidget {
  const AdaptiveFieldRow({
    super.key,
    required this.label,
    required this.child,
    this.labelWidth = 148,
  });

  final String label;
  final Widget child;
  final double labelWidth;

  @override
  Widget build(BuildContext context) {
    return Card(
      margin: const EdgeInsets.only(bottom: 10),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
        child: LayoutBuilder(
          builder: (context, constraints) {
            if (constraints.maxWidth < 360) {
              return Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [Text(label), const SizedBox(height: 8), child],
              );
            }

            return Row(
              children: [
                SizedBox(width: labelWidth, child: Text(label)),
                Expanded(child: child),
              ],
            );
          },
        ),
      ),
    );
  }
}
