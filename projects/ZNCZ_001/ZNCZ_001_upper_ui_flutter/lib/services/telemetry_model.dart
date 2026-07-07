class SocketTelemetry {
  const SocketTelemetry({
    this.relayOn = false,
    this.mode = 'manual',
    this.time = '--:--:--',
    this.onTime = '08:00:00',
    this.offTime = '22:00:00',
    this.wifi = 'offline',
    this.lastUpdated,
  });

  final bool relayOn;
  final String mode;
  final String time;
  final String onTime;
  final String offTime;
  final String wifi;
  final DateTime? lastUpdated;

  bool get timerMode => mode.toLowerCase() == 'timer';
  bool get wifiConnected => wifi.toLowerCase() == 'connected';

  SocketTelemetry copyWith({
    bool? relayOn,
    String? mode,
    String? time,
    String? onTime,
    String? offTime,
    String? wifi,
    DateTime? lastUpdated,
  }) {
    return SocketTelemetry(
      relayOn: relayOn ?? this.relayOn,
      mode: mode ?? this.mode,
      time: time ?? this.time,
      onTime: onTime ?? this.onTime,
      offTime: offTime ?? this.offTime,
      wifi: wifi ?? this.wifi,
      lastUpdated: lastUpdated ?? this.lastUpdated,
    );
  }

  factory SocketTelemetry.fromJson(
    Map<String, dynamic> json, {
    SocketTelemetry? previous,
  }) {
    final base = previous ?? const SocketTelemetry();
    final data = _jsonMap(json['data']) ?? json;

    return SocketTelemetry(
      relayOn: _toBool(data['relay'], base.relayOn),
      mode: _firstText([data['mode'], base.mode]),
      time: _firstText([data['time'], base.time]),
      onTime: _firstText([data['on_time'], base.onTime]),
      offTime: _firstText([data['off_time'], base.offTime]),
      wifi: _firstText([data['wifi'], base.wifi]),
      lastUpdated: DateTime.now(),
    );
  }

  static bool isTelemetryPayload(Map<String, dynamic> json) {
    final data = _jsonMap(json['data']) ?? json;
    return data.containsKey('relay') ||
        data.containsKey('mode') ||
        data.containsKey('time') ||
        data.containsKey('on_time') ||
        data.containsKey('off_time') ||
        data.containsKey('wifi');
  }
}

Map<String, dynamic>? _jsonMap(Object? value) {
  if (value is Map) return Map<String, dynamic>.from(value);
  return null;
}

String _firstText(List<Object?> values) {
  for (final value in values) {
    final text = value?.toString().trim() ?? '';
    if (text.isNotEmpty) return text;
  }
  return '';
}

bool _toBool(Object? value, bool fallback) {
  if (value == null) return fallback;
  if (value is bool) return value;
  if (value is num) return value.toInt() != 0;
  final text = value.toString().trim().toLowerCase();
  if (text == 'true' || text == 'on' || text == '1') return true;
  if (text == 'false' || text == 'off' || text == '0') return false;
  return fallback;
}
