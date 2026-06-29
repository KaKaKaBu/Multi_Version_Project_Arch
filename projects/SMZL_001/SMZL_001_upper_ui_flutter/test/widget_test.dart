import 'package:flutter_test/flutter_test.dart';

import 'package:smzl_001/app.dart';

void main() {
  testWidgets('SMZL_001 app renders', (tester) async {
    await tester.pumpWidget(const Smzl001App());

    expect(find.text('SMZL_001'), findsOneWidget);
    expect(find.text('MQTT'), findsOneWidget);
    expect(find.text('BLE'), findsOneWidget);
  });
}
