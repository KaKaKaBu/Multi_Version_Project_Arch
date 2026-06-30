class GpsFix {
  final bool valid;
  final double latitude;
  final double longitude;

  const GpsFix({
    this.valid = false,
    this.latitude = 0,
    this.longitude = 0,
  });

  factory GpsFix.fromJson(Map<String, dynamic>? json) {
    if (json == null) return const GpsFix();
    return GpsFix(
      valid: ((json['valid'] as num?)?.toInt() ?? 0) != 0,
      latitude: (json['latitude'] as num?)?.toDouble() ?? 0,
      longitude: (json['longitude'] as num?)?.toDouble() ?? 0,
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
  });

  factory ManholeTelemetry.fromJson(Map<String, dynamic> json) {
    final data = json['data'] as Map<String, dynamic>? ?? {};
    final thresholds = data['thresholds'] as Map<String, dynamic>? ?? {};

    return ManholeTelemetry(
      methanePpm: (data['methane_ppm'] as num?)?.toInt() ?? 0,
      waterLevelPercent:
          (data['water_level_percent'] as num?)?.toDouble() ?? 0,
      tiltDegree: (data['tilt_degree'] as num?)?.toDouble() ?? 0,
      alarm: ((data['alarm'] as num?)?.toInt() ?? 0) != 0,
      mode: data['mode'] as String? ?? 'auto',
      alarms: AlarmFlags.fromJson(data['alarms'] as Map<String, dynamic>?),
      methaneThresholdPpm:
          (thresholds['methane_ppm'] as num?)?.toInt() ?? 1000,
      waterThresholdPercent:
          (thresholds['water_level_percent'] as num?)?.toDouble() ?? 70,
      tiltThresholdDegree:
          (thresholds['tilt_degree'] as num?)?.toDouble() ?? 15,
      gps: GpsFix.fromJson(data['gps'] as Map<String, dynamic>?),
    );
  }
}
