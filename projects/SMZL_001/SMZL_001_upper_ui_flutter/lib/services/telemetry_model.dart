class SmzlData {
  final int heartRate;
  final int spo2;
  final double temperature;
  final int turnoverCount;
  final int alarm;
  final int hrUpper;
  final int hrLower;
  final int spo2Upper;
  final int spo2Lower;
  final double tempUpper;
  final double tempLower;

  const SmzlData({
    this.heartRate = 0,
    this.spo2 = 0,
    this.temperature = 0,
    this.turnoverCount = 0,
    this.alarm = 0,
    this.hrUpper = 100,
    this.hrLower = 50,
    this.spo2Upper = 100,
    this.spo2Lower = 90,
    this.tempUpper = 38,
    this.tempLower = 35,
  });

  factory SmzlData.fromJson(Map<String, dynamic> json) {
    final data = json['data'] as Map<String, dynamic>? ?? {};
    final thresholds = data['thresholds'] as Map<String, dynamic>? ?? {};
    return SmzlData(
      heartRate: (data['heart_rate'] as num?)?.toInt() ?? 0,
      spo2: (data['spo2'] as num?)?.toInt() ?? 0,
      temperature: (data['temperature'] as num?)?.toDouble() ?? 0,
      turnoverCount: (data['turnover_count'] as num?)?.toInt() ?? 0,
      alarm: (data['alarm'] as num?)?.toInt() ?? 0,
      hrUpper: (thresholds['hr_upper'] as num?)?.toInt() ?? 100,
      hrLower: (thresholds['hr_lower'] as num?)?.toInt() ?? 50,
      spo2Upper: (thresholds['spo2_upper'] as num?)?.toInt() ?? 100,
      spo2Lower: (thresholds['spo2_lower'] as num?)?.toInt() ?? 90,
      tempUpper: (thresholds['temp_upper'] as num?)?.toDouble() ?? 38,
      tempLower: (thresholds['temp_lower'] as num?)?.toDouble() ?? 35,
    );
  }
}
