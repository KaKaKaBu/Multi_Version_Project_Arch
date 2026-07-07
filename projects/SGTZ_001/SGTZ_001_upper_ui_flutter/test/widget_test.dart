import 'package:flutter_test/flutter_test.dart';

import 'package:sgtz_001/app.dart';

void main() {
  testWidgets('SGTZ_001 app renders', (tester) async {
    await tester.pumpWidget(const Sgtz001App());

    expect(find.text('SGTZ_001 电子秤'), findsOneWidget);
    expect(find.text('MQTT'), findsOneWidget);
    expect(find.text('BLE'), findsOneWidget);
  });
}
