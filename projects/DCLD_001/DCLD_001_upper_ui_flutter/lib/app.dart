import 'package:flutter/material.dart';
import 'package:mvp_flutter_common/mvp_flutter_common.dart';

import 'features/dashboard/dashboard_page.dart';

class Dcld001App extends StatelessWidget {
  const Dcld001App({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'DCLD_001 倒车雷达控制台',
      debugShowCheckedModeBanner: false,
      theme: MvpAppTheme.light(),
      home: const DcldHomePage(),
    );
  }
}
