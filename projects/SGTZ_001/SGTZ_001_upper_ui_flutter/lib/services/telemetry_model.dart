class CameraStreamInfo {
  const CameraStreamInfo({
    this.deviceId = '',
    this.ip = '',
    this.streamUrl = '',
    this.port = 8080,
  });

  final String deviceId;
  final String ip;
  final String streamUrl;
  final int port;

  bool get available => ip.isNotEmpty || streamUrl.isNotEmpty;

  factory CameraStreamInfo.fromJson(Map<String, dynamic> json) {
    final data = _jsonMap(json['data']) ?? json;
    final params = _jsonMap(json['params']) ?? const <String, dynamic>{};
    final ip = _firstText([
      json['cam_ip'],
      json['camera_ip'],
      data['cam_ip'],
      data['camera_ip'],
      params['ip'],
      params['cam_ip'],
    ]);
    final port = _toInt(
      json['mjpeg_port'] ?? data['mjpeg_port'] ?? params['mjpeg_port'],
      8080,
    );
    final stream = _firstText([
      json['mjpeg_url'],
      json['camera_stream'],
      data['mjpeg_url'],
      data['camera_stream'],
      params['stream'],
      params['mjpeg_url'],
    ]);

    return CameraStreamInfo(
      deviceId: _firstText([
        json['device_id'],
        data['device_id'],
        params['device_id'],
      ]),
      ip: ip,
      port: port,
      streamUrl: stream.isNotEmpty || ip.isEmpty
          ? stream
          : 'http://$ip:$port/stream',
    );
  }
}

class ScaleTelemetry {
  const ScaleTelemetry({
    this.device = 'SGTZ_001',
    this.weightG = 0,
    this.heightCm = 0,
    this.bmiX10 = 0,
    this.state = 'WAIT',
    this.locked = false,
    this.fanOn = false,
    this.manualMode = false,
    this.alarmOn = false,
    this.thresholdG = 500,
    this.voicePaused = false,
    this.camera = const CameraStreamInfo(),
  });

  final String device;
  final int weightG;
  final int heightCm;
  final int bmiX10;
  final String state;
  final bool locked;
  final bool fanOn;
  final bool manualMode;
  final bool alarmOn;
  final int thresholdG;
  final bool voicePaused;
  final CameraStreamInfo camera;

  double get weightKg => weightG / 1000.0;
  double get heightM => heightCm / 100.0;
  double get bmi => bmiX10 / 10.0;

  String get bmiLabel {
    final normalized = state.toUpperCase();
    if (normalized == 'LIGHT') return '过轻';
    if (normalized == 'NORMAL') return '正常';
    if (normalized == 'HEAVY') return '过重';
    if (bmiX10 <= 0) return '等待数据';
    if (bmiX10 < 185) return '过轻';
    if (bmiX10 < 239) return '正常';
    return '过重';
  }

  factory ScaleTelemetry.fromJson(
    Map<String, dynamic> json, {
    ScaleTelemetry? previous,
  }) {
    final base = previous ?? const ScaleTelemetry();
    final data = _jsonMap(json['data']) ?? json;
    final cameraInfo = CameraStreamInfo.fromJson(json);

    return ScaleTelemetry(
      device: _firstText([data['device'], data['device_id'], base.device]),
      weightG: _toInt(data['weight_g'] ?? data['pressure_g'], base.weightG),
      heightCm: _toInt(data['height_cm'], base.heightCm),
      bmiX10: _toInt(data['bmi_x10'], base.bmiX10),
      state: _firstText([data['state'], base.state]),
      locked: _toBool(data['locked'], base.locked),
      fanOn: _toBool(data['fan_on'] ?? data['fan'], base.fanOn),
      manualMode: _toBool(
        data['manual_mode'] ?? data['manual'],
        base.manualMode,
      ),
      alarmOn: _toBool(data['alarm_on'] ?? data['alarm'], base.alarmOn),
      thresholdG: _toInt(
        data['threshold_g'] ?? data['threshold'],
        base.thresholdG,
      ),
      voicePaused: _toBool(data['voice_paused'], base.voicePaused),
      camera: cameraInfo.available ? cameraInfo : base.camera,
    );
  }

  static bool isTelemetryPayload(Map<String, dynamic> json) {
    if (json['type'] == 'telemetry' || json['data'] is Map) return true;
    return json.containsKey('weight_g') ||
        json.containsKey('height_cm') ||
        json.containsKey('bmi_x10') ||
        CameraStreamInfo.fromJson(json).available;
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
