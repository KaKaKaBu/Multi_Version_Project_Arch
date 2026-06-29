import 'package:flutter_test/flutter_test.dart';

import 'package:znlyj_001/app.dart';

void main() {
  testWidgets('ZNLYJ_001 app renders', (tester) async {
    await tester.pumpWidget(const Znlyj001App());

    expect(find.text('ZNLYJ_001'), findsOneWidget);
    expect(find.text('MQTT'), findsOneWidget);
    expect(find.text('BLE'), findsOneWidget);
  });
}
