class AqmjTelemetry {
  const AqmjTelemetry({
    this.version = 15,
    this.home = true,
    this.armed = false,
    this.presence = false,
    this.alarm = false,
    this.lamp = false,
    this.light = 0,
    this.message = false,
    this.video = false,
    this.lastCallHour = 0,
    this.lastCallMinute = 0,
    this.mjpegUrl = '',
  });

  final int version;
  final bool home;
  final bool armed;
  final bool presence;
  final bool alarm;
  final bool lamp;
  final int light;
  final bool message;
  final bool video;
  final int lastCallHour;
  final int lastCallMinute;
  final String mjpegUrl;

  String get lastCallText =>
      '${lastCallHour.toString().padLeft(2, '0')}:${lastCallMinute.toString().padLeft(2, '0')}';

  factory AqmjTelemetry.fromJson(
    Map<String, dynamic> json, {
    AqmjTelemetry? previous,
  }) {
    final base = previous ?? const AqmjTelemetry();
    final data = _jsonMap(json['data']) ?? _jsonMap(json['message']) ?? json;

    return AqmjTelemetry(
      version: _toInt(data['version'], base.version),
      home: _toBool(data['home'], base.home),
      armed: _toBool(data['armed'], base.armed),
      presence: _toBool(data['presence'], base.presence),
      alarm: _toBool(data['alarm'], base.alarm),
      lamp: _toBool(data['lamp'], base.lamp),
      light: _toInt(data['light'], base.light),
      message: _toBool(data['message'], base.message),
      video: _toBool(data['video'], base.video),
      lastCallHour:
          _toInt(data['last_call_hour'], base.lastCallHour).clamp(0, 23),
      lastCallMinute:
          _toInt(data['last_call_minute'], base.lastCallMinute).clamp(0, 59),
      mjpegUrl: _firstText([
        data['mjpeg_url'],
        data['mjpegUrl'],
        data['stream_url'],
        data['url'],
        base.mjpegUrl,
      ]),
    );
  }

  static bool isTelemetryPayload(Map<String, dynamic> json) {
    final data = _jsonMap(json['data']) ?? _jsonMap(json['message']) ?? json;
    return json['type'] == 'telemetry' ||
        data.containsKey('version') ||
        data.containsKey('home') ||
        data.containsKey('armed') ||
        data.containsKey('presence') ||
        data.containsKey('alarm') ||
        data.containsKey('lamp') ||
        data.containsKey('light') ||
        data.containsKey('message') ||
        data.containsKey('last_call_hour');
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

int _toInt(Object? value, int fallback) {
  if (value is num) return value.toInt();
  return int.tryParse(value?.toString() ?? '') ?? fallback;
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
