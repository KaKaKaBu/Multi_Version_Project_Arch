#!/usr/bin/env python3
from __future__ import annotations

import os
import queue
import re
import shutil
import threading
import traceback
from pathlib import Path
from tkinter import END, NORMAL, DISABLED
from typing import Callable, Iterable, List, Optional

PROJECT_NAME_RE = re.compile(r"^[A-Za-z0-9_][A-Za-z0-9_.-]*$")
ANDROID_PACKAGE_RE = re.compile(r"^[A-Za-z][A-Za-z0-9_]*(\.[A-Za-z][A-Za-z0-9_]*)+$")
WINDOWS_INVALID_CHARS = set('<>:"/\\|?*')


class GuiLogSink:
    def __init__(self, events: "queue.Queue[tuple[str, object]]"):
        self.events = events

    def __call__(self, message: str) -> None:
        self.events.put(("log", str(message)))


class ThreadedWorker:
    def __init__(self, events: "queue.Queue[tuple[str, object]]"):
        self.events = events
        self.thread: Optional[threading.Thread] = None

    def running(self) -> bool:
        return self.thread is not None and self.thread.is_alive()

    def start(self, func: Callable[[], object]) -> None:
        if self.running():
            raise RuntimeError("已有任务正在执行")

        def run() -> None:
            try:
                result = func()
            except Exception as exc:  # noqa: BLE001 - surface worker failures to GUI
                self.events.put(("error", f"{exc}\n\n{traceback.format_exc()}"))
            else:
                self.events.put(("done", result))

        self.thread = threading.Thread(target=run, daemon=True)
        self.thread.start()


def append_log(text_widget, message: str) -> None:
    text_widget.configure(state=NORMAL)
    text_widget.insert(END, message.rstrip() + "\n")
    text_widget.see(END)
    text_widget.configure(state=DISABLED)


def drain_events(root, events: "queue.Queue[tuple[str, object]]", on_log: Callable[[str], None], on_done: Callable[[object], None], on_error: Callable[[str], None]) -> None:
    try:
        while True:
            kind, payload = events.get_nowait()
            if kind == "log":
                on_log(str(payload))
            elif kind == "done":
                on_done(payload)
            elif kind == "error":
                on_error(str(payload))
    except queue.Empty:
        pass
    root.after(100, lambda: drain_events(root, events, on_log, on_done, on_error))


def validate_project_name(value: str) -> Optional[str]:
    if not value:
        return "工程名不能为空"
    if value in {".", ".."} or ".." in value:
        return "工程名不能包含 .."
    if any(char in WINDOWS_INVALID_CHARS for char in value):
        return "工程名包含非法路径字符"
    if not PROJECT_NAME_RE.match(value):
        return "工程名只能包含字母、数字、下划线、点和短横线"
    return None


def validate_android_package_name(value: str) -> Optional[str]:
    if not value:
        return "App 包名不能为空"
    if not ANDROID_PACKAGE_RE.match(value):
        return "App 包名格式应类似 com.company.app"
    return None


def command_exists(command: str) -> bool:
    return shutil.which(command) is not None


_FLUTTER_CANDIDATES = [
    os.path.expanduser("~/development/flutter/bin/flutter"),
    os.path.expanduser("~/flutter/bin/flutter"),
    "/usr/local/flutter/bin/flutter",
    "/opt/flutter/bin/flutter",
]


def find_flutter() -> Optional[str]:
    """Return the full path to the flutter executable, or None if not found."""
    path = shutil.which("flutter")
    if path:
        return path
    for candidate in _FLUTTER_CANDIDATES:
        if os.path.isfile(candidate) and os.access(candidate, os.X_OK):
            return candidate
    env_paths = [
        os.environ.get("FLUTTER_ROOT", ""),
        os.environ.get("FLUTTER_HOME", ""),
    ]
    for env_path in env_paths:
        if env_path:
            candidate = os.path.join(env_path, "bin", "flutter")
            if os.path.isfile(candidate) and os.access(candidate, os.X_OK):
                return candidate
    return None


def parse_comma_list(value: str) -> List[str]:
    return [item.strip() for item in value.split(",") if item.strip()]


def validate_relative_paths(base_dir: Path, items: Iterable[str]) -> Optional[str]:
    base_dir = base_dir.resolve()
    for item in items:
        path = Path(item)
        if path.is_absolute():
            return f"附加文件必须是相对路径: {item}"
        resolved = (base_dir / path).resolve()
        try:
            resolved.relative_to(base_dir)
        except ValueError:
            return f"附加文件不能指向工程外部: {item}"
        if not resolved.exists():
            return f"附加文件不存在: {item}"
    return None


def open_path(path: Path) -> None:
    if os.name == "nt":
        os.startfile(path)  # type: ignore[attr-defined]
        return
    opener = shutil.which("xdg-open") or shutil.which("open")
    if opener:
        import subprocess

        subprocess.Popen([opener, str(path)])
