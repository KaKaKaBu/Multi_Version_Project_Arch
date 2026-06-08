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
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Set, Tuple

TEMPLATE_ROOT_NAME = "project_template"
DEFAULT_STM32_TOOLCHAIN_BIN = (
    "D:/ProgramFile/ST/STM32CubeCLT_1.20.0/GNU-tools-for-STM32/bin"
)


def find_template_root(start: Path) -> Path:
    for parent in [start, *start.parents]:
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
    candidates = [
        shutil.which("cmake"),
        r"D:/ProgramFile/ST/STM32CubeCLT_1.20.0/CMake/bin/cmake.exe",
    ]
    for candidate in candidates:
        if candidate and Path(candidate).exists():
            return candidate
    raise FileNotFoundError("未找到 cmake，请安装 CMake 或将其加入 PATH")


def parse_project_name(cmake_text: str) -> str:
    match = re.search(r"project\s*\(\s*(\w+)", cmake_text)
    if not match:
        raise ValueError("CMakeLists.txt 中未找到 project(...) 名称")
    return match.group(1)


def parse_set_block(cmake_text: str, var_name: str) -> List[str]:
    single = re.search(
        rf"set\s*\(\s*{re.escape(var_name)}\s+([^)\n]+)\)",
        cmake_text,
    )
    if single:
        body = single.group(1).strip()
        if body:
            return [body]

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
        items.append(line)
    return items


def parse_driver_catalog_ref(items: List[str]) -> Optional[str]:
    for item in items:
        match = re.search(r"\$\{DRIVER_CATALOG_(\w+)\}", item.strip())
        if match:
            return match.group(1)
    return None


def parse_version_var(cmake_text: str) -> Optional[str]:
    match = re.search(r"set\s*\(\s*(\w+VERSION)\s+\d+\s+CACHE\s+STRING", cmake_text)
    if match:
        return match.group(1)
    return None


def parse_version_default(cmake_text: str, version_var: str) -> int:
    match = re.search(
        rf"set\s*\(\s*{re.escape(version_var)}\s+(\d+)\s+CACHE\s+STRING",
        cmake_text,
    )
    if match:
        return int(match.group(1))
    return 1


def parse_version_range(cmake_text: str, version_var: str) -> Tuple[int, int]:
    pattern = rf"{re.escape(version_var)} must be (\d+)-(\d+)"
    match = re.search(pattern, cmake_text)
    if match:
        return int(match.group(1)), int(match.group(2))
    default = parse_version_default(cmake_text, version_var)
    return default, default


def parse_executable_groups(cmake_text: str) -> List[str]:
    match = re.search(
        r"add_executable\s*\(\s*\$\{PROJECT_NAME\}\.elf\s+(.*?)\n\)",
        cmake_text,
        re.DOTALL,
    )
    if not match:
        return ["APP_SRCS", "COMMON_SRCS", "BSP_SRCS", "DRIVER_SRCS", "SPL_SRCS"]

    groups: List[str] = []
    for line in match.group(1).splitlines():
        ref = re.search(r"\$\{(\w+)\}", line.strip())
        if ref:
            groups.append(ref.group(1))
    return groups


def parse_target_compile_definitions(cmake_text: str) -> List[str]:
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
    return DEFAULT_STM32_TOOLCHAIN_BIN


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

    if item.startswith("app/"):
        return [project_dir / item]

    path = Path(item)
    if path.is_absolute():
        return [path]
    return [project_dir / item]


def resolve_source_lists(
    cmake_text: str,
    template_root: Path,
    project_dir: Path,
    version: int,
    version_var: Optional[str],
) -> Tuple[Dict[str, List[Path]], Dict[str, bool]]:
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
    block_match = re.search(
        r"target_include_directories\s*\(\s*\$\{PROJECT_NAME\}\.elf\s+PRIVATE\s+(.*?)\n\)",
        cmake_text,
        re.DOTALL,
    )
    includes: List[Path] = [project_dir / "app"]
    if not block_match:
        return includes

    for line in block_match.group(1).splitlines():
        line = line.strip()
        if not line or line == "app":
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


def patch_version_header(text: str, version_var: Optional[str], version: int) -> str:
    if not version_var:
        return text
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


def rel_export_path(path: Path, template_root: Path, project_dir: Path) -> Path:
    resolved = path.resolve()
    template_root = template_root.resolve()
    project_dir = project_dir.resolve()
    app_dir = project_dir / "app"
    try:
        return Path("app") / resolved.relative_to(app_dir.resolve())
    except ValueError:
        pass
    try:
        return resolved.relative_to(template_root)
    except ValueError:
        return resolved.relative_to(project_dir)


def copy_app_file(src: Path, dst: Path, version_var: Optional[str], version: int) -> None:
    if src.suffix.lower() in {".h", ".c"}:
        content = patch_version_in_app_files(src.name, read_text(src), version_var, version)
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
    compile_defs.extend(hal_compile_definitions(hal_options))

    cpu_flag_items = parse_set_block(src_cmake_text, "CPU_FLAGS")
    if not cpu_flag_items:
        cpu_flag_items = ["-mcpu=cortex-m3", "-mthumb"]

    link_libs = ""
    if "target_link_libraries" in src_cmake_text and "PRIVATE m" in src_cmake_text:
        link_libs = "\ntarget_link_libraries(${PROJECT_NAME}.elf PRIVATE m)"

    link_extras = ""
    for sym in parse_link_undef_symbols(src_cmake_text):
        link_extras += f"\n    -Wl,-u,{sym}"

    lines: List[str] = [
        "cmake_minimum_required(VERSION 3.20)",
        "",
        "set(CMAKE_SYSTEM_NAME Generic)",
        "set(CMAKE_SYSTEM_PROCESSOR arm)",
        "set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)",
        "",
        f"project({project_name} C ASM)",
        "",
        "set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR})",
        "",
        f'set(STM32_TOOLCHAIN_BIN "{toolchain_bin}" CACHE PATH "STM32 GNU toolchain bin directory")',
        f"set(LINKER_SCRIPT ${{PROJECT_ROOT}}/{linker_items[0] if linker_items else 'bsp/linker_scripts/STM32F103XX_FLASH.ld'})",
        "",
        "if(NOT CMAKE_OBJCOPY)",
        "    set(CMAKE_OBJCOPY ${STM32_TOOLCHAIN_BIN}/arm-none-eabi-objcopy.exe)",
        "endif()",
        "",
        "if(NOT CMAKE_SIZE)",
        "    set(CMAKE_SIZE ${STM32_TOOLCHAIN_BIN}/arm-none-eabi-size.exe)",
        "endif()",
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


def run_upper_builds(upper_src: Path, version: Optional[int], features: List[str]) -> None:
    dist_dir = upper_src / "dist"
    if dist_dir.exists():
        shutil.rmtree(dist_dir)

    env = os.environ.copy()
    env["UPPER_VERSION"] = str(version if version is not None else 0)
    env["UPPER_FEATURES"] = ",".join(features)

    subprocess.run(["cmd", "/c", "npm", "run", "build:h5"], cwd=upper_src, check=True, env=env)
    subprocess.run(["cmd", "/c", "npm", "run", "build:mp-weixin"], cwd=upper_src, check=True, env=env)


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


def export_upper_ui_versions(
    project_dir: Path,
    project_export_root: Path,
    versions: List[int],
    clean: bool,
) -> None:
    project_name = project_dir.name
    upper_src = project_dir / f"{project_name}_upper_ui"
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


def export_one_version(
    project_dir: Path,
    template_root: Path,
    project_export_root: Path,
    version: Optional[int],
    extras: List[str],
    toolchain_bin: str,
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
        if is_app_path(src, project_dir):
            copy_app_file(src, dst, version_var, version)
        else:
            copy_file(src, dst)
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
            copy_file(header, dst)
            copied.add(dst.resolve())
        manifest_lines.append(rel.as_posix())

    copy_export_extras(project_dir, export_dir, extras, manifest_lines)

    toolchain_src = template_root / "cmake" / "stm32-gcc-toolchain.cmake"
    if toolchain_src.exists():
        copy_file(toolchain_src, export_dir / "cmake" / "stm32-gcc-toolchain.cmake")

    for app_file in (project_dir / "app").glob("*"):
        if not app_file.is_file():
            continue
        rel = Path("app") / app_file.name
        dst = export_dir / rel
        if dst.resolve() in copied:
            continue
        copy_app_file(app_file, dst, version_var, version)
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
            "-DCMAKE_TOOLCHAIN_FILE=cmake/stm32-gcc-toolchain.cmake "
            "-DCMAKE_MAKE_PROGRAM=<Ninja路径>/ninja.exe\n"
            "  cmake --build build\n",
        )

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
) -> None:
    project_export_root = output_root / project_dir.name
    if clean and project_export_root.exists():
        shutil.rmtree(project_export_root)

    for ver in versions:
        actual_version = ver if ver > 0 else None
        out = export_one_version(
            project_dir,
            template_root,
            project_export_root,
            actual_version,
            extras,
            toolchain_bin,
        )
        print(f"已导出: {out}")

    export_upper_ui_versions(project_dir, project_export_root, versions, clean)


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
    )


if __name__ == "__main__":
    main()
