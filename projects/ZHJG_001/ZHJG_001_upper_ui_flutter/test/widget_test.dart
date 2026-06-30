import 'package:flutter_test/flutter_test.dart';

import 'package:zhjg_001/app.dart';

void main() {
  testWidgets('ZHJG_001 app renders', (tester) async {
    await tester.pumpWidget(const Zhjg001App());

    expect(find.text('ZHJG_001 智能井盖'), findsOneWidget);
    expect(find.text('MQTT'), findsOneWidget);
    expect(find.text('BLE'), findsOneWidget);
  });
}
