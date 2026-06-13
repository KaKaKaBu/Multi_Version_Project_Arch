# 上位机代码编写规范

本文档面向 `projects/<NAME>/<NAME>_upper_ui_flutter/` Flutter 主上位机，以及仅在项目需要微信小程序时生成的 `projects/<NAME>/<NAME>_upper_ui/` Vue3 + uni-app + Capacitor 小程序栈。嵌入式侧约定见 [maintenance_guide.md](./maintenance_guide.md)，导出约定见其 §12。

`tools/gen_project.py` 默认生成 Flutter 上位机；只有传入 `--with-mini-program` 或产品需求明确包含微信小程序时，才额外生成 uni-app 栈。KQZL3 采用双栈：Flutter 覆盖 v3/v4/v6/v7/v8/v10/v12/v13/v14 的 App/Web 上位机，uni-app 保留用于 v9 微信小程序。

---

## 1. 目录结构约定

```
<NAME>_upper_ui_flutter/
├── lib/
│   ├── core/
│   │   ├── config/              # debug_flags.dart、MQTT/BLE 等产品配置
│   │   └── version/             # UPPER_VERSION / UPPER_FEATURES 解析
│   ├── features/                # 页面与业务 feature
│   ├── services/                # MQTT / BLE 等真实通信实现
│   ├── widgets/                 # 可复用组件
│   ├── app.dart
│   └── main.dart
├── android/                     # Flutter Android 工程
├── web/                         # Flutter Web 工程
├── test/
├── analysis_options.yaml
├── version_features.json        # Flutter 主上位机版本→特性映射，导出工具读取
└── pubspec.yaml
```

仅当项目需要微信小程序时，额外生成 uni-app 栈：

```
<NAME>_upper_ui/
├── src/
│   ├── features/
│   │   ├── index.js             # 特性注册入口，统一 setupFeatures(app)
│   │   ├── common/              # 所有版本均包含的基础功能
│   │   ├── wifi/                # WiFi 相关页面与逻辑（版本可选）
│   │   └── ble/                 # BLE 相关（版本可选）
│   ├── pages/                   # uni-app 页面（属 common feature）
│   ├── static/                  # 图片、字体等静态资源
│   ├── App.vue
│   ├── main.js
│   ├── manifest.json
│   └── pages.json
├── android/                     # Capacitor Android 工程（仅兼容旧 H5/App 栈）
├── vite.config.js
├── capacitor.config.json
├── version_features.json        # 版本→特性映射，导出工具读取
└── package.json
```

**规则：**
- Flutter 是默认上位机栈，新项目不要再用 uni-app 承担 App/Web 主界面。
- Flutter 与 uni-app 使用各自目录下的 `version_features.json`；双栈项目不要用 uni-app 小程序矩阵作为 Flutter 主上位机的唯一版本来源。
- `lib/features/<feature>/` 内只放该 feature 的页面和业务状态；跨 feature 复用放 `lib/widgets/`、`lib/services/` 或 `lib/core/`。
- uni-app 的 `src/features/<feature>/` 仅用于小程序版本；新增小程序页面必须更新 `src/pages.json`。
- `android/` 下原生代码只允许放平台通道或插件桥接，不写 UI/业务规则。

---

## 2. 编译期常量

### Flutter（`--dart-define` 注入）

| 常量 | 类型 | 说明 |
| --- | --- | --- |
| `UPPER_VERSION` | `String` | 固件版本号字符串，如 `"3"`；通过 `String.fromEnvironment('UPPER_VERSION')` 读取 |
| `UPPER_FEATURES` | `String` | 当前版本启用的 feature，逗号分隔，如 `"common,wifi"`；解析后再暴露给 UI |
| `upperUiShowCommunicationLog` | `bool` | `lib/core/config/debug_flags.dart` 中定义，等于 `kDebugMode`；Debug 显示通信日志，Release/Profile 隐藏 |

**使用方式：**

```dart
const upperVersionDefine = String.fromEnvironment('UPPER_VERSION', defaultValue: '1');
const upperFeaturesDefine = String.fromEnvironment('UPPER_FEATURES', defaultValue: 'common');
const bool upperUiShowCommunicationLog = kDebugMode;
```

### uni-app（`vite.config.js` 注入）

| 常量 | 类型 | 说明 |
| --- | --- | --- |
| `__UPPER_VERSION__` | `string` | 固件版本号字符串，如 `"3"` |
| `__UPPER_FEATURES__` | `string[]` | 当前版本启用的 feature 名称数组，如 `["common","wifi"]` |
| `__UPPER_FEATURE_FLAGS__` | `Record<string, boolean>` | feature 名 → `true` 的映射，用于条件渲染 |
| `__UPPER_UI_DEBUG__` | `boolean` | `process.env.NODE_ENV !== 'production'` 的编译期结果；dev 显示通信日志，生产构建隐藏 |

**使用方式：**

```javascript
// 条件渲染判断
if (__UPPER_FEATURE_FLAGS__.wifi) { /* ... */ }

// 模板中
// <view v-if="featureFlags.wifi">...</view>
// data: { featureFlags: __UPPER_FEATURE_FLAGS__ }
```

**禁止：**
- 不得在运行时通过接口或 `localStorage` 动态切换 feature——版本差异必须在编译期确定。
- 不得在 `vite.config.js` 以外的地方读取 `process.env.UPPER_*`。

---

## 3. version_features.json 编写规则

Flutter 主上位机与 uni-app 小程序/历史栈都可以拥有自己的 `version_features.json`。Flutter 侧至少需要 `defaultFeatures` 与 `versions`，用于 `export_project.py` 解析 `UPPER_VERSION` / `UPPER_FEATURES` 并决定导出哪些主上位机版本；uni-app 侧还会使用 `alwaysInclude` 与 `featureGlobs` 裁剪小程序源码。双栈项目必须分别维护两套矩阵，避免把 `mpWeixin` 专用版本误导出为 Flutter 主上位机。

```json
{
  "defaultFeatures": ["common"],
  "alwaysInclude": [
    "package.json", "vite.config.js", "capacitor.config.json",
    "version_features.json", "src/main.js", "src/App.vue",
    "src/manifest.json", "src/pages.json", "src/static/**"
  ],
  "featureGlobs": {
    "common": ["src/features/common/**", "src/features/index.js", "src/pages/**"],
    "wifi":   ["src/features/wifi/**"],
    "ble":    ["src/features/ble/**"]
  },
  "versions": {
    "default": { "features": ["common"] },
    "3":       { "features": ["common", "wifi"] }
  }
}
```

| 字段 | 说明 |
| --- | --- |
| `defaultFeatures` | 未在 `versions` 中匹配到的版本使用此列表 |
| `alwaysInclude` | 无论版本，导出必须包含的文件（支持 glob） |
| `featureGlobs` | 每个 feature 对应的源码路径 glob；`!` 前缀表示排除 |
| `versions.<N>` | 覆盖版本 N 的 feature 列表；键与固件版本号对应 |

**规则：**
- 新增 feature 必须同步在对应栈的 `featureGlobs` / 版本解析层中注册，否则该 feature 的源码或能力不会被导出。
- glob 路径相对于上位机根目录，必须能匹配到真实文件；路径写错会被导出工具静默忽略。
- 禁止在 `alwaysInclude` 中放特定 feature 的专有文件——应放进对应 `featureGlobs`。
- 双栈项目的 Flutter 矩阵只列主上位机覆盖版本；微信小程序专用版本只应出现在 uni-app 矩阵中并包含 `mpWeixin` feature。

---

## 4. src/features/index.js 约定

```javascript
import { setupCommonFeature } from "./common";
import { setupWifiFeature }   from "./wifi";
import { setupBleFeature }    from "./ble";

const registry = {
    common: setupCommonFeature,
    wifi:   setupWifiFeature,
    ble:    setupBleFeature,
};

export function setupFeatures(app) {
    const flags = (typeof __UPPER_FEATURE_FLAGS__ !== "undefined")
        ? __UPPER_FEATURE_FLAGS__ : {};
    Object.entries(registry).forEach(([key, installer]) => {
        if (flags[key]) installer(app);
    });
}
```

**规则：**
- 新增 feature 需在 `registry` 中添加条目，安装函数命名为 `setup<Feature>Feature`。
- `setupFeatures` 在 `src/main.js` 的 `createApp(App).use(...)` 之后调用一次。
- 安装函数只做路由注册、全局组件挂载、插件初始化等**一次性**操作，不做数据请求。

---

## 5. Flutter 编写约束

| 约束 | 说明 |
| --- | --- |
| 版本输入 | 只通过 `String.fromEnvironment('UPPER_VERSION')` / `String.fromEnvironment('UPPER_FEATURES')` 读取编译期版本与特性 |
| 特性判断 | UI 和 service 层只判断解析后的 capabilities / feature flags，不在组件里硬编码版本号 |
| 平台差异 | Web、Android、Windows 等平台差异用条件导入或平台通道隔离，不在页面组件里散落 `kIsWeb` 分支 |
| 通信抽象 | MQTT、BLE 等真实通信实现必须实现统一 transport/service 接口，页面只依赖抽象 |
| 通信日志 | 通信日志卡片只能由 `upperUiShowCommunicationLog` / `__UPPER_UI_DEBUG__` 控制显示，Release/production 不显示也不采集原始 TX/RX 日志 |
| 自适应布局 | 页面必须支持窄屏和高字体缩放，卡片头、按钮、表单、长 URL/JSON/topic 必须可换行，不允许固定宽度造成像素溢出 |
| 原生代码 | Android/iOS 原生代码只做 MethodChannel/EventChannel 或插件桥接，不写业务状态机和 UI 规则 |
| 资源路径 | 不使用绝对路径；图片、字体等走 `pubspec.yaml` assets 或 Flutter/Web 标准资源目录 |
| 验证 | 改 Flutter UI 后至少运行 `flutter analyze` 和 `flutter test`；涉及 App/Web 交互时再执行对应 `flutter build` |

**禁止：**
- 禁止在 Widget 中直接写 `APP_VERSION == N` / `UPPER_VERSION == N` 这类版本分支。
- 禁止把 MQTT Topic、账号、蓝牙服务名等产品参数散落在页面中，应集中在 `lib/core/config/` 或产品配置文件。
- 禁止 feature 之间直接 import 私有实现；共享模型放 `lib/core/`，共享组件放 `lib/widgets/`。
- 禁止实现 mock transport、模拟上报、本地遥测仿真或缺少 Broker/remote 时自动回退模拟数据。
- 禁止 Release/production 构建保留通信日志卡片或采集原始 TX/RX JSON 日志。
- 禁止固定宽度 hero、不可换行 card header、单行按钮/表单等导致窄屏像素溢出的布局。
- 禁止为绕过 analyzer 关闭全局 lint；确有平台限制时只在最小作用域处理。

---

## 6. 通信日志、真实通信与布局标准

- **Debug-only 日志**：Flutter 使用 `lib/core/config/debug_flags.dart` 的 `upperUiShowCommunicationLog = kDebugMode`；uni-app 使用 `vite.config.js` 注入的 `__UPPER_UI_DEBUG__`。Debug/dev 构建可显示通信日志卡片；Release/Profile/production 构建必须隐藏通信日志卡片，并且 `pushLog` / `_pushLog` 不应采集原始 TX/RX JSON。
- **无模拟功能**：上位机只允许连接真实 MQTT/BLE/平台通道。禁止 mock transport、模拟上报按钮、mock 开关、`mockMode`、`useMock`、`mockRoundTrip`、本地随机遥测、缺少 Broker 时自动回退模拟数据等实现。无 `remote` 能力的版本应显示“不支持远程通信”，命令发送应提示未连接或不支持，不得本地修改遥测伪装设备响应。
- **用户可见状态**：Release 隐藏通信日志后，连接状态、错误提示、按钮 disabled 状态仍必须能让用户判断是否已连接/未配置/不支持。
- **自适应布局**：Flutter 使用 `SafeArea`、`SingleChildScrollView`、`ConstrainedBox`、`Wrap`、`Flexible`/`Expanded` 或断点布局，避免固定 `SizedBox(width: ...)` 撑破屏幕；uni-app 使用 `flex-wrap`、`grid-template-columns: repeat(auto-fit, minmax(...))`、`min-width: 0`、`overflow-wrap: anywhere`。长 URL、MQTT topic、JSON 日志、按钮行、表单行、卡片头必须可换行。

---

## 7. 通信协议约定（与嵌入式对接）

- 上位机与嵌入式通过 **MQTT JSON** 或 **BLE 透传 JSON** 通信，格式以各产品 `PRODUCT_SPEC.md` 为准。
- JSON key 使用 **snake_case**，与固件侧 `esp8266_mqtt.c` / `jdy31_ble.c` 解析保持一致。
- 上行（App → 设备）：`{ "cmd": "<command>", "params": { ... } }`
- 下行（设备 → App）：`{ "type": "telemetry", "data": { ... } }`
- KQZL3 的 WiFi/网页/小程序/App 版本必须默认使用 `projects/KQZL3/app/board_config.h` 中的 MQTT 参数：Broker `121.40.131.194:1883`，Client ID `KQZL3`，用户名 `yskj`，密码 `yskj@123`，上位机发布命令 Topic `KQZL3`，订阅遥测 Topic `KQZL3/web`。Flutter 的 `lib/core/config/mqtt_config.dart` 与 uni-app 的 `src/config/mqtt.js` 必须保持同步。

---

## 8. 新增 feature 标准流程

1. Flutter 默认流程：在 `lib/features/<feature>/` 下创建页面、controller 或状态对象，并把跨 feature 复用模型放入 `lib/core/`。
2. 在版本解析层增加 feature 名称与 capabilities 判断，页面只消费 capabilities，不直接判断版本号。
3. 如涉及通信，新增或复用 `lib/services/` 下的 transport/service 抽象，页面不直接调用 MQTT/BLE SDK。
4. 如项目含微信小程序，再在 uni-app 的 `src/features/<feature>/` 下创建安装函数，并在 `src/features/index.js` 的 `registry` 中追加条目。
5. 如项目含微信小程序，同步更新 `version_features.json` 的 `featureGlobs` 与目标 `versions.<N>`。
6. 本地验证：Flutter 执行 `flutter analyze` / `flutter test`；uni-app 小程序执行 `UPPER_VERSION=<N> UPPER_FEATURES=common,<feature> npm run build:mp-weixin`。
7. 重新运行 `export_project.py`，确认 Flutter-only 项目导出到 `<NAME>_upper_ui_flutter/versions/versionN/`，双栈项目的小程序版本导出到 `<NAME>_upper_ui/versions/versionN/`。

---

## 9. 禁止行为

| 禁止 | 原因 |
| --- | --- |
| 在 feature 组件内直接 `import` 其他 feature 的模块 | 破坏版本隔离，导出时依赖文件未被复制 |
| 在 `src/pages/` 外创建路由页面 | `pages.json` 未注册，uni-app 运行时报错 |
| 在 `android/` 下手写原生业务代码 | 与 `npx cap sync` 覆盖流程冲突 |
| 运行时读取 `process.env.UPPER_*` | 该变量仅构建期有效，运行时为 `undefined` |
| `version_features.json` 中使用绝对路径 | 导出到其他机器路径失效 |
| 修改 `vite.config.js` 中三个 `__UPPER_*__` 常量的变量名 | `export_project.py` 与 `index.js` 均依赖固定名称 |

---

## 10. 构建命令速查

### 新项目生成

```bash
# 默认：只生成 Flutter 上位机
python tools/gen_project.py <NAME>

# 项目需求包含微信小程序：生成 Flutter + uni-app
python tools/gen_project.py <NAME> --with-mini-program
```

### Flutter 主上位机

```bash
cd projects/<NAME>/<NAME>_upper_ui_flutter
flutter pub get
flutter analyze
flutter test
flutter build apk --debug --dart-define=UPPER_VERSION=1 --dart-define=UPPER_FEATURES=common
flutter build web --dart-define=UPPER_VERSION=1 --dart-define=UPPER_FEATURES=common,web
```

### uni-app / Capacitor（仅小程序栈）

```bash
# 本地 H5 开发预览
npm run dev:h5

# 构建 H5（模拟导出环境）
UPPER_VERSION=3 UPPER_FEATURES=common,wifi npm run build:h5

# 构建微信小程序
UPPER_VERSION=3 UPPER_FEATURES=common,wifi npm run build:mp-weixin

# Capacitor 同步到 Android（构建 H5 后）
npx cap sync android

# 由 export_project.py 统一调用（推荐，自动注入环境变量）
python tools/export_project.py --project <NAME> --version 3 -o ../exports
```

Flutter 侧使用 `String.fromEnvironment('UPPER_VERSION')` 与 `String.fromEnvironment('UPPER_FEATURES')`，对应 uni-app 的 `__UPPER_VERSION__` / `__UPPER_FEATURES__`。

---

*最后更新：初版，与 gen_project.py scaffold 模板、export_project.py 版本化导出流程同步（2026-06）。*