class MedicineTimer {
  const MedicineTimer({
    required this.index,
    this.enabled = false,
    this.hour = 8,
    this.minute = 0,
    this.dose = 1,
    this.category = 0,
  });

  final int index;
  final bool enabled;
  final int hour;
  final int minute;
  final int dose;
  final int category;

  String get timeText =>
      '${hour.toString().padLeft(2, '0')}:${minute.toString().padLeft(2, '0')}';
}

class YyyhTelemetry {
  const YyyhTelemetry({
    this.version = 'YYYH',
    this.versionNo = 24,
    this.alarm = false,
    this.boxOpen = false,
    this.taken = false,
    this.lowMedicine = false,
    this.timerIndex = 1,
    this.timerEnabled = false,
    this.timerHour = 8,
    this.timerMinute = 0,
    this.timerDose = 1,
    this.timerCategory = 0,
    this.medicine1 = 10,
    this.medicine2 = 10,
    this.medicine3 = 10,
    this.weightG,
    this.temperature,
    this.humidity,
    this.mjpegUrl = '',
  });

  final String version;
  final int versionNo;
  final bool alarm;
  final bool boxOpen;
  final bool taken;
  final bool lowMedicine;
  final int timerIndex;
  final bool timerEnabled;
  final int timerHour;
  final int timerMinute;
  final int timerDose;
  final int timerCategory;
  final int medicine1;
  final int medicine2;
  final int medicine3;
  final double? weightG;
  final double? temperature;
  final double? humidity;
  final String mjpegUrl;

  MedicineTimer get selectedTimer => MedicineTimer(
        index: timerIndex,
        enabled: timerEnabled,
        hour: timerHour,
        minute: timerMinute,
        dose: timerDose,
        category: timerCategory,
      );

  int medicineCount(int index) {
    return switch (index) {
      1 => medicine1,
      2 => medicine2,
      3 => medicine3,
      _ => 0,
    };
  }

  factory YyyhTelemetry.fromJson(
    Map<String, dynamic> json, {
    YyyhTelemetry? previous,
  }) {
    final base = previous ?? const YyyhTelemetry();
    final data = _jsonMap(json['data']) ?? _jsonMap(json['message']) ?? json;

    return YyyhTelemetry(
      version: _firstText([data['version'], base.version]),
      versionNo: _toInt(data['version_no'], base.versionNo),
      alarm: _toBool(data['alarm'], base.alarm),
      boxOpen: _toBool(data['box_open'], base.boxOpen),
      taken: _toBool(data['taken'], base.taken),
      lowMedicine: _toBool(data['low_medicine'], base.lowMedicine),
      timerIndex: _toInt(data['timer_index'], base.timerIndex).clamp(1, 3),
      timerEnabled: _toBool(data['timer_enabled'], base.timerEnabled),
      timerHour: _toInt(data['timer_hour'], base.timerHour).clamp(0, 23),
      timerMinute: _toInt(data['timer_minute'], base.timerMinute).clamp(0, 59),
      timerDose: _toInt(data['timer_dose'], base.timerDose).clamp(1, 99),
      timerCategory:
          _toInt(data['timer_category'], base.timerCategory).clamp(0, 2),
      medicine1: _toInt(data['medicine1'], base.medicine1),
      medicine2: _toInt(data['medicine2'], base.medicine2),
      medicine3: _toInt(data['medicine3'], base.medicine3),
      weightG: data.containsKey('weight_g')
          ? _toDouble(data['weight_g'], base.weightG ?? 0)
          : base.weightG,
      temperature: data.containsKey('temperature')
          ? _toDouble(data['temperature'], base.temperature ?? 0)
          : base.temperature,
      humidity: data.containsKey('humidity')
          ? _toDouble(data['humidity'], base.humidity ?? 0)
          : base.humidity,
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
        data.containsKey('version_no') ||
        data.containsKey('alarm') ||
        data.containsKey('box_open') ||
        data.containsKey('taken') ||
        data.containsKey('timer_index') ||
        data.containsKey('medicine1') ||
        data.containsKey('weight_g') ||
        data.containsKey('temperature') ||
        data.containsKey('mjpeg_url');
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

double _toDouble(Object? value, double fallback) {
  if (value is num) return value.toDouble();
  return double.tryParse(value?.toString() ?? '') ?? fallback;
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
