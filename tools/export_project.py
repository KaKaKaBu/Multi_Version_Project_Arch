#!/usr/bin/env python3
"""
从 project_template 工程中按版本导出独立可编译目录。

设计约定（新工程需遵循）：
  - CMakeLists.txt 使用 ${TEMPLATE_ROOT} 引用框架路径
  - include(${TEMPLATE_ROOT}/cmake/hal_options.cmake)
  - DRIVER_SRCS 引用 ${DRIVER_CATALOG_<工程名>}（在 driver_catalog.cmake 中定义）
  - 可选：set(<XXX>_VERSION N CACHE STRING ...) 用于多版本导出
  - add_executable(${PROJECT_NAME}.elf ${APP_SRCS} ... ${STARTUP_FILE})

解析策略（尽量不在 Python 硬编码业务规则）：
  - HAL 源/开关、驱动列表 → tools/cmake/resolve_export.cmake（CMake 脚本模式）
  - HAL 前条件逻辑 → 从工程 CMakeLists 截取 preamble 临时 include
  - 源文件组 / 编译宏 / 工具链路径 → 从工程 CMakeLists 正则提取

示例：
  python export_project.py
  python export_project.py --project RTJK_001 --version 10 -o exports
  python export_project.py --project RTJK_001 --batch --versions 1-10 -o exports --clean \\
      --extras readme.txt,PRODUCT_SPEC.md
  python export_project.py --cmake projects/RTJK_001/CMakeLists.txt --version 7 \\
      --toolchain-bin D:/path/to/GNU-tools-for-STM32/bin
  set STM32_TOOLCHAIN_BIN=D:/path/to/bin && python export_project.py --project RTJK_001
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable, Dict, Iterable, List, Optional, Set, Tuple

TEMPLATE_ROOT_NAME = "project_template"
DEFAULT_STM32_TOOLCHAIN_BIN = ""

DRIVER_FEATURE_MACRO_BY_FILE = {
    "a7670c_sms.c": "VERSION_FEATURE_DRIVER_A7670C_SMS",
    "bmp180.c": "VERSION_FEATURE_DRIVER_BMP180",
    "buzzer.c": "VERSION_FEATURE_DRIVER_BUZZER",
    "co2_uart.c": "VERSION_FEATURE_DRIVER_CO2_UART",
    "dht11.c": "VERSION_FEATURE_DRIVER_DHT11",
    "ds1302_rtc.c": "VERSION_FEATURE_DRIVER_DS1302_RTC",
    "ds18b20.c": "VERSION_FEATURE_DRIVER_DS18B20",
    "e18_presence.c": "VERSION_FEATURE_DRIVER_E18_PRESENCE",
    "esp8266_mqtt.c": "VERSION_FEATURE_DRIVER_ESP8266_MQTT",
    "esp8266_wifi.c": "VERSION_FEATURE_DRIVER_ESP8266_WIFI",
    "gl5506_light.c": "VERSION_FEATURE_DRIVER_GL5506_LIGHT",
    "hc595_seg.c": "VERSION_FEATURE_DRIVER_HC595_SEG",
    "hcsr04.c": "VERSION_FEATURE_DRIVER_HCSR04",
    "hx711.c": "VERSION_FEATURE_DRIVER_HX711",
    "ir_clothes.c": "VERSION_FEATURE_DRIVER_IR_CLOTHES",
    "jdy31_ble.c": "VERSION_FEATURE_DRIVER_JDY31_BLE",
    "key.c": "VERSION_FEATURE_DRIVER_KEY",
    "key_4ch.c": "VERSION_FEATURE_DRIVER_KEY_4CH",
    "l76k_gnss.c": "VERSION_FEATURE_DRIVER_L76K_GNSS",
    "led.c": "VERSION_FEATURE_DRIVER_LED",
    "light_channel.c": "VERSION_FEATURE_DRIVER_LIGHT_CHANNEL",
    "max30102.c": "VERSION_FEATURE_DRIVER_MAX30102",
    "mpu6050.c": "VERSION_FEATURE_DRIVER_MPU6050",
    "mq135.c": "VERSION_FEATURE_DRIVER_MQ135",
    "mq2_smoke.c": "VERSION_FEATURE_DRIVER_MQ2_SMOKE",
    "mq4_methane.c": "VERSION_FEATURE_DRIVER_MQ4_METHANE",
    "mq7_co.c": "VERSION_FEATURE_DRIVER_MQ7_CO",
    "msp20_bp.c": "VERSION_FEATURE_DRIVER_MSP20_BP",
    "nrf24l01.c": "VERSION_FEATURE_DRIVER_NRF24L01",
    "oled_font.c": "VERSION_FEATURE_DRIVER_OLED_FONT",
    "oled_ssd1306.c": "VERSION_FEATURE_DRIVER_OLED_SSD1306",
    "ph_sensor.c": "VERSION_FEATURE_DRIVER_PH_SENSOR",
    "pm25.c": "VERSION_FEATURE_DRIVER_PM25",
    "relay.c": "VERSION_FEATURE_DRIVER_RELAY",
    "rfid_rc522.c": "VERSION_FEATURE_DRIVER_RFID_RC522",
    "sg90.c": "VERSION_FEATURE_DRIVER_SG90",
    "stepmotor.c": "VERSION_FEATURE_DRIVER_STEPMOTOR",
    "su03t_voice.c": "VERSION_FEATURE_DRIVER_SU03T_VOICE",
    "water_level.c": "VERSION_FEATURE_DRIVER_WATER_LEVEL",
}

ALL_DRIVER_FEATURE_MACROS = sorted(set(DRIVER_FEATURE_MACRO_BY_FILE.values()))

@dataclass
class ProjectExportInfo:
    project_dir: Path
    project_name: str
    version_var: Optional[str]
    default_version: Optional[int]
    version_range: Optional[Tuple[int, int]]
    available_versions: List[int]
    has_flutter_upper_ui: bool
    has_uniapp_upper_ui: bool


@dataclass
class PruneStats:
    folded_blocks: int = 0
    removed_branches: int = 0
    unknown_blocks: int = 0
    warnings: List[str] = field(default_factory=list)


@dataclass
class ExportPruneContext:
    enabled: bool
    macros: Dict[str, int]
    stats: Dict[str, PruneStats] = field(default_factory=dict)
    warnings: List[str] = field(default_factory=list)


class PreprocessorExprParser:
    def __init__(self, expr: str, macros: Dict[str, int]):
        self.tokens = self._tokenize(expr)
        self.macros = macros
        self.pos = 0
        self.unknown = False

    @staticmethod
    def _tokenize(expr: str) -> List[str]:
        token_re = re.compile(
            r"defined|0[xX][0-9A-Fa-f]+[uUlL]*|\d+[uUlL]*|[A-Za-z_]\w*|&&|\|\||==|!=|<=|>=|<<|>>|[()!<>+\-*/%&|^~]"
        )
        tokens = token_re.findall(expr)
        consumed = "".join(tokens)
        compact = re.sub(r"\s+", "", expr)
        if consumed != compact:
            return ["__UNKNOWN__"]
        return tokens

    def parse(self) -> Optional[int]:
        if not self.tokens:
            return None
        value = self._parse_or()
        if self.unknown or self.pos != len(self.tokens) or value is None:
            return None
        return 1 if value else 0

    def _peek(self) -> Optional[str]:
        if self.pos >= len(self.tokens):
            return None
        return self.tokens[self.pos]

    def _take(self, token: Optional[str] = None) -> Optional[str]:
        current = self._peek()
        if current is None:
            return None
        if token is not None and current != token:
            return None
        self.pos += 1
        return current

    def _parse_or(self) -> Optional[int]:
        left = self._parse_and()
        while self._take("||"):
            right = self._parse_and()
            if left is not None and left != 0:
                left = 1
            elif right is not None and right != 0:
                left = 1
            elif left == 0 and right == 0:
                left = 0
            else:
                left = None
        return left

    def _parse_and(self) -> Optional[int]:
        left = self._parse_compare()
        while self._take("&&"):
            right = self._parse_compare()
            if left == 0 or right == 0:
                left = 0
            elif left is not None and right is not None:
                left = 1 if (left != 0 and right != 0) else 0
            else:
                left = None
        return left

    def _parse_compare(self) -> Optional[int]:
        left = self._parse_add()
        op = self._peek()
        if op not in {"==", "!=", "<", "<=", ">", ">="}:
            return left
        self._take()
        right = self._parse_add()
        if left is None or right is None:
            return None
        if op == "==":
            return 1 if left == right else 0
        if op == "!=":
            return 1 if left != right else 0
        if op == "<":
            return 1 if left < right else 0
        if op == "<=":
            return 1 if left <= right else 0
        if op == ">":
            return 1 if left > right else 0
        return 1 if left >= right else 0

    def _parse_add(self) -> Optional[int]:
        left = self._parse_mul()
        while self._peek() in {"+", "-", "|", "^"}:
            op = self._take()
            right = self._parse_mul()
            if left is None or right is None:
                left = None
            elif op == "+":
                left += right
            elif op == "-":
                left -= right
            elif op == "|":
                left |= right
            else:
                left ^= right
        return left

    def _parse_mul(self) -> Optional[int]:
        left = self._parse_unary()
        while self._peek() in {"*", "/", "%", "&", "<<", ">>"}:
            op = self._take()
            right = self._parse_unary()
            if left is None or right is None:
                left = None
            elif op == "*":
                left *= right
            elif op == "/":
                if right == 0:
                    self.unknown = True
                    return None
                left //= right
            elif op == "%":
                if right == 0:
                    self.unknown = True
                    return None
                left %= right
            elif op == "&":
                left &= right
            elif op == "<<":
                left <<= right
            else:
                left >>= right
        return left

    def _parse_unary(self) -> Optional[int]:
        token = self._peek()
        if token == "!":
            self._take()
            value = self._parse_unary()
            if value is None:
                return None
            return 0 if value else 1
        if token == "+":
            self._take()
            return self._parse_unary()
        if token == "-":
            self._take()
            value = self._parse_unary()
            return None if value is None else -value
        if token == "~":
            self._take()
            value = self._parse_unary()
            return None if value is None else ~value
        return self._parse_primary()

    def _parse_primary(self) -> Optional[int]:
        token = self._peek()
        if token is None:
            self.unknown = True
            return None
        if token == "__UNKNOWN__":
            self.unknown = True
            self._take()
            return None
        if token == "(":
            self._take("(")
            value = self._parse_or()
            if not self._take(")"):
                self.unknown = True
                return None
            return value
        if token == "defined":
            self._take("defined")
            if self._take("("):
                name = self._take()
                if not name or not re.match(r"^[A-Za-z_]\w*$", name) or not self._take(")"):
                    self.unknown = True
                    return None
                return defined_macro_value(name, self.macros)
            name = self._take()
            if not name or not re.match(r"^[A-Za-z_]\w*$", name):
                self.unknown = True
                return None
            return defined_macro_value(name, self.macros)
        self._take()
        if re.match(r"^0[xX][0-9A-Fa-f]+[uUlL]*$", token):
            return int(re.sub(r"[uUlL]+$", "", token), 16)
        if re.match(r"^\d+[uUlL]*$", token):
            return int(re.sub(r"[uUlL]+$", "", token), 10)
        if re.match(r"^[A-Za-z_]\w*$", token):
            if token in self.macros:
                return self.macros[token]
            self.unknown = True
            return None
        self.unknown = True
        return None


def is_controlled_macro(name: str) -> bool:
    return (
        name == "APP_VERSION"
        or name.endswith("VERSION")
        or name.startswith("VERSION_FEATURE_")
        or name.startswith("HAL_")
    )


def defined_macro_value(name: str, macros: Dict[str, int]) -> Optional[int]:
    if name in macros:
        return 1
    if is_controlled_macro(name):
        return 0
    return None


def eval_pp_expr(expr: str, macros: Dict[str, int]) -> Optional[int]:
    return PreprocessorExprParser(expr, macros).parse()


def find_template_root(start: Path) -> Path:
    for parent in [start, *start.parents]:
        if (parent / "cmake" / "driver_catalog.cmake").is_file() and (parent / "tools" / "export_project.py").is_file():
            return parent
        if parent.name == TEMPLATE_ROOT_NAME:
            return parent
        if (parent / TEMPLATE_ROOT_NAME).is_dir():
            return parent / TEMPLATE_ROOT_NAME
    raise FileNotFoundError(f"无法定位 {TEMPLATE_ROOT_NAME} 目录（从 {start} 向上查找）")


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def find_cmake_executable() -> str:
    candidate = shutil.which("cmake")
    if candidate:
        return candidate
    raise FileNotFoundError("未找到 cmake，请安装 CMake 或将其加入 PATH")


def parse_project_name(cmake_text: str) -> str:
    match = re.search(r"project\s*\(\s*(\w+)", cmake_text)
    if not match:
        raise ValueError("CMakeLists.txt 中未找到 project(...) 名称")
    return match.group(1)


def parse_set_block(cmake_text: str, var_name: str) -> List[str]:
    def split_items(body: str) -> List[str]:
        return [
            item.strip().strip('"')
            for item in re.findall(r'"[^"]*"|\S+', body)
            if item.strip()
        ]

    single = re.search(
        rf"set\s*\(\s*{re.escape(var_name)}\s+([^)\n]+)\)",
        cmake_text,
    )
    if single:
        body = single.group(1).strip()
        if body:
            return split_items(body)

    multi = re.search(
        rf"set\s*\(\s*{re.escape(var_name)}\s+(.*?)\n\)",
        cmake_text,
        re.DOTALL,
    )
    if not multi:
        return []
    items: List[str] = []
    for line in multi.group(1).splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        items.extend(split_items(line))
    return items


def split_cmake_args(body: str) -> List[str]:
    items: List[str] = []
    for raw_line in body.splitlines():
        line = raw_line.split("#", 1)[0].strip()
        if not line:
            continue
        items.extend(item.strip().strip('"') for item in re.findall(r'"[^"]*"|\S+', line) if item.strip())
    return items


def parse_cmake_call_args(cmake_text: str, function_name: str) -> List[str]:
    match = re.search(
        rf"{re.escape(function_name)}\s*\((.*?)\)",
        cmake_text,
        re.DOTALL,
    )
    if not match:
        return []
    return split_cmake_args(match.group(1))


def uses_mvp_stm32_template(cmake_text: str) -> bool:
    return bool(parse_cmake_call_args(cmake_text, "mvp_add_stm32f1_project"))


def mvp_has_option(cmake_text: str, option_name: str) -> bool:
    return option_name in parse_cmake_call_args(cmake_text, "mvp_add_stm32f1_project")


def parse_mvp_driver_catalog_var(cmake_text: str) -> Optional[str]:
    args = parse_cmake_call_args(cmake_text, "mvp_add_stm32f1_project")
    for idx, item in enumerate(args):
        if item == "DRIVER_CATALOG_VAR" and idx + 1 < len(args):
            return args[idx + 1]
    return None


def catalog_name_from_var(var_name: Optional[str]) -> Optional[str]:
    if not var_name:
        return None
    prefix = "DRIVER_CATALOG_"
    if var_name.startswith(prefix):
        return var_name[len(prefix) :]
    return None


def parse_driver_catalog_ref(items: List[str]) -> Optional[str]:
    for item in items:
        match = re.search(r"\$\{DRIVER_CATALOG_(\w+)\}", item.strip())
        if match:
            return match.group(1)
    return None


def parse_version_var(cmake_text: str) -> Optional[str]:
    if parse_cmake_call_args(cmake_text, "mvp_resolve_app_version"):
        return "APP_VERSION"
    app_match = re.search(r"set\s*\(\s*APP_VERSION\s+\"?(\d+)\"?\s+CACHE\s+STRING", cmake_text)
    if app_match:
        return "APP_VERSION"
    match = re.search(r"set\s*\(\s*(\w+VERSION)\s+\"?(\d+)\"?\s+CACHE\s+STRING", cmake_text)
    if match:
        return match.group(1)
    return None


def parse_legacy_version_var(cmake_text: str) -> Optional[str]:
    for match in re.finditer(r"set\s*\(\s*(\w+VERSION)\s+[^)]*CACHE\s+STRING", cmake_text):
        name = match.group(1)
        if name != "APP_VERSION":
            return name
    return None


def parse_version_default(cmake_text: str, version_var: str) -> int:
    if version_var == "APP_VERSION":
        args = parse_cmake_call_args(cmake_text, "mvp_resolve_app_version")
        for idx, item in enumerate(args):
            if item == "DEFAULT" and idx + 1 < len(args):
                return int(args[idx + 1])
    match = re.search(
        rf"set\s*\(\s*{re.escape(version_var)}\s+\"?(\d+)\"?\s+CACHE\s+STRING",
        cmake_text,
    )
    if match:
        return int(match.group(1))
    return 1


def parse_version_range(cmake_text: str, version_var: str) -> Tuple[int, int]:
    if version_var == "APP_VERSION":
        args = parse_cmake_call_args(cmake_text, "mvp_resolve_app_version")
        min_value: Optional[int] = None
        max_value: Optional[int] = None
        for idx, item in enumerate(args):
            if item == "MIN" and idx + 1 < len(args):
                min_value = int(args[idx + 1])
            elif item == "MAX" and idx + 1 < len(args):
                max_value = int(args[idx + 1])
        if min_value is not None and max_value is not None:
            return min_value, max_value
    pattern = rf"{re.escape(version_var)} must be (\d+)-(\d+)"
    match = re.search(pattern, cmake_text)
    if match:
        return int(match.group(1)), int(match.group(2))
    default = parse_version_default(cmake_text, version_var)
    return default, default


def parse_executable_groups(cmake_text: str) -> List[str]:
    if uses_mvp_stm32_template(cmake_text):
        return ["APP_SRCS", "COMMON_SRCS", "BSP_SRCS", "DRIVER_SRCS", "SPL_SRCS"]

    match = re.search(
        r"add_executable\s*\((.*?)\)",
        cmake_text,
        re.DOTALL,
    )
    if not match:
        return ["APP_SRCS", "COMMON_SRCS", "BSP_SRCS", "DRIVER_SRCS", "SPL_SRCS"]

    groups: List[str] = []
    for ref in re.findall(r"\$\{(\w+)\}", match.group(1)):
        if ref != "PROJECT_NAME" and ref not in groups:
            groups.append(ref)
    return groups


def parse_target_compile_definitions(cmake_text: str) -> List[str]:
    if uses_mvp_stm32_template(cmake_text):
        defs = ["STM32F10X_MD", "USE_STDPERIPH_DRIVER", "CJSON_NESTING_LIMIT=32"]
        match = re.search(
            r"mvp_add_stm32f1_project\s*\((.*?)\)",
            cmake_text,
            re.DOTALL,
        )
        if match:
            args = split_cmake_args(match.group(1))
            if "EXTRA_DEFINES" in args:
                start = args.index("EXTRA_DEFINES") + 1
                stop = len(args)
                for idx in range(start, len(args)):
                    if args[idx] in {"DRIVER_CATALOG_VAR", "ENABLE_APP_FRAMEWORK"}:
                        stop = idx
                        break
                defs.extend(args[start:stop])
        return defs

    match = re.search(
        r"target_compile_definitions\s*\(\s*\$\{PROJECT_NAME\}\.elf\s+PRIVATE\s+(.*?)\n\)",
        cmake_text,
        re.DOTALL,
    )
    if not match:
        return []

    defs: List[str] = []
    for line in match.group(1).splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("${") or "=" in line:
            continue
        defs.append(line)
    return defs


def parse_link_undef_symbols(cmake_text: str) -> List[str]:
    if uses_mvp_stm32_template(cmake_text):
        return ["malloc", "_malloc_r", "cjson_port_init"]
    return re.findall(r"-Wl,-u,(\w+)", cmake_text)


def parse_cmake_quoted_value(cmake_text: str, var_name: str) -> Optional[str]:
    match = re.search(rf'set\s*\(\s*{re.escape(var_name)}\s+"([^"]+)"', cmake_text)
    if match:
        return match.group(1)
    return None


def resolve_toolchain_bin(
    src_cmake_text: str,
    cli_override: Optional[str] = None,
) -> str:
    if cli_override:
        return cli_override.replace("\\", "/")
    env = os.environ.get("STM32_TOOLCHAIN_BIN", "").strip()
    if env:
        return env.replace("\\", "/")
    parsed = parse_cmake_quoted_value(src_cmake_text, "STM32_TOOLCHAIN_BIN")
    if parsed:
        return parsed.replace("\\", "/")
    return ""


def parse_extras_list(extras_arg: Optional[List[str]], no_extras: bool) -> List[str]:
    if no_extras:
        return []
    if not extras_arg:
        return []

    result: List[str] = []
    for item in extras_arg:
        for part in item.split(","):
            part = part.strip()
            if part:
                result.append(part)
    return result


def extract_hal_preamble(cmake_text: str) -> str:
    """截取 include(hal_options.cmake) 之前的工程条件逻辑（版本/option 等）。"""
    if uses_mvp_stm32_template(cmake_text):
        lines: List[str] = []
        for line in cmake_text.splitlines():
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                continue
            if stripped.startswith("mvp_add_stm32f1_project"):
                break
            if re.match(r"set\s*\(\s*HAL_", stripped):
                lines.append(line)
        return "\n".join(lines) + ("\n" if lines else "")

    match = re.search(
        r"^(.*?)include\s*\(\s*\$\{TEMPLATE_ROOT\}/cmake/hal_options\.cmake\s*\)",
        cmake_text,
        re.MULTILINE | re.DOTALL,
    )
    if not match:
        return ""

    skip_prefixes = (
        "cmake_minimum_required",
        "set(CMAKE_SYSTEM_NAME",
        "set(CMAKE_SYSTEM_PROCESSOR",
        "set(CMAKE_TRY_COMPILE_TARGET_TYPE",
        "project(",
    )
    lines: List[str] = []
    for line in match.group(1).splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        if any(stripped.startswith(prefix) for prefix in skip_prefixes):
            continue
        if "stm32-gcc-toolchain.cmake" in stripped:
            continue
        if re.match(r"set\s*\(\s*TEMPLATE_ROOT\s", stripped):
            continue
        lines.append(line)
    if not lines:
        return ""
    return "\n".join(lines) + "\n"


def run_cmake_export_resolve(
    template_root: Path,
    catalog_name: Optional[str],
    version_var: Optional[str],
    version: int,
    preamble_text: str,
) -> Tuple[Dict[str, List[Path]], Dict[str, bool], List[Path]]:
    script = template_root / "tools" / "cmake" / "resolve_export.cmake"
    if not script.exists():
        raise FileNotFoundError(f"缺少解析脚本: {script}")

    cmake_bin = find_cmake_executable()
    preamble_file: Optional[Path] = None
    if preamble_text.strip():
        tmp = tempfile.NamedTemporaryFile(
            mode="w",
            suffix=".cmake",
            prefix="export_hal_preamble_",
            delete=False,
            encoding="utf-8",
        )
        tmp.write(preamble_text)
        tmp.close()
        preamble_file = Path(tmp.name)

    args = [
        cmake_bin,
        f"-DTEMPLATE_ROOT={template_root.resolve().as_posix()}",
    ]
    if catalog_name:
        args.append(f"-DCATALOG_NAME={catalog_name}")
    if preamble_file:
        args.append(f"-DPROJECT_HAL_PREAMBLE={preamble_file.resolve().as_posix()}")
    if version_var and version > 0:
        args.append(f"-D{version_var}={version}")
        if version_var == "APP_VERSION":
            legacy_version_var = parse_legacy_version_var(preamble_text)
            if legacy_version_var:
                args.append(f"-D{legacy_version_var}={version}")
    args.extend(["-P", str(script.resolve())])

    try:
        proc = subprocess.run(
            args,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
    finally:
        if preamble_file and preamble_file.exists():
            preamble_file.unlink(missing_ok=True)

    if proc.returncode != 0:
        raise RuntimeError(
            "CMake 导出解析失败:\n"
            f"command: {' '.join(args)}\n"
            f"stdout:\n{proc.stdout}\n"
            f"stderr:\n{proc.stderr}"
        )

    hal_map: Dict[str, List[Path]] = {}
    hal_options: Dict[str, bool] = {}
    drivers: List[Path] = []
    seen_drivers: Set[str] = set()

    for stream in (proc.stdout, proc.stderr):
        for line in stream.splitlines():
            if "EXPORT_HAL_SRC:" in line:
                _, payload = line.split("EXPORT_HAL_SRC:", 1)
                group, path = payload.split(":", 1)
                hal_map.setdefault(group.strip(), []).append(Path(path.strip()))
            elif "EXPORT_HAL_DEFINE:" in line:
                _, payload = line.split("EXPORT_HAL_DEFINE:", 1)
                name, value = payload.split("=", 1)
                hal_options[name.strip()] = value.strip() == "1"
            elif "EXPORT_DRIVER:" in line:
                _, path = line.split("EXPORT_DRIVER:", 1)
                item = path.strip()
                if item and item not in seen_drivers:
                    seen_drivers.add(item)
                    drivers.append(Path(item))

    if catalog_name and not drivers:
        raise RuntimeError(
            f"driver_catalog 未返回驱动 (catalog={catalog_name}, version={version})\n"
            f"stdout:\n{proc.stdout}\nstderr:\n{proc.stderr}"
        )

    return hal_map, hal_options, drivers


def expand_cmake_item(
    item: str,
    template_root: Path,
    project_dir: Path,
    hal_map: Dict[str, List[Path]],
    driver_paths: List[Path],
) -> List[Path]:
    item = item.strip()
    if not item:
        return []

    if item.startswith("${") and item.endswith("}"):
        var = item[2:-1]
        if var in hal_map:
            return list(hal_map[var])
        if var.startswith("DRIVER_CATALOG_"):
            return list(driver_paths)
        return []

    if item.startswith("${TEMPLATE_ROOT}/"):
        rel = item[len("${TEMPLATE_ROOT}/") :]
        return [template_root / rel.replace("\\", "/")]

    if item.startswith("app/") or item.startswith("board/"):
        return [project_dir / item]

    path = Path(item)
    if path.is_absolute():
        return [path]
    return [project_dir / item]


def resolve_mvp_stm32_source_lists(
    cmake_text: str,
    template_root: Path,
    project_dir: Path,
    version: int,
    version_var: Optional[str],
) -> Tuple[Dict[str, List[Path]], Dict[str, bool]]:
    catalog_name = catalog_name_from_var(parse_mvp_driver_catalog_var(cmake_text))
    preamble = extract_hal_preamble(cmake_text)
    hal_map, hal_options, driver_paths = run_cmake_export_resolve(
        template_root,
        catalog_name,
        version_var,
        version,
        preamble,
    )

    common_sources = [
        template_root / "common" / "device_manager" / "devmgr.c",
        template_root / "common" / "device_manager" / "driver_registry_gcc.c",
        template_root / "common" / "scheduler" / "scheduler.c",
        template_root / "common" / "scheduler" / "sched_loop.c",
        template_root / "common" / "irq_event" / "irq_event.c",
        template_root / "common" / "utils" / "tiny_printf.c",
        template_root / "common" / "utils" / "mem_pool.c",
        template_root / "common" / "utils" / "cJSON.c",
        template_root / "common" / "utils" / "cjson_port.c",
        template_root / "common" / "utils" / "soft_uart.c",
        template_root / "common" / "utils" / "debug_uart.c",
        template_root / "common" / "utils" / "comm_port.c",
    ]
    if mvp_has_option(cmake_text, "ENABLE_APP_FRAMEWORK"):
        common_sources.append(template_root / "common" / "app_framework" / "app_fsm.c")

    bsp_sources = [
        template_root / "bsp" / "stm32" / "syscalls" / "syscalls.c",
        template_root / "bsp" / "stm32" / "hal" / "stm32f1_hal_map.c",
        template_root / "bsp" / "stm32" / "hal" / "hal_common.c",
        template_root / "bsp" / "stm32" / "hal" / "gpio_hal.c",
        template_root / "bsp" / "stm32" / "hal" / "exti_hal.c",
        *hal_map.get("BSP_HAL_I2C_SRC", []),
        *hal_map.get("BSP_HAL_SPI_SRC", []),
        template_root / "bsp" / "stm32" / "hal" / "usart_hal.c",
        *hal_map.get("BSP_HAL_USART_DMA_SRC", []),
        *hal_map.get("BSP_HAL_ADC_SRC", []),
        *hal_map.get("BSP_HAL_ADC_DMA_SRC", []),
        template_root / "bsp" / "stm32" / "hal" / "timer_hal.c",
        template_root / "bsp" / "stm32" / "irq_handlers" / "irq_handlers.c",
    ]

    spl_sources = [
        template_root / "bsp" / "stm32" / "vendor" / "Libraries" / "CMSIS" / "system_stm32f10x.c",
        template_root / "bsp" / "stm32" / "vendor" / "Libraries" / "FWlib" / "src" / "misc.c",
        template_root / "bsp" / "stm32" / "vendor" / "Libraries" / "FWlib" / "src" / "stm32f10x_gpio.c",
        template_root / "bsp" / "stm32" / "vendor" / "Libraries" / "FWlib" / "src" / "stm32f10x_exti.c",
        *hal_map.get("SPL_HAL_I2C_SRC", []),
        *hal_map.get("SPL_HAL_SPI_SRC", []),
        template_root / "bsp" / "stm32" / "vendor" / "Libraries" / "FWlib" / "src" / "stm32f10x_rcc.c",
        template_root / "bsp" / "stm32" / "vendor" / "Libraries" / "FWlib" / "src" / "stm32f10x_tim.c",
        template_root / "bsp" / "stm32" / "vendor" / "Libraries" / "FWlib" / "src" / "stm32f10x_usart.c",
        *hal_map.get("SPL_HAL_ADC_SRC", []),
        *hal_map.get("SPL_HAL_DMA_SRC", []),
    ]

    source_groups: Dict[str, List[Path]] = {
        "APP_SRCS": [
            project_dir / "app" / "app_main.c",
            project_dir / "app" / "app_callbacks.c",
            project_dir / "app" / "app_logic.c",
            project_dir / "board" / "board_devices.c",
        ],
        "COMMON_SRCS": common_sources,
        "BSP_SRCS": bsp_sources,
        "DRIVER_SRCS": list(driver_paths),
        "SPL_SRCS": spl_sources,
        "STARTUP_FILE": [template_root / "bsp" / "stm32" / "startup" / "startup_stm32f103xb.s"],
        "LINKER_SCRIPT": [template_root / "bsp" / "stm32" / "linker_scripts" / "STM32F103XX_FLASH.ld"],
    }
    for key, paths in list(source_groups.items()):
        seen: Set[Path] = set()
        deduped: List[Path] = []
        for path in paths:
            if path not in seen:
                seen.add(path)
                deduped.append(path)
        source_groups[key] = deduped
    return source_groups, hal_options


def resolve_source_lists(
    cmake_text: str,
    template_root: Path,
    project_dir: Path,
    version: int,
    version_var: Optional[str],
) -> Tuple[Dict[str, List[Path]], Dict[str, bool]]:
    if uses_mvp_stm32_template(cmake_text):
        return resolve_mvp_stm32_source_lists(cmake_text, template_root, project_dir, version, version_var)

    driver_items = parse_set_block(cmake_text, "DRIVER_SRCS")
    catalog_name = parse_driver_catalog_ref(driver_items)

    preamble = extract_hal_preamble(cmake_text)
    hal_map, hal_options, driver_paths = run_cmake_export_resolve(
        template_root,
        catalog_name,
        version_var,
        version,
        preamble,
    )

    source_groups = parse_executable_groups(cmake_text)
    resolved: Dict[str, List[Path]] = {}

    for group in source_groups:
        if group == "DRIVER_SRCS":
            resolved[group] = list(driver_paths)
            continue
        resolved[group] = []
        for item in parse_set_block(cmake_text, group):
            resolved[group].extend(
                expand_cmake_item(item, template_root, project_dir, hal_map, driver_paths)
            )

    startup_match = re.search(r"set\s*\(\s*STARTUP_FILE\s+(\S+)\s*\)", cmake_text)
    if startup_match:
        startup = startup_match.group(1).replace("${TEMPLATE_ROOT}/", "")
        resolved["STARTUP_FILE"] = [template_root / startup.replace("\\", "/")]
    else:
        resolved["STARTUP_FILE"] = []

    linker_match = re.search(r"set\s*\(\s*LINKER_SCRIPT\s+(\S+)\s*\)", cmake_text)
    if linker_match:
        linker = linker_match.group(1).replace("${TEMPLATE_ROOT}/", "")
        resolved["LINKER_SCRIPT"] = [template_root / linker.replace("\\", "/")]
    else:
        resolved["LINKER_SCRIPT"] = []

    return resolved, hal_options


def collect_include_dirs(cmake_text: str, template_root: Path, project_dir: Path) -> List[Path]:
    if uses_mvp_stm32_template(cmake_text):
        includes = [
            project_dir / "board",
            project_dir / "app",
            template_root / "common" / "interfaces",
            template_root / "common" / "driver_core",
            template_root / "common" / "device_manager",
            template_root / "common" / "scheduler",
            template_root / "common" / "app_framework",
            template_root / "common" / "irq_event",
            template_root / "common" / "utils",
            template_root / "drivers" / "comm",
            template_root / "drivers" / "config",
            template_root / "bsp" / "board",
            template_root / "hal_wrapper",
            template_root / "bsp" / "stm32" / "hal",
            template_root / "bsp" / "stm32" / "vendor" / "Libraries" / "CMSIS",
            template_root / "bsp" / "stm32" / "vendor" / "Libraries" / "FWlib" / "inc",
        ]
        return includes

    block_match = re.search(
        r"target_include_directories\s*\(\s*\$\{PROJECT_NAME\}\.elf\s+PRIVATE\s+(.*?)\n\)",
        cmake_text,
        re.DOTALL,
    )
    includes: List[Path] = [project_dir / "board", project_dir / "app"]
    if not block_match:
        return includes

    for line in block_match.group(1).splitlines():
        line = line.strip()
        if not line or line in {"app", "board"}:
            continue
        if line.startswith("${TEMPLATE_ROOT}/"):
            includes.append(template_root / line[len("${TEMPLATE_ROOT}/") :])
        elif not line.startswith("$"):
            includes.append(project_dir / line)
    return includes


def collect_header_files(include_dirs: Iterable[Path]) -> Set[Path]:
    headers: Set[Path] = set()
    for directory in include_dirs:
        if not directory.exists():
            continue
        for path in directory.rglob("*"):
            if path.is_file() and path.suffix.lower() in {".h", ".hpp"}:
                headers.add(path.resolve())
    return headers


def driver_local_headers(driver_sources: Iterable[Path]) -> Set[Path]:
    headers: Set[Path] = set()
    for src in driver_sources:
        for ext in (".h", ".hpp"):
            candidate = src.with_suffix(ext)
            if candidate.exists():
                headers.add(candidate.resolve())
    return headers


def normalize_pp_expr(expr: str) -> str:
    return re.sub(r"\\\s*\n", " ", expr).strip()


def parse_pp_directive(line: str) -> Tuple[Optional[str], str]:
    normalized = re.sub(r"\\\s*\n\s*", " ", line)
    match = re.match(r"^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)$", normalized)
    if not match:
        return None, ""
    return match.group(1), match.group(2).strip()


def is_header_guard_name(name: str) -> bool:
    return bool(re.match(r"^[A-Z][A-Z0-9_]*_H_?$", name))


def directive_condition(kind: str, arg: str, macros: Dict[str, int]) -> Optional[int]:
    if kind in {"ifdef", "ifndef"}:
        parts = arg.split()
        if not parts:
            return None
        if kind == "ifndef" and is_header_guard_name(parts[0]):
            return None
        value = defined_macro_value(parts[0], macros)
        if value is None:
            return None
        return value if kind == "ifdef" else 0 if value else 1
    return eval_pp_expr(normalize_pp_expr(arg), macros)


def collect_logical_line(lines: List[str], index: int) -> Tuple[str, int]:
    parts = [lines[index]]
    end = index
    while parts[-1].rstrip().endswith("\\") and end + 1 < len(lines):
        end += 1
        parts.append(lines[end])
    return "".join(parts), end + 1


def find_conditional_end(lines: List[str], start: int, stop: int) -> int:
    depth = 0
    index = start
    while index < stop:
        logical, next_index = collect_logical_line(lines, index)
        kind, _ = parse_pp_directive(logical)
        if kind in {"if", "ifdef", "ifndef"}:
            depth += 1
        elif kind == "endif":
            depth -= 1
            if depth == 0:
                return next_index
        index = next_index
    return stop


def parse_conditional_chain(lines: List[str], start: int, stop: int) -> Tuple[List[Tuple[str, str, int, int]], int, bool]:
    first_logical, index = collect_logical_line(lines, start)
    first_kind, first_arg = parse_pp_directive(first_logical)
    if first_kind not in {"if", "ifdef", "ifndef"}:
        return [], start + 1, False

    branches: List[Tuple[str, str, int, int]] = []
    current_kind = first_kind
    current_arg = first_arg
    body_start = index
    depth = 0

    while index < stop:
        logical, next_index = collect_logical_line(lines, index)
        kind, arg = parse_pp_directive(logical)
        if kind in {"if", "ifdef", "ifndef"}:
            depth += 1
        elif kind == "endif":
            if depth == 0:
                branches.append((current_kind, current_arg, body_start, index))
                return branches, next_index, True
            depth -= 1
        elif kind in {"elif", "else"} and depth == 0:
            branches.append((current_kind, current_arg, body_start, index))
            current_kind = kind
            current_arg = arg
            body_start = next_index
        index = next_index

    return branches, stop, False


def prune_lines(lines: List[str], start: int, stop: int, macros: Dict[str, int], stats: PruneStats) -> List[str]:
    output: List[str] = []
    index = start
    while index < stop:
        logical, next_index = collect_logical_line(lines, index)
        kind, _ = parse_pp_directive(logical)
        if kind in {"if", "ifdef", "ifndef"}:
            branches, after, complete = parse_conditional_chain(lines, index, stop)
            if not complete:
                output.extend(lines[index:next_index])
                index = next_index
                continue
            pruned = prune_conditional_chain(lines, branches, macros, stats, index, after)
            output.extend(pruned)
            index = after
            continue
        output.extend(lines[index:next_index])
        index = next_index
    return output


def preserve_unknown_conditional_chain(
    lines: List[str],
    branches: List[Tuple[str, str, int, int]],
    macros: Dict[str, int],
    stats: PruneStats,
    chain_start: int,
    chain_end: int,
) -> List[str]:
    output: List[str] = []
    cursor = chain_start
    for _, _, body_start, body_end in branches:
        output.extend(lines[cursor:body_start])
        output.extend(prune_lines(lines, body_start, body_end, macros, stats))
        cursor = body_end
    output.extend(lines[cursor:chain_end])
    return output


def prune_conditional_chain(
    lines: List[str],
    branches: List[Tuple[str, str, int, int]],
    macros: Dict[str, int],
    stats: PruneStats,
    chain_start: int,
    chain_end: int,
) -> List[str]:
    selected: Optional[Tuple[int, int]] = None
    unknown = False
    false_branches = 0

    for kind, arg, body_start, body_end in branches:
        if kind == "else":
            if unknown:
                break
            selected = (body_start, body_end)
            break
        cond = directive_condition(kind, arg, macros)
        if cond is None:
            unknown = True
            break
        if cond != 0:
            selected = (body_start, body_end)
            break
        false_branches += 1

    if unknown:
        stats.unknown_blocks += 1
        return preserve_unknown_conditional_chain(lines, branches, macros, stats, chain_start, chain_end)

    stats.folded_blocks += 1
    stats.removed_branches += max(0, len(branches) - (1 if selected else 0))
    if selected is None:
        return []
    body_start, body_end = selected
    return prune_lines(lines, body_start, body_end, macros, stats)


def prune_inactive_conditionals(text: str, ctx: ExportPruneContext, rel_path: Path) -> str:
    lines = text.splitlines(keepends=True)
    stats = PruneStats()
    if re.search(r"^\s*#\s*(if|elif).*\bAPP_VERSION\b", text, re.MULTILINE):
        stats.warnings.append(f"{rel_path.as_posix()}: direct APP_VERSION conditional remains in source")
    pruned = "".join(prune_lines(lines, 0, len(lines), ctx.macros, stats))
    if stats.folded_blocks or stats.removed_branches or stats.unknown_blocks or stats.warnings:
        ctx.stats[rel_path.as_posix()] = stats
        ctx.warnings.extend(stats.warnings)
    return pruned


def parse_numeric_define(line: str, macros: Dict[str, int]) -> Optional[Tuple[str, int]]:
    match = re.match(r"^\s*#\s*define\s+([A-Za-z_]\w*)\s+(.+?)\s*$", line)
    if not match:
        return None
    name, value_expr = match.groups()
    value_expr = value_expr.split("/*", 1)[0].split("//", 1)[0].strip()
    value = eval_pp_expr(value_expr, macros)
    if value is None:
        return None
    return name, value


def collect_version_feature_macros(version_config_path: Path, macros: Dict[str, int]) -> Dict[str, int]:
    if not version_config_path.exists():
        return {}
    lines = read_text(version_config_path).splitlines(keepends=True)
    collected: Dict[str, int] = {}
    local_macros = dict(macros)
    stack: List[Dict[str, object]] = []
    active = True
    index = 0

    while index < len(lines):
        logical, next_index = collect_logical_line(lines, index)
        kind, arg = parse_pp_directive(logical)
        if kind in {"if", "ifdef", "ifndef"}:
            cond = directive_condition(kind, arg, local_macros)
            if cond is None:
                name = arg.split()[0] if arg.split() else ""
                cond = 1 if kind == "ifndef" and is_header_guard_name(name) else 0
            branch_active = active and cond != 0
            stack.append({"parent": active, "active": branch_active, "taken": branch_active})
            active = branch_active
        elif kind == "elif" and stack:
            frame = stack[-1]
            if frame["taken"]:
                active = False
            else:
                cond = directive_condition("if", arg, local_macros)
                branch_active = bool(frame["parent"]) and cond is not None and cond != 0
                frame["taken"] = branch_active
                frame["active"] = branch_active
                active = branch_active
        elif kind == "else" and stack:
            frame = stack[-1]
            branch_active = bool(frame["parent"]) and not bool(frame["taken"])
            frame["taken"] = True
            frame["active"] = branch_active
            active = branch_active
        elif kind == "endif" and stack:
            stack.pop()
            active = bool(stack[-1]["active"]) if stack else True
        elif active:
            parsed = parse_numeric_define(logical, local_macros)
            if parsed:
                name, value = parsed
                local_macros[name] = value
                if name.startswith("VERSION_FEATURE_"):
                    collected[name] = value
        index = next_index
    return collected


def collect_driver_feature_macros(driver_sources: Iterable[Path]) -> Dict[str, int]:
    collected = {name: 0 for name in ALL_DRIVER_FEATURE_MACROS}
    for src in driver_sources:
        macro = DRIVER_FEATURE_MACRO_BY_FILE.get(src.name)
        if macro:
            collected[macro] = 1
    return collected


def build_export_prune_context(
    project_dir: Path,
    cmake_text: str,
    version_var: Optional[str],
    version: int,
    hal_options: Dict[str, bool],
    driver_sources: Iterable[Path],
    enabled: bool,
) -> ExportPruneContext:
    macros: Dict[str, int] = {}
    if version_var and version > 0:
        macros[version_var] = version
        macros["APP_VERSION"] = version
        legacy = parse_legacy_version_var(cmake_text)
        if legacy:
            macros[legacy] = version
    for name, value in hal_options.items():
        macros[name] = 1 if value else 0
    macros.update(collect_version_feature_macros(project_dir / "app" / "version_config.h", macros))
    macros.update(collect_driver_feature_macros(driver_sources))
    return ExportPruneContext(enabled=enabled, macros=macros)


def is_prune_candidate(src: Path, template_root: Path, project_dir: Path) -> bool:
    if src.suffix.lower() not in {".c", ".h"}:
        return False
    resolved = src.resolve()
    template_root = template_root.resolve()
    project_dir = project_dir.resolve()
    excluded = [
        template_root / "bsp" / "stm32" / "vendor",
        template_root / "common" / "utils" / "cJSON.c",
        template_root / "common" / "utils" / "cJSON.h",
        template_root / "hal_wrapper" / "hal_features.h",
        project_dir / "app" / "version_config.h",
    ]
    for item in excluded:
        try:
            if item.is_dir() and resolved.relative_to(item.resolve()):
                return False
        except ValueError:
            pass
        if resolved == item.resolve():
            return False
    include_roots = [
        project_dir / "board",
        project_dir / "app",
        template_root / "common",
        template_root / "drivers",
        template_root / "hal_wrapper",
        template_root / "bsp" / "stm32" / "hal",
        template_root / "bsp" / "stm32" / "irq_handlers",
    ]
    for root in include_roots:
        try:
            resolved.relative_to(root.resolve())
            return True
        except ValueError:
            continue
    return False


def patch_version_header(text: str, version_var: Optional[str], version: int) -> str:
    if not version_var:
        return text
    patched = re.sub(
        r"#ifndef\s+APP_VERSION\s*\n#define\s+APP_VERSION\s+\d+",
        f"#ifndef APP_VERSION\n#define APP_VERSION {version}",
        text,
    )
    if patched != text or version_var == "APP_VERSION":
        return patched
    return re.sub(
        rf"#ifndef\s+{re.escape(version_var)}\s*\n#define\s+{re.escape(version_var)}\s+\d+",
        f"#ifndef {version_var}\n#define {version_var} {version}",
        text,
    )


def patch_version_in_app_files(
    filename: str,
    content: str,
    version_var: Optional[str],
    version: int,
) -> str:
    if not version_var:
        return content
    if filename in {"version_config.h", "board_config.h"}:
        return patch_version_header(content, version_var, version)
    return content


def is_app_path(path: Path, project_dir: Path) -> bool:
    try:
        path.resolve().relative_to((project_dir / "app").resolve())
        return True
    except ValueError:
        return False


def is_project_owned_path(path: Path, project_dir: Path) -> bool:
    resolved = path.resolve()
    for dirname in ("app", "board"):
        try:
            resolved.relative_to((project_dir / dirname).resolve())
            return True
        except ValueError:
            continue
    return False


def rel_export_path(path: Path, template_root: Path, project_dir: Path) -> Path:
    resolved = path.resolve()
    template_root = template_root.resolve()
    project_dir = project_dir.resolve()
    app_dir = project_dir / "app"
    board_dir = project_dir / "board"
    try:
        return Path("board") / resolved.relative_to(board_dir.resolve())
    except ValueError:
        pass
    try:
        return Path("app") / resolved.relative_to(app_dir.resolve())
    except ValueError:
        pass
    try:
        return resolved.relative_to(template_root)
    except ValueError:
        return resolved.relative_to(project_dir)


def append_prune_manifest(manifest_lines: List[str], prune_ctx: ExportPruneContext) -> None:
    manifest_lines.extend(["", "[prune_macros]"])
    manifest_lines.append(f"enabled={'1' if prune_ctx.enabled else '0'}")
    for name in sorted(prune_ctx.macros):
        if name == "APP_VERSION" or name.startswith("VERSION_FEATURE_") or name.startswith("HAL_") or name.endswith("VERSION"):
            manifest_lines.append(f"{name}={prune_ctx.macros[name]}")

    manifest_lines.extend(["", "[prune_stats]"])
    for path, stats in sorted(prune_ctx.stats.items()):
        manifest_lines.append(
            f"{path}: folded={stats.folded_blocks}, removed={stats.removed_branches}, unknown={stats.unknown_blocks}"
        )

    if prune_ctx.warnings:
        manifest_lines.extend(["", "[prune_warnings]"])
        for warning in prune_ctx.warnings:
            manifest_lines.append(warning)


def copy_export_file(
    src: Path,
    dst: Path,
    version_var: Optional[str],
    version: int,
    prune_ctx: ExportPruneContext,
    template_root: Path,
    project_dir: Path,
) -> None:
    if src.suffix.lower() in {".h", ".c"}:
        if not is_project_owned_path(src, project_dir) and not (prune_ctx.enabled and is_prune_candidate(src, template_root, project_dir)):
            copy_file(src, dst)
            return
        content = patch_version_in_app_files(src.name, read_text(src), version_var, version)
        rel = rel_export_path(src, template_root, project_dir)
        if prune_ctx.enabled and is_prune_candidate(src, template_root, project_dir):
            content = prune_inactive_conditionals(content, prune_ctx, rel)
        write_text(dst, content)
    else:
        copy_file(src, dst)


def copy_file(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def hal_compile_definitions(hal_options: Dict[str, bool]) -> List[str]:
    return [f"{name}={'1' if enabled else '0'}" for name, enabled in sorted(hal_options.items())]


def generate_standalone_cmake(
    project_name: str,
    source_groups: Dict[str, List[Path]],
    executable_groups: List[str],
    include_dirs: List[Path],
    template_root: Path,
    project_dir: Path,
    hal_options: Dict[str, bool],
    version_var: Optional[str],
    version: int,
    src_cmake_text: str,
    toolchain_bin: str,
) -> str:
    def rel_list(paths: List[Path]) -> List[str]:
        out: List[str] = []
        for path in paths:
            out.append(rel_export_path(path, template_root, project_dir).as_posix())
        return sorted(set(out))

    startup = rel_list(source_groups.get("STARTUP_FILE", []))
    linker_items = rel_list(source_groups.get("LINKER_SCRIPT", []))

    include_rel: List[str] = []
    seen_includes: Set[str] = set()
    for directory in include_dirs:
        rel = rel_export_path(directory, template_root, project_dir).as_posix()
        if rel not in seen_includes:
            include_rel.append(rel)
            seen_includes.add(rel)
    if "app" not in seen_includes:
        include_rel.insert(0, "app")

    compile_defs = list(parse_target_compile_definitions(src_cmake_text))
    if version_var and version > 0:
        compile_defs.append(f"{version_var}={version}")
        if version_var == "APP_VERSION":
            legacy_version_var = parse_legacy_version_var(src_cmake_text)
            if legacy_version_var:
                compile_defs.append(f"{legacy_version_var}={version}")
    compile_defs.extend(hal_compile_definitions(hal_options))

    cpu_flag_items = parse_set_block(src_cmake_text, "CPU_FLAGS")
    if not cpu_flag_items:
        cpu_flag_items = ["-mcpu=cortex-m3", "-mthumb"]

    link_libs = ""
    if uses_mvp_stm32_template(src_cmake_text) or ("target_link_libraries" in src_cmake_text and "PRIVATE m" in src_cmake_text):
        link_libs = "\ntarget_link_libraries(${PROJECT_NAME}.elf PRIVATE m)"

    link_extras = ""
    for sym in parse_link_undef_symbols(src_cmake_text):
        link_extras += f"\n    -Wl,-u,{sym}"

    lines: List[str] = [
        "cmake_minimum_required(VERSION 3.20)",
        "",
        "include(${CMAKE_CURRENT_LIST_DIR}/cmake/stm32-gcc-toolchain.cmake)",
        "",
        f"project({project_name} C ASM)",
        "",
        "set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR})",
        "",
        f"set(LINKER_SCRIPT ${{PROJECT_ROOT}}/{linker_items[0] if linker_items else 'bsp/stm32/linker_scripts/STM32F103XX_FLASH.ld'})",
        "",
        "set(CPU_FLAGS",
    ]
    for flag in cpu_flag_items:
        lines.append(f"    {flag}")
    lines.extend([")", ""])

    group_vars: List[str] = []
    for group in executable_groups:
        files = rel_list(source_groups.get(group, []))
        if not files and group != "DRIVER_SRCS":
            continue
        group_vars.append(group)
        lines.append(f"set({group}")
        for item in files:
            lines.append(f"    ${{PROJECT_ROOT}}/{item}")
        lines.extend([")", ""])

    lines.extend(
        [
            "add_executable(${PROJECT_NAME}.elf",
        ]
    )
    for group in group_vars:
        lines.append(f"    ${{{group}}}")
    for item in startup:
        lines.append(f"    ${{PROJECT_ROOT}}/{item}")
    lines.extend(
        [
            ")",
            "",
            "set_target_properties(${PROJECT_NAME}.elf PROPERTIES",
            "    C_STANDARD 99",
            "    C_STANDARD_REQUIRED ON",
            '    SUFFIX ""',
            ")",
            "",
            "target_include_directories(${PROJECT_NAME}.elf PRIVATE",
        ]
    )
    for item in include_rel:
        lines.append(f"    ${{PROJECT_ROOT}}/{item}")
    lines.extend([")", "", "target_compile_definitions(${PROJECT_NAME}.elf PRIVATE"])
    for item in compile_defs:
        lines.append(f"    {item}")
    lines.extend(
        [
            ")",
            "",
            "target_compile_options(${PROJECT_NAME}.elf PRIVATE",
            "    ${CPU_FLAGS}",
            "    $<$<COMPILE_LANGUAGE:C>:-Wall>",
            "    $<$<COMPILE_LANGUAGE:C>:-Wextra>",
            "    $<$<COMPILE_LANGUAGE:C>:-Os>",
            "    $<$<COMPILE_LANGUAGE:C>:-ffunction-sections>",
            "    $<$<COMPILE_LANGUAGE:C>:-fdata-sections>",
            "    $<$<COMPILE_LANGUAGE:ASM>:-x>",
            "    $<$<COMPILE_LANGUAGE:ASM>:assembler-with-cpp>",
            ")",
            link_libs,
            "",
            "target_link_options(${PROJECT_NAME}.elf PRIVATE",
            "    ${CPU_FLAGS}",
            "    -T${LINKER_SCRIPT}",
            "    -Wl,--gc-sections",
            "    -Wl,-Map=${PROJECT_NAME}.map",
            "    -Wl,--print-memory-usage",
            link_extras,
            ")",
            "",
            "add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD",
            "    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin",
            "    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>",
            "    BYPRODUCTS ${PROJECT_NAME}.bin ${PROJECT_NAME}.map",
            ")",
            "",
        ]
    )
    return "\n".join(lines)


def copy_export_extras(
    project_dir: Path,
    export_dir: Path,
    extras: List[str],
    manifest_lines: List[str],
) -> None:
    if not extras:
        return

    manifest_lines.extend(["", "[extras]"])
    for extra in extras:
        src = project_dir / extra
        if not src.exists():
            raise FileNotFoundError(f"附加文件不存在: {src}")
        dst = export_dir / extra
        copy_file(src, dst)
        manifest_lines.append(extra.replace("\\", "/"))


def copy_any_path(src: Path, dst: Path) -> None:
    if src.is_dir():
        shutil.copytree(src, dst, dirs_exist_ok=True)
    else:
        copy_file(src, dst)


def copy_if_exists(src: Path, dst: Path) -> None:
    if src.exists():
        copy_any_path(src, dst)


def load_upper_version_config(upper_src: Path) -> Optional[Dict]:
    config_path = upper_src / "version_features.json"
    if not config_path.exists():
        return None
    return json.loads(config_path.read_text(encoding="utf-8"))


def resolve_version_feature_list(config: Dict, version: Optional[int]) -> List[str]:
    versions_cfg = config.get("versions", {})
    key = str(version) if version is not None else "default"
    entry = versions_cfg.get(key)
    if not entry and "default" in versions_cfg:
        entry = versions_cfg["default"]
    if entry and "features" in entry:
        features = entry["features"]
    else:
        features = config.get("defaultFeatures", [])
    seen: List[str] = []
    for item in features:
        if item not in seen:
            seen.append(item)
    return seen


def has_explicit_upper_version(config: Dict, version: Optional[int]) -> bool:
    versions_cfg = config.get("versions", {})
    key = str(version) if version is not None else "default"
    return key in versions_cfg


def is_flutter_upper_feature_set(features: List[str]) -> bool:
    if "mpWeixin" not in features:
        return True
    return any(feature in features for feature in ("app", "web"))


def collect_upper_feature_paths(
    upper_src: Path,
    config: Dict,
    features: List[str],
) -> List[Path]:
    patterns: List[str] = list(config.get("alwaysInclude", []))
    feature_globs = config.get("featureGlobs", {})
    for feature in features:
        patterns.extend(feature_globs.get(feature, []))

    includes: Set[Path] = set()
    excludes: Set[Path] = set()

    def handle_pattern(pattern: str, target: Set[Path]) -> None:
        negate = pattern.startswith("!")
        raw = pattern[1:] if negate else pattern
        matches = list(upper_src.glob(raw))
        if not matches and not any(ch in raw for ch in "*?[]"):
            candidate = upper_src / raw
            if candidate.exists():
                matches = [candidate]
        for match in matches:
            try:
                resolved = match.resolve()
            except FileNotFoundError:
                continue
            target.add(resolved)

    for pattern in patterns:
        if pattern.startswith("!"):
            handle_pattern(pattern, excludes)
        else:
            handle_pattern(pattern, includes)

    return sorted(includes - excludes)


def copy_relative_paths(paths: List[Path], source_root: Path, dest_root: Path) -> None:
    for path in paths:
        try:
            rel = path.relative_to(source_root)
        except ValueError:
            continue
        dst = dest_root / rel
        if path.is_dir():
            shutil.copytree(path, dst, dirs_exist_ok=True)
        else:
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, dst)


def npm_command(*args: str) -> List[str]:
    if os.name == "nt":
        return ["cmd", "/c", "npm", *args]
    return ["npm", *args]


def flutter_command(*args: str) -> List[str]:
    if os.name == "nt":
        return ["cmd", "/c", "flutter", *args]
    return ["flutter", *args]


def run_upper_builds(upper_src: Path, version: Optional[int], features: List[str]) -> None:
    dist_dir = upper_src / "dist"
    if dist_dir.exists():
        shutil.rmtree(dist_dir)

    env = os.environ.copy()
    env["UPPER_VERSION"] = str(version if version is not None else 0)
    env["UPPER_FEATURES"] = ",".join(features)

    subprocess.run(npm_command("run", "build:h5"), cwd=upper_src, check=True, env=env)
    subprocess.run(npm_command("run", "build:mp-weixin"), cwd=upper_src, check=True, env=env)


def run_upper_mp_weixin_build(upper_src: Path, version: Optional[int], features: List[str]) -> None:
    dist_dir = upper_src / "dist" / "build" / "mp-weixin"
    if dist_dir.exists():
        shutil.rmtree(dist_dir)

    env = os.environ.copy()
    env["UPPER_VERSION"] = str(version if version is not None else 0)
    env["UPPER_FEATURES"] = ",".join(features)
    subprocess.run(npm_command("run", "build:mp-weixin"), cwd=upper_src, check=True, env=env)


def run_flutter_upper_builds(flutter_src: Path, version: Optional[int], features: List[str]) -> None:
    build_dir = flutter_src / "build"
    if build_dir.exists():
        shutil.rmtree(build_dir)

    dart_defines = [
        f"--dart-define=UPPER_VERSION={version if version is not None else 0}",
        f"--dart-define=UPPER_FEATURES={','.join(features)}",
    ]
    subprocess.run(flutter_command("pub", "get"), cwd=flutter_src, check=True)
    subprocess.run(flutter_command("build", "apk", "--debug", *dart_defines), cwd=flutter_src, check=True)
    if features and "web" in features:
        subprocess.run(flutter_command("build", "web", *dart_defines), cwd=flutter_src, check=True)


def copy_upper_build_outputs(upper_src: Path, version_dir: Path) -> None:
    dist_src = upper_src / "dist"
    if not dist_src.exists():
        return
    dist_dst = version_dir / "dist"
    if dist_dst.exists():
        shutil.rmtree(dist_dst)
    shutil.copytree(dist_src, dist_dst)


def export_single_upper_version(
    upper_src: Path,
    base_dst: Path,
    config: Dict,
    version: Optional[int],
) -> None:
    label = f"version{version}" if version not in (None, 0) else "default"
    version_dir = base_dst / label
    if version_dir.exists():
        shutil.rmtree(version_dir)
    version_dir.mkdir(parents=True, exist_ok=True)

    features = resolve_version_feature_list(config, version)
    run_upper_builds(upper_src, version, features)
    paths = collect_upper_feature_paths(upper_src, config, features)
    copy_relative_paths(paths, upper_src, version_dir)
    copy_upper_build_outputs(upper_src, version_dir)


def export_single_upper_mp_weixin_version(
    upper_src: Path,
    base_dst: Path,
    config: Dict,
    version: Optional[int],
) -> None:
    label = f"version{version}" if version not in (None, 0) else "default"
    version_dir = base_dst / label
    if version_dir.exists():
        shutil.rmtree(version_dir)
    version_dir.mkdir(parents=True, exist_ok=True)

    features = resolve_version_feature_list(config, version)
    if "mpWeixin" not in features:
        return
    run_upper_mp_weixin_build(upper_src, version, features)
    paths = collect_upper_feature_paths(upper_src, config, features)
    copy_relative_paths(paths, upper_src, version_dir)
    copy_upper_build_outputs(upper_src, version_dir)


def export_single_flutter_upper_version(
    flutter_src: Path,
    base_dst: Path,
    config: Dict,
    version: Optional[int],
) -> None:
    label = f"version{version}" if version not in (None, 0) else "default"
    version_dir = base_dst / label
    if version_dir.exists():
        shutil.rmtree(version_dir)
    version_dir.mkdir(parents=True, exist_ok=True)

    features = resolve_version_feature_list(config, version)
    run_flutter_upper_builds(flutter_src, version, features)
    ignored = shutil.ignore_patterns(".dart_tool", "build", ".git", ".idea")
    shutil.copytree(flutter_src, version_dir, dirs_exist_ok=True, ignore=ignored)
    build_dst = version_dir / "build_outputs"
    copy_if_exists(flutter_src / "build" / "app" / "outputs" / "flutter-apk", build_dst / "flutter-apk")
    if "web" in features:
        copy_if_exists(flutter_src / "build" / "web", build_dst / "web")


def export_flutter_upper_ui_versions(
    flutter_src: Path,
    project_export_root: Path,
    versions: List[int],
    clean: bool,
) -> None:
    flutter_dst = project_export_root / flutter_src.name
    if clean and flutter_dst.exists():
        shutil.rmtree(flutter_dst)
    flutter_dst.mkdir(parents=True, exist_ok=True)
    versions_dst = flutter_dst / "versions"
    versions_dst.mkdir(parents=True, exist_ok=True)
    config = load_upper_version_config(flutter_src) or {
        "defaultFeatures": ["common"],
        "versions": {"default": {"features": ["common"]}},
    }
    copy_if_exists(flutter_src / "version_features.json", flutter_dst / "version_features.json")
    for ver in versions:
        actual_version = ver if ver > 0 else None
        export_single_flutter_upper_version(flutter_src, versions_dst, config, actual_version)


def export_upper_ui_versions(
    project_dir: Path,
    project_export_root: Path,
    versions: List[int],
    clean: bool,
) -> None:
    project_name = project_dir.name
    upper_src = project_dir / f"{project_name}_upper_ui"
    flutter_src = project_dir / f"{project_name}_upper_ui_flutter"

    if flutter_src.exists() and upper_src.exists():
        export_dual_stack_upper_ui_versions(project_dir, project_export_root, versions, clean)
        return

    if flutter_src.exists():
        export_flutter_upper_ui_versions(flutter_src, project_export_root, versions, clean)
        return

    if not upper_src.exists():
        return

    config = load_upper_version_config(upper_src)
    upper_dst = project_export_root / upper_src.name
    if clean and upper_dst.exists():
        shutil.rmtree(upper_dst)
    upper_dst.mkdir(parents=True, exist_ok=True)

    copy_if_exists(upper_src / "capacitor.config.json", upper_dst / "capacitor.config.json")
    copy_if_exists(upper_src / "android", upper_dst / "android")

    if not config:
        shutil.copytree(upper_src, upper_dst, dirs_exist_ok=True)
        return

    copy_file(upper_src / "version_features.json", upper_dst / "version_features.json")

    versions_dst = upper_dst / "versions"
    versions_dst.mkdir(parents=True, exist_ok=True)

    for ver in versions:
        actual_version = ver if ver > 0 else None
        export_single_upper_version(upper_src, versions_dst, config, actual_version)


def export_dual_stack_upper_ui_versions(
    project_dir: Path,
    project_export_root: Path,
    versions: List[int],
    clean: bool,
) -> None:
    project_name = project_dir.name
    upper_src = project_dir / f"{project_name}_upper_ui"
    flutter_src = project_dir / f"{project_name}_upper_ui_flutter"
    flutter_config = load_upper_version_config(flutter_src)
    uniapp_config = load_upper_version_config(upper_src)
    fallback_config = flutter_config or uniapp_config or {
        "defaultFeatures": ["common"],
        "versions": {"default": {"features": ["common"]}},
    }
    flutter_export_config = flutter_config or fallback_config
    uniapp_export_config = uniapp_config or fallback_config

    flutter_dst = project_export_root / flutter_src.name
    if clean and flutter_dst.exists():
        shutil.rmtree(flutter_dst)
    flutter_dst.mkdir(parents=True, exist_ok=True)
    copy_if_exists(flutter_src / "version_features.json", flutter_dst / "version_features.json")
    flutter_versions_dst = flutter_dst / "versions"
    flutter_versions_dst.mkdir(parents=True, exist_ok=True)
    for ver in versions:
        actual_version = ver if ver > 0 else None
        features = resolve_version_feature_list(flutter_export_config, actual_version)
        if flutter_config and not has_explicit_upper_version(flutter_export_config, actual_version):
            continue
        if not flutter_config and not is_flutter_upper_feature_set(features):
            continue
        export_single_flutter_upper_version(
            flutter_src,
            flutter_versions_dst,
            flutter_export_config,
            actual_version,
        )

    mp_weixin_versions = [
        ver
        for ver in versions
        if "mpWeixin" in resolve_version_feature_list(uniapp_export_config, ver if ver > 0 else None)
    ]
    if not mp_weixin_versions:
        return

    upper_dst = project_export_root / upper_src.name
    if clean and upper_dst.exists():
        shutil.rmtree(upper_dst)
    upper_dst.mkdir(parents=True, exist_ok=True)
    copy_file(upper_src / "version_features.json", upper_dst / "version_features.json")
    versions_dst = upper_dst / "versions"
    versions_dst.mkdir(parents=True, exist_ok=True)
    for ver in mp_weixin_versions:
        export_single_upper_mp_weixin_version(
            upper_src,
            versions_dst,
            uniapp_export_config,
            ver if ver > 0 else None,
        )


def export_one_version(
    project_dir: Path,
    template_root: Path,
    project_export_root: Path,
    version: Optional[int],
    extras: List[str],
    toolchain_bin: str,
    prune_conditionals: bool,
) -> Path:
    cmake_path = project_dir / "CMakeLists.txt"
    cmake_text = read_text(cmake_path)
    project_name = parse_project_name(cmake_text)
    version_var = parse_version_var(cmake_text)

    if version is None:
        version = parse_version_default(cmake_text, version_var) if version_var else 0

    source_groups, hal_options = resolve_source_lists(
        cmake_text, template_root, project_dir, version, version_var
    )
    executable_groups = parse_executable_groups(cmake_text)
    include_dirs = collect_include_dirs(cmake_text, template_root, project_dir)
    prune_ctx = build_export_prune_context(
        project_dir,
        cmake_text,
        version_var,
        version,
        hal_options,
        source_groups.get("DRIVER_SRCS", []),
        prune_conditionals,
    )

    if version_var and version > 0:
        export_name = f"{project_name}_version{version}"
    else:
        export_name = project_name

    project_export_root.mkdir(parents=True, exist_ok=True)
    export_dir = project_export_root / export_name
    if export_dir.exists():
        shutil.rmtree(export_dir)
    export_dir.mkdir(parents=True, exist_ok=True)

    all_sources: List[Path] = []
    for key, paths in source_groups.items():
        if key == "LINKER_SCRIPT":
            continue
        all_sources.extend(paths)

    copied: Set[Path] = set()
    manifest_lines = [
        f"project={project_name}",
        f"export_dir={export_dir}",
        f"version={version}",
        f"version_var={version_var or ''}",
        f"driver_count={len(source_groups.get('DRIVER_SRCS', []))}",
        f"source_groups={','.join(executable_groups)}",
        "",
        "[sources]",
    ]

    for src in all_sources:
        if not src.exists():
            raise FileNotFoundError(f"源文件不存在: {src}")
        rel = rel_export_path(src, template_root, project_dir)
        dst = export_dir / rel
        copy_export_file(src, dst, version_var, version, prune_ctx, template_root, project_dir)
        copied.add(dst.resolve())
        manifest_lines.append(rel.as_posix())

    for linker in source_groups.get("LINKER_SCRIPT", []):
        rel = rel_export_path(linker, template_root, project_dir)
        dst = export_dir / rel
        copy_file(linker, dst)
        manifest_lines.append(rel.as_posix())

    header_files = collect_header_files(include_dirs)
    header_files.update(driver_local_headers(source_groups.get("DRIVER_SRCS", [])))
    manifest_lines.extend(["", "[headers]"])
    for header in sorted(header_files):
        rel = rel_export_path(header, template_root, project_dir)
        dst = export_dir / rel
        if dst.resolve() not in copied:
            copy_export_file(header, dst, version_var, version, prune_ctx, template_root, project_dir)
            copied.add(dst.resolve())
        manifest_lines.append(rel.as_posix())

    copy_export_extras(project_dir, export_dir, extras, manifest_lines)

    toolchain_src = template_root / "cmake" / "stm32-gcc-toolchain.cmake"
    if toolchain_src.exists():
        copy_file(toolchain_src, export_dir / "cmake" / "stm32-gcc-toolchain.cmake")

    for project_subdir in ("board", "app"):
        for project_file in (project_dir / project_subdir).glob("*"):
            if not project_file.is_file():
                continue
            rel = Path(project_subdir) / project_file.name
            dst = export_dir / rel
            if dst.resolve() in copied:
                continue
            copy_export_file(project_file, dst, version_var, version, prune_ctx, template_root, project_dir)
            copied.add(dst.resolve())
            manifest_lines.append(rel.as_posix())

    standalone_cmake = generate_standalone_cmake(
        project_name,
        source_groups,
        executable_groups,
        include_dirs,
        template_root,
        project_dir,
        hal_options,
        version_var,
        version,
        cmake_text,
        toolchain_bin,
    )
    write_text(export_dir / "CMakeLists.txt", standalone_cmake)

    readme = export_dir / "readme.txt"
    if not readme.exists():
        write_text(
            readme,
            f"{export_name} 导出版本\n\n"
            f"由 export_project.py 自 {project_name} 导出。\n"
            f"版本: {version if version_var else 'default'}\n\n"
            "构建:\n"
            "  cmake -S . -B build -G Ninja "
            "-DCMAKE_TOOLCHAIN_FILE=cmake/stm32-gcc-toolchain.cmake\n"
            "  cmake --build build\n",
        )

    append_prune_manifest(manifest_lines, prune_ctx)
    write_text(export_dir / "export_manifest.txt", "\n".join(manifest_lines) + "\n")
    return export_dir


def export_project_versions(
    project_dir: Path,
    template_root: Path,
    output_root: Path,
    versions: List[int],
    extras: List[str],
    toolchain_bin: str,
    clean: bool,
    prune_conditionals: bool,
    layout: str = "legacy",
    export_root_name: Optional[str] = None,
    firmware_only: bool = False,
    log: Callable[[str], None] = print,
) -> None:
    if layout == "legacy":
        project_export_root = output_root / project_dir.name
        firmware_export_root = project_export_root
        upper_export_root = project_export_root
        clean_root = project_export_root
    elif layout == "gui_package":
        project_export_root = output_root / (export_root_name or f"export_{project_dir.name}")
        firmware_export_root = project_export_root / "firmware"
        upper_export_root = project_export_root / "upper_ui"
        clean_root = project_export_root
    else:
        raise ValueError(f"未知导出布局: {layout}")

    if clean and clean_root.exists():
        shutil.rmtree(clean_root)

    for ver in versions:
        actual_version = ver if ver > 0 else None
        out = export_one_version(
            project_dir,
            template_root,
            firmware_export_root,
            actual_version,
            extras,
            toolchain_bin,
            prune_conditionals,
        )
        log(f"已导出下位机: {out}")

    if not firmware_only:
        export_upper_ui_versions(project_dir, upper_export_root, versions, clean=False)
    log(f"导出完成: {project_export_root}")


def get_project_export_info(project_dir: Path, template_root: Path) -> ProjectExportInfo:
    del template_root
    cmake_text = read_text(project_dir / "CMakeLists.txt")
    project_name = parse_project_name(cmake_text)
    version_var = parse_version_var(cmake_text)
    default_version: Optional[int] = None
    version_range: Optional[Tuple[int, int]] = None
    available_versions: List[int] = [0]
    if version_var:
        default_version = parse_version_default(cmake_text, version_var)
        version_range = parse_version_range(cmake_text, version_var)
        available_versions = list(range(version_range[0], version_range[1] + 1))
    return ProjectExportInfo(
        project_dir=project_dir,
        project_name=project_name,
        version_var=version_var,
        default_version=default_version,
        version_range=version_range,
        available_versions=available_versions,
        has_flutter_upper_ui=(project_dir / f"{project_dir.name}_upper_ui_flutter").exists(),
        has_uniapp_upper_ui=(project_dir / f"{project_dir.name}_upper_ui").exists(),
    )


def discover_projects(template_root: Path) -> List[Path]:
    projects_dir = template_root / "projects"
    if not projects_dir.exists():
        return []
    return sorted(p for p in projects_dir.iterdir() if (p / "CMakeLists.txt").exists())


def parse_versions(text: str) -> List[int]:
    text = text.strip()
    if "-" in text:
        start, end = text.split("-", 1)
        return list(range(int(start), int(end) + 1))
    return [int(v.strip()) for v in text.split(",") if v.strip()]


def interactive_select(template_root: Path) -> Tuple[Path, List[int], Path, List[str], str]:
    projects = discover_projects(template_root)
    if not projects:
        raise SystemExit("projects 目录下未找到工程")

    print("可选工程：")
    for idx, project in enumerate(projects, start=1):
        print(f"  [{idx}] {project.name}")

    choice = input("选择工程编号: ").strip()
    project = projects[int(choice) - 1]
    cmake_text = read_text(project / "CMakeLists.txt")
    version_var = parse_version_var(cmake_text)

    versions: List[int] = [0]

    if version_var:
        batch_answer = input("是否批量导出全部版本? [y/N]: ").strip().lower()
        batch = batch_answer in {"y", "yes", "1"}
        if batch:
            default_range = parse_version_range(cmake_text, version_var)
            range_text = input(
                f"版本范围 [{default_range[0]}-{default_range[1]}]: "
            ).strip()
            versions = (
                parse_versions(range_text)
                if range_text
                else list(range(default_range[0], default_range[1] + 1))
            )
        else:
            versions = [int(input("输入版本号: ").strip())]
    else:
        print("该工程无版本变量，将导出单版本。")
        versions = [0]

    output = input("输出目录 [../exports]: ").strip()
    output_root = Path(output) if output else template_root.parent / "exports"

    extras_text = input("附加文件（逗号分隔，留空跳过）: ").strip()
    extras = parse_extras_list([extras_text] if extras_text else None, no_extras=False)

    toolchain_text = input(
        f"工具链 bin 目录 [环境变量/STM32_TOOLCHAIN_BIN 或工程 CMakeLists]: "
    ).strip()
    toolchain_bin = resolve_toolchain_bin(
        cmake_text,
        toolchain_text or None,
    )
    return project, versions, output_root, extras, toolchain_bin


def main() -> None:
    parser = argparse.ArgumentParser(description="按版本导出 project_template 工程为独立目录")
    parser.add_argument("--project", help="工程名或工程目录，如 RTJK_001")
    parser.add_argument("--cmake", type=Path, help="CMakeLists.txt 路径")
    parser.add_argument("--version", type=int, help="导出指定版本")
    parser.add_argument("--batch", action="store_true", help="批量导出多个版本")
    parser.add_argument(
        "--versions",
        default="",
        help="批量版本范围，如 1-10 或 1,3,7；默认从 CMakeLists 读取",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=None,
        help="输出根目录，默认 <repo>/exports",
    )
    parser.add_argument("--clean", action="store_true", help="导出前清空目标目录")
    parser.add_argument(
        "--extras",
        nargs="*",
        default=None,
        metavar="FILE",
        help="附加复制到导出目录的文件（可多次或逗号分隔，相对工程目录）",
    )
    parser.add_argument("--no-extras", action="store_true", help="不复制任何附加文件")
    parser.add_argument(
        "--toolchain-bin",
        default=None,
        help="覆盖 STM32 工具链 bin 目录；亦可用环境变量 STM32_TOOLCHAIN_BIN",
    )
    parser.add_argument(
        "--no-prune-conditionals",
        action="store_true",
        help="不裁剪导出源码中的已知未启用条件编译分支",
    )
    parser.add_argument(
        "--firmware-only",
        action="store_true",
        help="只导出下位机固件，不构建/导出 Flutter 或 uni-app 上位机",
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="project_template 根目录",
    )
    args = parser.parse_args()

    template_root = find_template_root(args.root)
    extras = parse_extras_list(args.extras, args.no_extras)

    if args.cmake:
        project_dir = args.cmake.parent
    elif args.project:
        candidate = Path(args.project)
        project_dir = candidate if candidate.is_dir() else template_root / "projects" / args.project
    else:
        project_dir, versions, output_root, extras, toolchain_bin = interactive_select(
            template_root
        )
        export_project_versions(
            project_dir,
            template_root,
            output_root,
            versions,
            extras,
            toolchain_bin,
            args.clean,
            not args.no_prune_conditionals,
            firmware_only=args.firmware_only,
        )
        return

    if not project_dir.exists():
        raise SystemExit(f"工程不存在: {project_dir}")

    output_root = args.output or template_root.parent / "exports"
    cmake_text = read_text(project_dir / "CMakeLists.txt")
    version_var = parse_version_var(cmake_text)
    toolchain_bin = resolve_toolchain_bin(cmake_text, args.toolchain_bin)

    if args.batch:
        if not version_var:
            raise SystemExit("该工程不支持 --batch（CMakeLists 中无 *VERSION CACHE STRING 变量）")
        if args.versions:
            versions = parse_versions(args.versions)
        else:
            lo, hi = parse_version_range(cmake_text, version_var)
            versions = list(range(lo, hi + 1))
    elif args.version is not None:
        versions = [args.version]
    elif version_var:
        versions = [parse_version_default(cmake_text, version_var)]
    else:
        versions = [0]

    export_project_versions(
        project_dir,
        template_root,
        output_root,
        versions,
        extras,
        toolchain_bin,
        args.clean,
        not args.no_prune_conditionals,
        firmware_only=args.firmware_only,
    )


if __name__ == "__main__":
    main()
