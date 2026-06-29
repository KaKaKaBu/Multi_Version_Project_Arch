import 'package:flutter_test/flutter_test.dart';

import 'package:wxcs_001/app.dart';

void main() {
  testWidgets('WXCS_001 app renders', (tester) async {
    await tester.pumpWidget(const Wxcs001App());

    expect(find.text('WXCS_001'), findsOneWidget);
    expect(find.text('MQTT'), findsOneWidget);
    expect(find.text('BLE'), findsOneWidget);
  });
}
