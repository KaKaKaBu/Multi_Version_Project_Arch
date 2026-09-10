"""Environment-controlled host tool resolution used by tools/*."""

from __future__ import annotations

import os
from pathlib import Path


def _existing_file(value: str) -> str | None:
    if not value:
        return None
    path = Path(value).expanduser()
    return str(path) if path.is_file() else None


def resolve_cmake_executable() -> str:
    """Resolve CMake only from explicit environment variables.

    CMAKE_EXE is preferred and should point directly to cmake.exe. No PATH
    lookup, fixed Windows path, or filesystem scan is performed.
    """
    for name in ("CMAKE_EXE", "CMAKE_EXECUTABLE"):
        value = os.environ.get(name, "").strip()
        resolved = _existing_file(value)
        if resolved:
            return resolved
        if value:
            raise FileNotFoundError(f"环境变量 {name} 指向的 CMake 文件不存在: {value}")

    for name in ("CMAKE_ROOT", "CMAKE_HOME", "CLION_HOME"):
        root_value = os.environ.get(name, "").strip()
        if not root_value:
            continue
        root = Path(root_value).expanduser()
        for candidate in (
            root / "cmake.exe",
            root / "bin" / "cmake.exe",
            root / "bin" / "cmake" / "win" / "x64" / "bin" / "cmake.exe",
        ):
            resolved = _existing_file(str(candidate))
            if resolved:
                return resolved
        raise FileNotFoundError(
            f"环境变量 {name} 未找到 CMake，可设置 CMAKE_EXE 为 cmake.exe 的完整路径: {root_value}"
        )

    raise FileNotFoundError(
        "未配置 CMake。请设置环境变量 CMAKE_EXE（cmake.exe 完整路径）；"
        "也兼容 CMAKE_EXECUTABLE、CMAKE_ROOT、CMAKE_HOME、CLION_HOME。"
    )
