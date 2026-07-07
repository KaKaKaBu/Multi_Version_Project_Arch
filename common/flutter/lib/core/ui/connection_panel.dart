import 'package:flutter/material.dart';

import '../connection/communication_controller.dart';
import '../transport/bluetooth_spp_transport.dart';

class CommonConnectionPanel extends StatelessWidget {
  const CommonConnectionPanel({
    super.key,
    required this.controller,
    required this.showBleAddressField,
    required this.onConnectMqtt,
    required this.onConnectHuaweiCloud,
    required this.onConnectBle,
    required this.onDisconnect,
  });

  final CommunicationController controller;
  final bool showBleAddressField;
  final VoidCallback onConnectMqtt;
  final VoidCallback onConnectHuaweiCloud;
  final VoidCallback onConnectBle;
  final VoidCallback onDisconnect;

  @override
  Widget build(BuildContext context) {
    return Material(
      color: Theme.of(context).colorScheme.surface,
      child: Padding(
        padding: const EdgeInsets.fromLTRB(16, 10, 16, 12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Wrap(
              spacing: 10,
              runSpacing: 10,
              crossAxisAlignment: WrapCrossAlignment.center,
              children: [
                Chip(
                  avatar: Icon(
                    controller.connected
                        ? Icons.check_circle
                        : Icons.radio_button_unchecked,
                    size: 18,
                  ),
                  label: Text(controller.label),
                ),
                if (controller.mqttEnabled)
                  FilledButton.icon(
                    onPressed: !controller.busy ? onConnectMqtt : null,
                    icon: const Icon(Icons.cloud),
                    label: const Text('MQTT'),
                  ),
                if (controller.cloudEnabled)
                  FilledButton.icon(
                    onPressed: !controller.busy ? onConnectHuaweiCloud : null,
                    icon: const Icon(Icons.cloud_sync),
                    label: const Text('华为云'),
                  ),
                if (controller.bleEnabled)
                  FilledButton.tonalIcon(
                    onPressed: !controller.busy ? onConnectBle : null,
                    icon: const Icon(Icons.bluetooth),
                    label: Text(controller.busy ? '搜索中' : 'BLE'),
                  ),
                OutlinedButton.icon(
                  onPressed: controller.connected && !controller.busy
                      ? onDisconnect
                      : null,
                  icon: const Icon(Icons.link_off),
                  label: const Text('断开'),
                ),
              ],
            ),
            if (controller.bleEnabled && showBleAddressField) ...[
              const SizedBox(height: 8),
              TextField(
                controller: controller.bleAddressController,
                decoration: const InputDecoration(
                  labelText: 'BLE MAC / Windows COM 口（可选）',
                  hintText: 'Android 留空自动连接；Windows 可填写 COM5',
                  isDense: true,
                  border: OutlineInputBorder(),
                ),
              ),
            ],
            if (controller.error != null) ...[
              const SizedBox(height: 8),
              Text(
                controller.error!,
                maxLines: 3,
                overflow: TextOverflow.ellipsis,
                style: TextStyle(color: Theme.of(context).colorScheme.error),
              ),
            ],
          ],
        ),
      ),
    );
  }
}

class BleDeviceSheet extends StatefulWidget {
  const BleDeviceSheet({super.key, required this.devices});

  final List<BluetoothSppDevice> devices;

  @override
  State<BleDeviceSheet> createState() => _BleDeviceSheetState();
}

class _BleDeviceSheetState extends State<BleDeviceSheet> {
  late BluetoothSppDevice _selected;

  @override
  void initState() {
    super.initState();
    _selected = widget.devices.first;
  }

  @override
  Widget build(BuildContext context) {
    return SafeArea(
      child: Padding(
        padding: const EdgeInsets.fromLTRB(16, 8, 16, 16),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Text('选择附近蓝牙设备', style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 12),
            DropdownButtonFormField<BluetoothSppDevice>(
              initialValue: _selected,
              isExpanded: true,
              decoration: const InputDecoration(
                border: OutlineInputBorder(),
                isDense: true,
                labelText: '附近设备',
              ),
              items: widget.devices
                  .map(
                    (device) => DropdownMenuItem(
                      value: device,
                      child: Text(
                        device.name.isEmpty
                            ? '未知蓝牙设备  ${device.address}'
                            : device.label,
                        overflow: TextOverflow.ellipsis,
                      ),
                    ),
                  )
                  .toList(),
              onChanged: (device) {
                if (device != null) setState(() => _selected = device);
              },
            ),
            const SizedBox(height: 12),
            FilledButton.icon(
              onPressed: () => Navigator.of(context).pop(_selected),
              icon: const Icon(Icons.bluetooth_connected),
              label: const Text('连接所选设备'),
            ),
          ],
        ),
      ),
    );
  }
}
