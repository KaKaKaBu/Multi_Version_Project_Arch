import 'package:flutter_test/flutter_test.dart';

import 'package:fjxt_001/app.dart';

void main() {
  testWidgets('FJXT_001 app renders', (tester) async {
    await tester.pumpWidget(const Fjxt001App());

    expect(find.text('FJXT_001 车窗防夹'), findsOneWidget);
    expect(find.text('MQTT'), findsOneWidget);
    expect(find.text('BLE'), findsOneWidget);
  });
}
