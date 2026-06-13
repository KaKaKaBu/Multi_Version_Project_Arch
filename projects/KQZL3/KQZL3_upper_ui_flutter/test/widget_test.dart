import 'package:flutter_test/flutter_test.dart';

import 'package:kqzl3/app.dart';

void main() {
  testWidgets('KQZL3 dashboard renders', (WidgetTester tester) async {
    await tester.pumpWidget(const Kqzl3App());

    expect(find.text('KQZL3 空气质量控制台'), findsOneWidget);
    expect(find.text('通信连接'), findsOneWidget);
    expect(find.text('实时数据'), findsOneWidget);
  });
}
