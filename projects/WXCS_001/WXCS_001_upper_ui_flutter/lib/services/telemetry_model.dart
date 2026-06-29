class TelemetryData {
  final double temperature;
  final int airQualityPpm;
  final int coPpm;
  final int alarm;
  final int fan;
  final String mode;
  final int tempThreshold;
  final int aqThreshold;
  final int coThreshold;

  const TelemetryData({
    this.temperature = 0,
    this.airQualityPpm = 0,
    this.coPpm = 0,
    this.alarm = 0,
    this.fan = 0,
    this.mode = 'AUTO',
    this.tempThreshold = 35,
    this.aqThreshold = 500,
    this.coThreshold = 100,
  });

  factory TelemetryData.fromJson(Map<String, dynamic> json) {
    final data = json['data'] as Map<String, dynamic>? ?? {};
    final thresholds = data['thresholds'] as Map<String, dynamic>? ?? {};

    return TelemetryData(
      temperature: (data['temperature'] as num?)?.toDouble() ?? 0,
      airQualityPpm: (data['air_quality_ppm'] as num?)?.toInt() ?? 0,
      coPpm: (data['co_ppm'] as num?)?.toInt() ?? 0,
      alarm: (data['alarm'] as num?)?.toInt() ?? 0,
      fan: (data['fan'] as num?)?.toInt() ?? 0,
      mode: data['mode'] as String? ?? 'AUTO',
      tempThreshold: (thresholds['temp'] as num?)?.toInt() ?? 35,
      aqThreshold: (thresholds['air_quality_ppm'] as num?)?.toInt() ?? 500,
      coThreshold: (thresholds['co_ppm'] as num?)?.toInt() ?? 100,
    );
  }

  Map<String, dynamic> toJson() => {
    'type': 'telemetry',
    'data': {
      'temperature': temperature,
      'air_quality_ppm': airQualityPpm,
      'co_ppm': coPpm,
      'alarm': alarm,
      'fan': fan,
      'mode': mode,
      'thresholds': {
        'temp': tempThreshold,
        'air_quality_ppm': aqThreshold,
        'co_ppm': coThreshold,
      },
    },
  };
}
