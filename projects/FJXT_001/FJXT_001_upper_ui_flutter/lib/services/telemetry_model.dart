class WindowTelemetry {
  final String state;
  final bool openLimit;
  final bool closeLimit;
  final bool pinch;
  final bool alarm;
  final bool camera;
  final bool cloud;
  final String cameraIp;
  final String cameraStream;

  const WindowTelemetry({
    this.state = 'Stopped',
    this.openLimit = false,
    this.closeLimit = false,
    this.pinch = false,
    this.alarm = false,
    this.camera = false,
    this.cloud = false,
    this.cameraIp = '',
    this.cameraStream = '',
  });

  factory WindowTelemetry.fromJson(Map<String, dynamic> json) {
    final data = json['data'] as Map<String, dynamic>? ?? {};
    return WindowTelemetry(
      state: data['state'] as String? ?? 'Stopped',
      openLimit: ((data['open_limit'] as num?)?.toInt() ?? 0) != 0,
      closeLimit: ((data['close_limit'] as num?)?.toInt() ?? 0) != 0,
      pinch: ((data['pinch'] as num?)?.toInt() ?? 0) != 0,
      alarm: ((data['alarm'] as num?)?.toInt() ?? 0) != 0,
      camera: ((data['camera'] as num?)?.toInt() ?? 0) != 0,
      cloud: ((data['cloud'] as num?)?.toInt() ?? 0) != 0,
      cameraIp: data['camera_ip'] as String? ?? '',
      cameraStream: data['camera_stream'] as String? ?? '',
    );
  }
}
