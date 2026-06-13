import 'package:flutter_test/flutter_test.dart';

import 'package:dcld_001/app.dart';

void main() {
  testWidgets('DCLD_001 dashboard renders', (WidgetTester tester) async {
    await tester.pumpWidget(const Dcld001App());

    expect(find.text('DCLD_001 倒车雷达控制台'), findsOneWidget);
    expect(find.text('通信连接'), findsOneWidget);
    expect(find.text('实时数据'), findsOneWidget);
  });
}
