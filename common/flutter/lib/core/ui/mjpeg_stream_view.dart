import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter/material.dart';

class MjpegStreamView extends StatefulWidget {
  const MjpegStreamView({super.key, required this.url, this.height = 220});

  final String url;
  final double height;

  @override
  State<MjpegStreamView> createState() => _MjpegStreamViewState();
}

class _MjpegStreamViewState extends State<MjpegStreamView> {
  StreamSubscription<Uint8List>? _subscription;
  Uint8List? _frame;
  Object? _error;

  @override
  void initState() {
    super.initState();
    _connect();
  }

  @override
  void didUpdateWidget(MjpegStreamView oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.url != widget.url) {
      _connect();
    }
  }

  @override
  void dispose() {
    _subscription?.cancel();
    super.dispose();
  }

  void _connect() {
    _subscription?.cancel();
    final streamUrl = widget.url.trim();
    setState(() {
      _frame = null;
      _error = null;
    });
    if (streamUrl.isEmpty) return;

    _subscription = _mjpegFrames(streamUrl).listen(
      (frame) {
        if (!mounted) return;
        setState(() {
          _frame = frame;
          _error = null;
        });
      },
      onError: (error) {
        if (!mounted) return;
        setState(() => _error = error);
      },
      cancelOnError: false,
    );
  }

  @override
  Widget build(BuildContext context) {
    final streamUrl = widget.url.trim();
    if (streamUrl.isEmpty) {
      return _StreamPlaceholder(
        height: widget.height,
        icon: Icons.videocam_off,
        text: '等待摄像头通过 MQTT 上报 MJPEG 地址',
      );
    }

    if (_error != null) {
      return _StreamPlaceholder(
        height: widget.height,
        icon: Icons.error_outline,
        text: 'MJPEG 连接失败：$_error',
        url: streamUrl,
      );
    }

    return ClipRRect(
      borderRadius: BorderRadius.circular(12),
      child: Container(
        width: double.infinity,
        height: widget.height,
        color: Colors.black,
        alignment: Alignment.center,
        child: _frame == null
            ? const CircularProgressIndicator()
            : Image.memory(
                _frame!,
                gaplessPlayback: true,
                fit: BoxFit.contain,
                width: double.infinity,
                height: double.infinity,
              ),
      ),
    );
  }
}

class _StreamPlaceholder extends StatelessWidget {
  const _StreamPlaceholder({
    required this.height,
    required this.icon,
    required this.text,
    this.url,
  });

  final double height;
  final IconData icon;
  final String text;
  final String? url;

  @override
  Widget build(BuildContext context) {
    return Container(
      width: double.infinity,
      height: height,
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Theme.of(
          context,
        ).colorScheme.surfaceContainerHighest.withAlpha(120),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: Theme.of(context).dividerColor),
      ),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(icon, size: 36, color: Theme.of(context).colorScheme.primary),
          const SizedBox(height: 10),
          Text(text, textAlign: TextAlign.center),
          if (url != null) ...[
            const SizedBox(height: 8),
            SelectableText(
              url!,
              textAlign: TextAlign.center,
              style: const TextStyle(fontFamily: 'monospace', fontSize: 12),
            ),
          ],
        ],
      ),
    );
  }
}

Stream<Uint8List> _mjpegFrames(String url) async* {
  final uri = Uri.parse(url);
  final client = HttpClient()..connectionTimeout = const Duration(seconds: 8);
  final buffer = <int>[];

  try {
    final request = await client.getUrl(uri);
    request.headers.set(
      HttpHeaders.acceptHeader,
      'multipart/x-mixed-replace,image/jpeg,*/*',
    );
    final response = await request.close();
    if (response.statusCode < 200 || response.statusCode >= 300) {
      throw HttpException('HTTP ${response.statusCode}', uri: uri);
    }

    await for (final chunk in response) {
      buffer.addAll(chunk);
      while (true) {
        final start = _indexOf(buffer, const [0xff, 0xd8], 0);
        if (start < 0) {
          if (buffer.length > 4096) {
            buffer.removeRange(0, buffer.length - 2);
          }
          break;
        }
        if (start > 0) {
          buffer.removeRange(0, start);
        }
        final end = _indexOf(buffer, const [0xff, 0xd9], 2);
        if (end < 0) {
          if (buffer.length > 1024 * 1024) {
            buffer.removeRange(0, buffer.length - 2);
          }
          break;
        }
        final frame = Uint8List.fromList(buffer.sublist(0, end + 2));
        buffer.removeRange(0, end + 2);
        yield frame;
      }
    }
  } finally {
    client.close(force: true);
  }
}

int _indexOf(List<int> source, List<int> pattern, int start) {
  if (pattern.isEmpty || source.length < pattern.length) return -1;
  for (var i = start; i <= source.length - pattern.length; i++) {
    var matched = true;
    for (var j = 0; j < pattern.length; j++) {
      if (source[i + j] != pattern[j]) {
        matched = false;
        break;
      }
    }
    if (matched) return i;
  }
  return -1;
}
