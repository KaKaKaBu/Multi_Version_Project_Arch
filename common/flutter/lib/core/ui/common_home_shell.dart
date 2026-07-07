import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';

import '../connection/communication_controller.dart';
import '../transport/bluetooth_spp_transport.dart';
import '../version/version_capabilities.dart';
import 'connection_panel.dart';

class UpperPageSpec {
  const UpperPageSpec({
    required this.icon,
    required this.label,
    required this.child,
    this.visibleWhen,
  });

  final IconData icon;
  final String label;
  final Widget child;
  final bool Function(VersionCapabilities capabilities)? visibleWhen;

  bool visibleFor(VersionCapabilities capabilities) {
    final predicate = visibleWhen;
    return predicate == null || predicate(capabilities);
  }
}

class CommonHomeShell extends StatefulWidget {
  const CommonHomeShell({
    super.key,
    required this.title,
    required this.capabilities,
    required this.controller,
    required this.pages,
    this.onConnected,
  });

  final String title;
  final VersionCapabilities capabilities;
  final CommunicationController controller;
  final List<UpperPageSpec> pages;
  final FutureOr<void> Function()? onConnected;

  @override
  State<CommonHomeShell> createState() => _CommonHomeShellState();
}

class _CommonHomeShellState extends State<CommonHomeShell> {
  int _selectedIndex = 0;

  Future<void> _connect(Future<bool> Function() action) async {
    final ok = await action();
    if (ok) await widget.onConnected?.call();
  }

  Future<void> _connectBle() async {
    if (defaultTargetPlatform == TargetPlatform.windows) {
      await _connect(widget.controller.connectBleFromInput);
      return;
    }

    final devices = await widget.controller.scanBleDevices();
    if (!mounted || devices.isEmpty) return;

    final selected = await showModalBottomSheet<BluetoothSppDevice>(
      context: context,
      showDragHandle: true,
      builder: (context) => BleDeviceSheet(devices: devices),
    );

    if (selected == null) {
      widget.controller.cancelBleSelectionIfDisconnected();
      return;
    }

    await _connect(() => widget.controller.connectBleDevice(selected));
  }

  @override
  Widget build(BuildContext context) {
    final pages = widget.pages
        .where((page) => page.visibleFor(widget.capabilities))
        .toList();
    final effectiveIndex = pages.isEmpty
        ? 0
        : _selectedIndex.clamp(0, pages.length - 1).toInt();

    return Scaffold(
      appBar: AppBar(
        title: Text(widget.title),
        actions: [
          AnimatedBuilder(
            animation: widget.controller,
            builder: (context, _) {
              return IconButton(
                icon: Icon(
                  widget.controller.connected ? Icons.link : Icons.link_off,
                ),
                onPressed: widget.controller.connected
                    ? widget.controller.disconnect
                    : null,
                tooltip: widget.controller.connected ? '断开连接' : '未连接',
              );
            },
          ),
        ],
      ),
      body: LayoutBuilder(
        builder: (context, constraints) {
          final panelMaxHeight =
              (constraints.maxHeight * 0.36).clamp(96.0, 260.0).toDouble();
          return Column(
            children: [
              Container(
                width: double.infinity,
                padding: const EdgeInsets.all(8),
                color: Theme.of(context).colorScheme.surfaceContainerHighest,
                child: Text(
                  'V${widget.capabilities.version}  ${widget.capabilities.features.join(", ")}',
                  style: Theme.of(context).textTheme.bodySmall,
                  textAlign: TextAlign.center,
                ),
              ),
              ConstrainedBox(
                constraints: BoxConstraints(maxHeight: panelMaxHeight),
                child: SingleChildScrollView(
                  child: AnimatedBuilder(
                    animation: widget.controller,
                    builder: (context, _) {
                      return CommonConnectionPanel(
                        controller: widget.controller,
                        showBleAddressField:
                            defaultTargetPlatform == TargetPlatform.windows,
                        onConnectMqtt:
                            () => _connect(widget.controller.connectMqtt),
                        onConnectHuaweiCloud:
                            () => _connect(
                              widget.controller.connectHuaweiCloud,
                            ),
                        onConnectBle: _connectBle,
                        onDisconnect: widget.controller.disconnect,
                      );
                    },
                  ),
                ),
              ),
              Expanded(
                child: pages.isEmpty
                    ? const SizedBox.shrink()
                    : IndexedStack(
                        index: effectiveIndex,
                        children: pages.map((page) => page.child).toList(),
                      ),
              ),
            ],
          );
        },
      ),
      bottomNavigationBar: pages.length <= 1
          ? null
          : NavigationBar(
              selectedIndex: effectiveIndex,
              onDestinationSelected: (index) {
                setState(() => _selectedIndex = index);
              },
              destinations: pages
                  .map(
                    (page) => NavigationDestination(
                      icon: Icon(page.icon),
                      label: page.label,
                    ),
                  )
                  .toList(),
            ),
    );
  }
}
