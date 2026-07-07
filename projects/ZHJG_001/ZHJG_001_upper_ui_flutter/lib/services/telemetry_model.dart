class GpsFix {
  final bool valid;
  final double latitude;
  final double longitude;

  const GpsFix({this.valid = false, this.latitude = 0, this.longitude = 0});

  factory GpsFix.fromJson(Map<String, dynamic>? json) {
    if (json == null) return const GpsFix();
    return GpsFix(
      valid: ((json['valid'] as num?)?.toInt() ?? 0) != 0,
      latitude: (json['latitude'] as num?)?.toDouble() ?? 0,
      longitude: (json['longitude'] as num?)?.toDouble() ?? 0,
    );
  }
}

class CameraFeed {
  final String deviceId;
  final String ip;
  final String streamUrl;
  final int port;

  const CameraFeed({
    this.deviceId = '',
    this.ip = '',
    this.streamUrl = '',
    this.port = 8080,
  });

  bool get available => ip.isNotEmpty || streamUrl.isNotEmpty;

  factory CameraFeed.fromJson(Map<String, dynamic> json) {
    final data = json['data'] is Map
        ? Map<String, dynamic>.from(json['data'] as Map)
        : const <String, dynamic>{};
    final camera = data['camera'] is Map
        ? Map<String, dynamic>.from(data['camera'] as Map)
        : const <String, dynamic>{};
    final params = json['params'] is Map
        ? Map<String, dynamic>.from(json['params'] as Map)
        : const <String, dynamic>{};
    final ip = _firstText([
      json['cam_ip'],
      json['camera_ip'],
      data['cam_ip'],
      data['camera_ip'],
      camera['cam_ip'],
      camera['ip'],
      params['cam_ip'],
      params['ip'],
    ]);
    final port = _toInt(
      json['mjpeg_port'] ??
          data['mjpeg_port'] ??
          camera['mjpeg_port'] ??
          params['mjpeg_port'],
      8080,
    );
    final stream = _firstText([
      json['mjpeg_url'],
      json['camera_stream'],
      data['mjpeg_url'],
      data['camera_stream'],
      camera['mjpeg_url'],
      camera['stream'],
      params['mjpeg_url'],
      params['stream'],
    ]);

    return CameraFeed(
      deviceId: _firstText([
        json['device_id'],
        data['device_id'],
        camera['device_id'],
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

class AlarmFlags {
  final bool methane;
  final bool water;
  final bool tilt;

  const AlarmFlags({
    this.methane = false,
    this.water = false,
    this.tilt = false,
  });

  factory AlarmFlags.fromJson(Map<String, dynamic>? json) {
    if (json == null) return const AlarmFlags();
    return AlarmFlags(
      methane: ((json['methane'] as num?)?.toInt() ?? 0) != 0,
      water: ((json['water'] as num?)?.toInt() ?? 0) != 0,
      tilt: ((json['tilt'] as num?)?.toInt() ?? 0) != 0,
    );
  }
}

class ManholeTelemetry {
  final int methanePpm;
  final double waterLevelPercent;
  final double tiltDegree;
  final bool alarm;
  final String mode;
  final AlarmFlags alarms;
  final int methaneThresholdPpm;
  final double waterThresholdPercent;
  final double tiltThresholdDegree;
  final GpsFix gps;
  final CameraFeed camera;

  const ManholeTelemetry({
    this.methanePpm = 0,
    this.waterLevelPercent = 0,
    this.tiltDegree = 0,
    this.alarm = false,
    this.mode = 'auto',
    this.alarms = const AlarmFlags(),
    this.methaneThresholdPpm = 1000,
    this.waterThresholdPercent = 70,
    this.tiltThresholdDegree = 15,
    this.gps = const GpsFix(),
    this.camera = const CameraFeed(),
  });

  factory ManholeTelemetry.fromJson(Map<String, dynamic> json) {
    final data = json['data'] as Map<String, dynamic>? ?? {};
    final thresholds = data['thresholds'] as Map<String, dynamic>? ?? {};

    return ManholeTelemetry(
      methanePpm: (data['methane_ppm'] as num?)?.toInt() ?? 0,
      waterLevelPercent: (data['water_level_percent'] as num?)?.toDouble() ?? 0,
      tiltDegree: (data['tilt_degree'] as num?)?.toDouble() ?? 0,
      alarm: ((data['alarm'] as num?)?.toInt() ?? 0) != 0,
      mode: data['mode'] as String? ?? 'auto',
      alarms: AlarmFlags.fromJson(data['alarms'] as Map<String, dynamic>?),
      methaneThresholdPpm: (thresholds['methane_ppm'] as num?)?.toInt() ?? 1000,
      waterThresholdPercent:
          (thresholds['water_level_percent'] as num?)?.toDouble() ?? 70,
      tiltThresholdDegree:
          (thresholds['tilt_degree'] as num?)?.toDouble() ?? 15,
      gps: GpsFix.fromJson(data['gps'] as Map<String, dynamic>?),
      camera: CameraFeed.fromJson(json),
    );
  }

  ManholeTelemetry copyWith({
    int? methanePpm,
    double? waterLevelPercent,
    double? tiltDegree,
    bool? alarm,
    String? mode,
    AlarmFlags? alarms,
    int? methaneThresholdPpm,
    double? waterThresholdPercent,
    double? tiltThresholdDegree,
    GpsFix? gps,
    CameraFeed? camera,
  }) {
    return ManholeTelemetry(
      methanePpm: methanePpm ?? this.methanePpm,
      waterLevelPercent: waterLevelPercent ?? this.waterLevelPercent,
      tiltDegree: tiltDegree ?? this.tiltDegree,
      alarm: alarm ?? this.alarm,
      mode: mode ?? this.mode,
      alarms: alarms ?? this.alarms,
      methaneThresholdPpm: methaneThresholdPpm ?? this.methaneThresholdPpm,
      waterThresholdPercent:
          waterThresholdPercent ?? this.waterThresholdPercent,
      tiltThresholdDegree: tiltThresholdDegree ?? this.tiltThresholdDegree,
      gps: gps ?? this.gps,
      camera: camera ?? this.camera,
    );
  }

  static bool hasCameraPayload(Map<String, dynamic> json) =>
      CameraFeed.fromJson(json).available;
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
