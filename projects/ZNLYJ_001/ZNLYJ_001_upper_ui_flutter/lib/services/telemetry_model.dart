class ZnlyjData {
  final double temperature;
  final double humidity;
  final int rackOpen;
  final int humanPresent;
  final int clothesPresent;
  final int alarm;
  final double light;
  final double weightKg;
  final int weightOverload;
  final double tempThreshold;
  final double humidityThreshold;
  final double lightThreshold;
  final double weightThreshold;

  const ZnlyjData({
    this.temperature = 0,
    this.humidity = 0,
    this.rackOpen = 0,
    this.humanPresent = 0,
    this.clothesPresent = 0,
    this.alarm = 0,
    this.light = 0,
    this.weightKg = 0,
    this.weightOverload = 0,
    this.tempThreshold = 30,
    this.humidityThreshold = 60,
    this.lightThreshold = 50,
    this.weightThreshold = 5,
  });

  factory ZnlyjData.fromJson(Map<String, dynamic> json) {
    final data = json['data'] as Map<String, dynamic>? ?? {};
    final t = data['thresholds'] as Map<String, dynamic>? ?? {};
    return ZnlyjData(
      temperature: (data['temperature'] as num?)?.toDouble() ?? 0,
      humidity: (data['humidity'] as num?)?.toDouble() ?? 0,
      rackOpen: (data['rack_open'] as num?)?.toInt() ?? 0,
      humanPresent: (data['human_present'] as num?)?.toInt() ?? 0,
      clothesPresent: (data['clothes_present'] as num?)?.toInt() ?? 0,
      alarm: (data['alarm'] as num?)?.toInt() ?? 0,
      light: (data['light'] as num?)?.toDouble() ?? 0,
      weightKg: (data['weight_kg'] as num?)?.toDouble() ?? 0,
      weightOverload: (data['weight_overload'] as num?)?.toInt() ?? 0,
      tempThreshold: (t['temp'] as num?)?.toDouble() ?? 30,
      humidityThreshold: (t['humidity'] as num?)?.toDouble() ?? 60,
      lightThreshold: (t['light'] as num?)?.toDouble() ?? 50,
      weightThreshold: (t['weight'] as num?)?.toDouble() ?? 5,
    );
  }
}
