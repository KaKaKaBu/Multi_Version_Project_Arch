class CameraStreamInfo {
  final String deviceId;
  final String ip;
  final String streamUrl;
  final int port;

  const CameraStreamInfo({
    this.deviceId = '',
    this.ip = '',
    this.streamUrl = '',
    this.port = 8080,
  });

  bool get available => ip.isNotEmpty || streamUrl.isNotEmpty;

  factory CameraStreamInfo.fromJson(Map<String, dynamic> json) {
    final data = _extractTelemetryData(json);
    final params = json['params'] is Map
        ? Map<String, dynamic>.from(json['params'] as Map)
        : const <String, dynamic>{};
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

class WindowTelemetry {
  final String state;
  final bool pinch;
  final bool alarm;
  final bool camera;
  final bool cloud;
  final String cameraDeviceId;
  final String cameraIp;
  final String cameraStream;

  const WindowTelemetry({
    this.state = 'Stopped',
    this.pinch = false,
    this.alarm = false,
    this.camera = false,
    this.cloud = false,
    this.cameraDeviceId = '',
    this.cameraIp = '',
    this.cameraStream = '',
  });

  factory WindowTelemetry.fromJson(
    Map<String, dynamic> json, {
    WindowTelemetry? previous,
  }) {
    final data = _extractTelemetryData(json);
    final hasTelemetry = json['type'] == 'telemetry' || data.isNotEmpty;
    final base = previous ?? const WindowTelemetry();
    final cameraInfo = CameraStreamInfo.fromJson(json);
    return WindowTelemetry(
      state: hasTelemetry ? _toText(data['state'], base.state) : base.state,
      pinch: hasTelemetry
          ? _toBool(data['pinch'], base.pinch)
          : base.pinch,
      alarm: hasTelemetry
          ? _toBool(data['alarm'], base.alarm)
          : base.alarm,
      camera:
          (hasTelemetry
              ? _toBool(data['camera'], base.camera)
              : base.camera) ||
          cameraInfo.available,
      cloud: hasTelemetry
          ? _toBool(data['cloud'], base.cloud)
          : base.cloud,
      cameraDeviceId: cameraInfo.deviceId.isNotEmpty
          ? cameraInfo.deviceId
          : base.cameraDeviceId,
      cameraIp: cameraInfo.ip.isNotEmpty
          ? cameraInfo.ip
          : (hasTelemetry
                ? data['camera_ip'] as String? ?? base.cameraIp
                : base.cameraIp),
      cameraStream: cameraInfo.streamUrl.isNotEmpty
          ? cameraInfo.streamUrl
          : (hasTelemetry
                ? data['camera_stream'] as String? ?? base.cameraStream
                : base.cameraStream),
    );
  }

  static bool hasCameraPayload(Map<String, dynamic> json) =>
      CameraStreamInfo.fromJson(json).available;
}

Map<String, dynamic> _extractTelemetryData(Map<String, dynamic> json) {
  final data = <String, dynamic>{};

  void collect(Object? value) {
    if (value == null || value is String) return;

    if (value is List) {
      for (final item in value) {
        collect(item);
      }
      return;
    }

    if (value is! Map) return;
    final map = Map<String, dynamic>.from(value);

    for (final key in const ['data', 'message', 'params', 'properties']) {
      final child = map[key];
      if (child is Map) data.addAll(Map<String, dynamic>.from(child));
    }

    final services = map['services'];
    if (services is List) {
      for (final service in services) {
        collect(service);
      }
    }

    for (final entry in map.entries) {
      if (entry.key == 'data' ||
          entry.key == 'message' ||
          entry.key == 'params' ||
          entry.key == 'properties' ||
          entry.key == 'services') {
        continue;
      }
      collect(entry.value);
    }
  }

  collect(json);
  if (data.isEmpty) data.addAll(json);
  return data;
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

String _toText(Object? value, String fallback) {
  final text = value?.toString().trim() ?? '';
  return text.isEmpty ? fallback : text;
}

bool _toBool(Object? value, bool fallback) {
  if (value == null) return fallback;
  if (value is bool) return value;
  if (value is num) return value.toInt() != 0;
  final text = value.toString().trim().toLowerCase();
  if (text == 'true' || text == 'on' || text == 'yes' || text == '1') {
    return true;
  }
  if (text == 'false' || text == 'off' || text == 'no' || text == '0') {
    return false;
  }
  return fallback;
}
