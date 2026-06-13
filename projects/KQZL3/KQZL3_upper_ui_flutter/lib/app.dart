import 'package:flutter/material.dart';

import 'features/dashboard/dashboard_page.dart';

class Kqzl3App extends StatelessWidget {
  const Kqzl3App({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'KQZL3 空气质量控制台',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: const Color(0xff0f8f70)),
        scaffoldBackgroundColor: const Color(0xffeef4f7),
        useMaterial3: true,
        fontFamilyFallback: const ['Noto Sans CJK SC', 'Microsoft YaHei', 'sans'],
      ),
      home: const DashboardPage(),
    );
  }
}
