import 'package:flutter_test/flutter_test.dart';

import 'package:zncz_001/app.dart';

void main() {
  testWidgets('ZNCZ_001 app renders', (tester) async {
    await tester.pumpWidget(const Zncz001App());

    expect(find.text('ZNCZ_001 智能插座'), findsOneWidget);
    expect(find.text('连接 MQTT'), findsOneWidget);
  });
}
