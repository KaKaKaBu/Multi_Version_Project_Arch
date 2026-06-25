#!/usr/bin/env python3
from __future__ import annotations

import ctypes
import json
import os
import queue
import re
import shlex
import shutil
import subprocess
import sys
import threading
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, scrolledtext, ttk
from typing import Callable, Dict, List, Optional, Sequence, Tuple, Union

try:
    from export_project import (
        discover_projects,
        get_project_export_info,
        load_upper_version_config,
        read_text,
        resolve_version_feature_list,
    )
    from gui_common import append_log, command_exists, drain_events, find_flutter, open_path
except ImportError:
    from tools.export_project import (
        discover_projects,
        get_project_export_info,
        load_upper_version_config,
        read_text,
        resolve_version_feature_list,
    )
    from tools.gui_common import append_log, command_exists, drain_events, find_flutter, open_path

TEMPLATE_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OPENOCD_CFG = str(TEMPLATE_ROOT / "stm32f103c8_blue_pill.cfg")
DEFAULT_ADB_TARGET = "127.0.0.1:5555"
DEFAULT_WINDOW_SIZE = "1120x820"
CommandArgs = Union[Sequence[str], Callable[[], Sequence[str]]]
CommandSpec = Tuple[CommandArgs, Path, Optional[Dict[str, str]]]


def enable_high_dpi_awareness() -> None:
    if os.name != "nt":
        return
    try:
        ctypes.windll.shcore.SetProcessDpiAwareness(1)
    except Exception:  # noqa: BLE001 - DPI APIs vary across Windows versions
        try:
            ctypes.windll.user32.SetProcessDPIAware()
        except Exception:
            pass


def configure_tk_scaling(root: tk.Tk) -> None:
    try:
        pixels_per_inch = root.winfo_fpixels("1i")
        root.tk.call("tk", "scaling", max(1.0, pixels_per_inch / 72.0))
    except tk.TclError:
        pass


class CommandWorker:
    def __init__(self, events: "queue.Queue[tuple[str, object]]"):
        self.events = events
        self.thread: Optional[threading.Thread] = None
        self.process: Optional[subprocess.Popen[str]] = None
        self._stop_requested = False

    def running(self) -> bool:
        return self.thread is not None and self.thread.is_alive()

    def start(self, title: str, commands: Sequence[CommandSpec]) -> None:
        if self.running():
            raise RuntimeError("已有任务正在执行")
        self._stop_requested = False

        def run() -> None:
            try:
                for args, cwd, env in commands:
                    if self._stop_requested:
                        raise RuntimeError("任务已取消")
                    resolved_args = args() if callable(args) else args
                    self._run_one(resolved_args, cwd, env)
            except RuntimeError as exc:
                if str(exc) == "任务已取消":
                    self.events.put(("done", f"{title} 已取消"))
                else:
                    self.events.put(("error", f"{title} 失败: {exc}"))
            except Exception as exc:  # noqa: BLE001 - GUI should surface worker errors
                self.events.put(("error", f"{title} 失败: {exc}"))
            else:
                self.events.put(("done", f"{title} 完成"))
            finally:
                self.process = None

        self.thread = threading.Thread(target=run, daemon=True)
        self.thread.start()

    def stop(self) -> None:
        self._stop_requested = True
        process = self.process
        if not process or process.poll() is not None:
            return
        if os.name == "nt":
            subprocess.run(
                ["taskkill", "/F", "/T", "/PID", str(process.pid)],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=False,
            )
        else:
            process.terminate()

    def _run_one(self, args: Sequence[str], cwd: Path, env: Optional[Dict[str, str]]) -> None:
        pretty = " ".join(shlex.quote(str(item)) for item in args)
        self.events.put(("log", f"$ {pretty}\n  cwd={cwd}"))
        merged_env = os.environ.copy()
        if env:
            merged_env.update(env)
        self.process = subprocess.Popen(
            list(args),
            cwd=str(cwd),
            env=merged_env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            bufsize=1,
        )
        assert self.process.stdout is not None
        for line in self.process.stdout:
            self.events.put(("log", line.rstrip()))
        returncode = self.process.wait()
        if self._stop_requested:
            raise RuntimeError("任务已取消")
        if returncode != 0:
            raise RuntimeError(f"命令退出码 {returncode}: {pretty}")


def command(*args: str) -> List[str]:
    if os.name == "nt":
        return ["cmd", "/c", *args]
    return list(args)


def parse_cmake_project_name(project_dir: Path) -> str:
    text = read_text(project_dir / "CMakeLists.txt")
    match = re.search(r"project\s*\(\s*(\w+)", text)
    return match.group(1) if match else project_dir.name


def find_firmware_binary(project_dir: Path, project_name: str, build_dir: Path) -> Optional[Path]:
    candidates = [
        build_dir / f"{project_name}.bin",
        build_dir / f"{project_name}.elf.bin",
        build_dir / f"{project_dir.name}.bin",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    matches = sorted(build_dir.rglob("*.bin"), key=lambda p: p.stat().st_mtime, reverse=True)
    return matches[0] if matches else None


def parse_device_rows(text: str) -> List[str]:
    rows = []
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("List of devices"):
            continue
        parts = line.split()
        if parts:
            rows.append(parts[0])
    return rows


def parse_flutter_devices_machine(output: str) -> List[tuple[str, str, str]]:
    """Parse `flutter devices --machine` JSON, return [(id, name, targetPlatform), ...]."""
    try:
        data = json.loads(output)
    except json.JSONDecodeError:
        return []
    result = []
    for device in data:
        device_id = device.get("id", "")
        name = device.get("name", device_id)
        platform = device.get("targetPlatform", "")
        if device.get("isSupported", False):
            result.append((device_id, name, platform))
    return result


class ProjectRunGui:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("项目烧录 / Flutter / ADB 工具")
        self.root.geometry(DEFAULT_WINDOW_SIZE)
        self.root.minsize(900, 640)
        self.events: "queue.Queue[tuple[str, object]]" = queue.Queue()
        self.worker = CommandWorker(self.events)
        self.projects: Dict[str, Path] = {}
        self.current_project_dir: Optional[Path] = None
        self.current_project_name = ""
        self.current_versions: List[int] = []
        self.last_firmware: Optional[Path] = None

        self.template_root_var = tk.StringVar(value=str(TEMPLATE_ROOT))
        self.project_var = tk.StringVar()
        self.version_var = tk.StringVar()
        self.version_info_var = tk.StringVar(value="未选择工程")
        self.adb_target_var = tk.StringVar(value=DEFAULT_ADB_TARGET)
        self.adb_status_var = tk.StringVar(value="ADB 状态：未知")
        self.device_var = tk.StringVar()
        self.flutter_target_var = tk.StringVar(value="android")
        self.build_dir_var = tk.StringVar(value="")
        self.openocd_cfg_var = tk.StringVar(value=DEFAULT_OPENOCD_CFG)
        self.openocd_scripts_var = tk.StringVar(value="")
        self.firmware_var = tk.StringVar(value="")

        self._build_ui()
        self._set_buttons(True)
        self._refresh_projects()
        drain_events(self.root, self.events, self._log, self._done, self._error)

    def _build_ui(self) -> None:
        outer = ttk.Frame(self.root, padding=10)
        outer.grid(row=0, column=0, sticky="nsew")
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        outer.columnconfigure(0, weight=1)
        outer.rowconfigure(0, weight=1)

        canvas = tk.Canvas(outer, highlightthickness=0)
        scrollbar = ttk.Scrollbar(outer, orient="vertical", command=canvas.yview)
        canvas.configure(yscrollcommand=scrollbar.set)
        canvas.grid(row=0, column=0, sticky="nsew")
        scrollbar.grid(row=0, column=1, sticky="ns")

        main = ttk.Frame(canvas, padding=2)
        window_id = canvas.create_window((0, 0), window=main, anchor="nw")

        def update_scroll_region(_event: tk.Event) -> None:
            canvas.configure(scrollregion=canvas.bbox("all"))

        def update_canvas_width(event: tk.Event) -> None:
            canvas.itemconfigure(window_id, width=event.width)

        def on_mousewheel(event: tk.Event) -> None:
            delta = -1 if getattr(event, "delta", 0) > 0 else 1
            canvas.yview_scroll(delta, "units")

        main.bind("<Configure>", update_scroll_region)
        canvas.bind("<Configure>", update_canvas_width)
        canvas.bind_all("<MouseWheel>", on_mousewheel)
        main.columnconfigure(1, weight=1)

        ttk.Label(main, text="project_template 根目录").grid(row=0, column=0, sticky="w", pady=4)
        ttk.Entry(main, textvariable=self.template_root_var).grid(row=0, column=1, sticky="ew", pady=4)
        ttk.Button(main, text="浏览", command=self._browse_root).grid(row=0, column=2, padx=(6, 0), pady=4)

        ttk.Label(main, text="ADB 地址").grid(row=1, column=0, sticky="w", pady=4)
        ttk.Entry(main, textvariable=self.adb_target_var).grid(row=1, column=1, sticky="ew", pady=4)
        adb_buttons = ttk.Frame(main)
        adb_buttons.grid(row=1, column=2, sticky="ew", padx=(6, 0), pady=4)
        ttk.Button(adb_buttons, text="adb connect", command=self._adb_connect).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(adb_buttons, text="刷新状态", command=self._adb_devices).pack(side=tk.LEFT)

        ttk.Label(main, textvariable=self.adb_status_var).grid(row=2, column=1, columnspan=2, sticky="w", pady=2)

        ttk.Label(main, text="Flutter 设备").grid(row=3, column=0, sticky="w", pady=4)
        self.device_combo = ttk.Combobox(main, textvariable=self.device_var)
        self.device_combo.grid(row=3, column=1, sticky="ew", pady=4)
        ttk.Button(main, text="flutter devices", command=self._flutter_devices).grid(row=3, column=2, padx=(6, 0), pady=4)

        ttk.Label(main, text="工程").grid(row=4, column=0, sticky="w", pady=4)
        self.project_combo = ttk.Combobox(main, textvariable=self.project_var, state="readonly")
        self.project_combo.grid(row=4, column=1, sticky="ew", pady=4)
        self.project_combo.bind("<<ComboboxSelected>>", lambda _event: self._select_project())
        ttk.Button(main, text="刷新工程", command=self._refresh_projects).grid(row=4, column=2, padx=(6, 0), pady=4)

        ttk.Label(main, text="版本").grid(row=5, column=0, sticky="w", pady=4)
        self.version_combo = ttk.Combobox(main, textvariable=self.version_var, state="readonly")
        self.version_combo.grid(row=5, column=1, sticky="ew", pady=4)
        self.version_combo.bind("<<ComboboxSelected>>", lambda _event: self._update_version_info())
        ttk.Label(main, textvariable=self.version_info_var, justify="left").grid(row=6, column=1, columnspan=2, sticky="w", pady=2)

        ttk.Label(main, text="构建目录").grid(row=7, column=0, sticky="w", pady=4)
        ttk.Entry(main, textvariable=self.build_dir_var).grid(row=7, column=1, sticky="ew", pady=4)
        ttk.Button(main, text="浏览", command=self._browse_build_dir).grid(row=7, column=2, padx=(6, 0), pady=4)

        ttk.Label(main, text="OpenOCD cfg").grid(row=8, column=0, sticky="w", pady=4)
        ttk.Entry(main, textvariable=self.openocd_cfg_var).grid(row=8, column=1, sticky="ew", pady=4)
        ttk.Label(main, text="逗号分隔 -f 项").grid(row=8, column=2, sticky="w", padx=(6, 0), pady=4)

        ttk.Label(main, text="OpenOCD scripts").grid(row=9, column=0, sticky="w", pady=4)
        ttk.Entry(main, textvariable=self.openocd_scripts_var).grid(row=9, column=1, sticky="ew", pady=4)
        ttk.Button(main, text="浏览", command=self._browse_openocd_scripts).grid(row=9, column=2, padx=(6, 0), pady=4)

        ttk.Label(main, text="固件 bin").grid(row=10, column=0, sticky="w", pady=4)
        ttk.Entry(main, textvariable=self.firmware_var).grid(row=10, column=1, sticky="ew", pady=4)
        ttk.Button(main, text="浏览", command=self._browse_firmware).grid(row=10, column=2, padx=(6, 0), pady=4)

        target_frame = ttk.Frame(main)
        target_frame.grid(row=11, column=1, sticky="w", pady=4)
        ttk.Radiobutton(target_frame, text="Flutter Android/ADB", variable=self.flutter_target_var, value="android").grid(row=0, column=0, padx=(0, 12))
        ttk.Radiobutton(target_frame, text="Flutter Web/Chrome", variable=self.flutter_target_var, value="web").grid(row=0, column=1)

        buttons = ttk.Frame(main)
        buttons.grid(row=12, column=0, columnspan=3, sticky="ew", pady=(8, 4))
        self.build_button = ttk.Button(buttons, text="CMake 构建", command=self._cmake_build)
        self.build_button.pack(side=tk.LEFT, padx=(0, 6), pady=2)
        self.flash_button = ttk.Button(buttons, text="OpenOCD 烧录", command=self._openocd_flash)
        self.flash_button.pack(side=tk.LEFT, padx=(0, 6), pady=2)
        self.flutter_button = ttk.Button(buttons, text="Flutter 运行", command=self._flutter_run)
        self.flutter_button.pack(side=tk.LEFT, padx=(0, 6), pady=2)
        ttk.Button(buttons, text="打开工程", command=self._open_project).pack(side=tk.LEFT, padx=(0, 6), pady=2)
        self.stop_button = ttk.Button(buttons, text="停止任务", command=self._stop_task)
        self.stop_button.pack(side=tk.LEFT, padx=(0, 6), pady=2)
        ttk.Button(buttons, text="清空日志", command=self._clear_log).pack(side=tk.LEFT, pady=2)

        self.log_text = scrolledtext.ScrolledText(main, height=20, state=tk.DISABLED)
        self.log_text.grid(row=13, column=0, columnspan=3, sticky="nsew", pady=(8, 0))
        main.rowconfigure(13, weight=1)

    def _browse_root(self) -> None:
        path = filedialog.askdirectory(initialdir=self.template_root_var.get() or str(TEMPLATE_ROOT))
        if path:
            self.template_root_var.set(path)
            self._refresh_projects()

    def _browse_build_dir(self) -> None:
        initial = self.build_dir_var.get() or str(self._template_root() / "build")
        path = filedialog.askdirectory(initialdir=initial)
        if path:
            self.build_dir_var.set(path)

    def _browse_openocd_scripts(self) -> None:
        path = filedialog.askdirectory(initialdir=self.openocd_scripts_var.get() or str(self._template_root()))
        if path:
            self.openocd_scripts_var.set(path)

    def _browse_firmware(self) -> None:
        initial = self.build_dir_var.get() or str(self._template_root())
        path = filedialog.askopenfilename(initialdir=initial, filetypes=[("Firmware bin", "*.bin"), ("All files", "*.*")])
        if path:
            self.firmware_var.set(path)

    def _template_root(self) -> Path:
        return Path(self.template_root_var.get().strip() or TEMPLATE_ROOT)

    def _refresh_projects(self) -> None:
        template_root = self._template_root()
        self.projects = {project.name: project for project in discover_projects(template_root)}
        names = sorted(self.projects)
        self.project_combo.configure(values=names)
        if names and self.project_var.get() not in self.projects:
            self.project_var.set(names[0])
        if self.project_var.get() in self.projects:
            self._select_project()

    def _select_project(self) -> None:
        project_dir = self.projects.get(self.project_var.get())
        if not project_dir:
            return
        self.current_project_dir = project_dir
        self.current_project_name = parse_cmake_project_name(project_dir)
        info = get_project_export_info(project_dir, self._template_root())
        versions = info.available_versions or [0]
        self.current_versions = versions
        values = [str(item) for item in versions]
        self.version_combo.configure(values=values)
        default = info.default_version if info.default_version is not None else versions[-1]
        self.version_var.set(str(default if default in versions else versions[-1]))
        self.build_dir_var.set(str(project_dir / "cmake-build-debug"))
        self._update_version_info()

    def _selected_project(self) -> Optional[Path]:
        if self.current_project_dir and self.current_project_dir.exists():
            return self.current_project_dir
        messagebox.showerror("校验失败", "请先选择工程")
        return None

    def _selected_version(self) -> int:
        try:
            return int(self.version_var.get())
        except ValueError:
            return 0

    def _flutter_dir(self) -> Optional[Path]:
        project = self._selected_project()
        if project is None:
            return None
        flutter_dir = project / f"{project.name}_upper_ui_flutter"
        if not flutter_dir.exists():
            messagebox.showerror("校验失败", f"未找到 Flutter 上位机目录: {flutter_dir}")
            return None
        return flutter_dir

    def _resolve_features(self) -> List[str]:
        project = self.current_project_dir
        if project is None:
            return ["common"]
        flutter_dir = project / f"{project.name}_upper_ui_flutter"
        config = load_upper_version_config(flutter_dir) if flutter_dir.exists() else None
        if not config:
            return ["common"]
        version = self._selected_version()
        return resolve_version_feature_list(config, version if version > 0 else None)

    def _update_version_info(self) -> None:
        features = self._resolve_features()
        project = self.current_project_dir
        flutter_dir = project / f"{project.name}_upper_ui_flutter" if project else None
        text = [
            f"工程: {project.name if project else '-'}",
            f"版本: {self.version_var.get() or '-'}",
            f"Flutter: {'存在' if flutter_dir and flutter_dir.exists() else '无'}",
            f"UPPER_FEATURES: {','.join(features)}",
        ]
        self.version_info_var.set("\n".join(text))
        if project:
            self.build_dir_var.set(str(project / "cmake-build-debug"))

    def _adb_connect(self) -> None:
        target = self.adb_target_var.get().strip()
        if not target:
            messagebox.showerror("校验失败", "ADB 地址不能为空")
            return
        if not command_exists("adb"):
            messagebox.showerror("校验失败", "未找到 adb 命令")
            return
        self._start_commands("ADB connect", [(command("adb", "connect", target), self._template_root(), None), (command("adb", "devices"), self._template_root(), None)])

    def _adb_devices(self) -> None:
        if not command_exists("adb"):
            messagebox.showerror("校验失败", "未找到 adb 命令")
            return
        self._start_commands("ADB devices", [(command("adb", "devices"), self._template_root(), None)])

    def _flutter_devices(self) -> None:
        flutter_exe = find_flutter()
        if not flutter_exe:
            messagebox.showerror("校验失败", "未找到 flutter 命令——请安装 Flutter 或将 ~/development/flutter/bin 加入 PATH")
            return
        try:
            result = subprocess.run(
                [flutter_exe, "devices", "--machine"],
                capture_output=True,
                text=True,
                timeout=30,
                cwd=str(self._template_root()),
            )
            if result.returncode != 0:
                self._log(f"flutter devices 失败: {result.stderr}")
                messagebox.showerror("Flutter devices", result.stderr.strip() or "未知错误")
                return
            devices = parse_flutter_devices_machine(result.stdout)
            if not devices:
                self._log("Flutter devices: 未找到支持的设备")
                self.device_combo.configure(values=[])
                self.device_var.set("")
                messagebox.showinfo("Flutter devices", "未找到支持的 Flutter 设备")
                return
            ids = [dev[0] for dev in devices]
            labels = [f"{dev[1]} ({dev[2]})" for dev in devices]
            self.device_combo.configure(values=ids)
            self.device_var.set(ids[0])
            self._log(f"Flutter devices: {len(devices)} 个设备")
            for dev in devices:
                self._log(f"  {dev[0]:20s} {dev[1]:20s} {dev[2]}")
        except Exception as exc:
            self._log(f"Flutter devices 异常: {exc}")
            messagebox.showerror("Flutter devices", str(exc))

    def _cmake_build(self) -> None:
        project = self._selected_project()
        if project is None:
            return
        if not command_exists("cmake"):
            messagebox.showerror("校验失败", "未找到 cmake 命令")
            return
        build_dir = Path(self.build_dir_var.get().strip())
        if not build_dir:
            messagebox.showerror("校验失败", "构建目录不能为空")
            return
        version = self._selected_version()
        args_config = command(
            "cmake",
            "-S",
            str(project),
            "-B",
            str(build_dir),
            "-G",
            "Ninja",
            f"-DAPP_VERSION={version}",
            f"-DCMAKE_TOOLCHAIN_FILE={self._template_root() / 'cmake' / 'stm32-gcc-toolchain.cmake'}",
        )
        args_build = command("cmake", "--build", str(build_dir))
        self._start_commands("CMake 构建", [(args_config, self._template_root(), None), (args_build, self._template_root(), None)])

    def _openocd_flash(self) -> None:
        project = self._selected_project()
        if project is None:
            return
        if not command_exists("cmake"):
            messagebox.showerror("校验失败", "未找到 cmake 命令")
            return
        if not command_exists("openocd"):
            messagebox.showerror("校验失败", "未找到 openocd 命令")
            return
        build_dir = Path(self.build_dir_var.get().strip())
        if not str(build_dir).strip():
            messagebox.showerror("校验失败", "构建目录不能为空")
            return
        cfg_items = [item.strip() for item in self.openocd_cfg_var.get().split(",") if item.strip()]
        if not cfg_items:
            messagebox.showerror("校验失败", "OpenOCD cfg 不能为空")
            return
        firmware_text = self.firmware_var.get().strip()
        scripts = self.openocd_scripts_var.get().strip()
        project_name = self.current_project_name
        version = self._selected_version()

        args_config = command(
            "cmake",
            "-S",
            str(project),
            "-B",
            str(build_dir),
            "-G",
            "Ninja",
            f"-DAPP_VERSION={version}",
            f"-DCMAKE_TOOLCHAIN_FILE={self._template_root() / 'cmake' / 'stm32-gcc-toolchain.cmake'}",
        )
        args_build = command("cmake", "--build", str(build_dir))

        def flash_args() -> List[str]:
            firmware = Path(firmware_text) if firmware_text else find_firmware_binary(project, project_name, build_dir)
            if firmware is None or not firmware.exists():
                raise RuntimeError("构建完成后仍未找到固件 .bin，请检查构建输出或手动选择固件")
            self.root.after(0, lambda: self.firmware_var.set(str(firmware)))
            args: List[str] = ["openocd"]
            if scripts:
                args.extend(["-s", scripts])
            for cfg in cfg_items:
                args.extend(["-f", cfg])
            args.extend([
                "-c",
                "init",
                "-c",
                "reset halt",
                "-c",
                f"program {firmware.as_posix()} 0x08000000 verify reset exit",
            ])
            return args

        self._start_commands(
            "构建并烧录",
            [
                (args_config, self._template_root(), None),
                (args_build, self._template_root(), None),
                (flash_args, self._template_root(), None),
            ],
        )

    def _flutter_run(self) -> None:
        flutter_dir = self._flutter_dir()
        if flutter_dir is None:
            return
        flutter_exe = find_flutter()
        if not flutter_exe:
            messagebox.showerror("校验失败", "未找到 flutter 命令——请安装 Flutter 或将 ~/development/flutter/bin 加入 PATH")
            return
        version = self._selected_version()
        features = self._resolve_features()
        dart_defines = [f"--dart-define=UPPER_VERSION={version}", f"--dart-define=UPPER_FEATURES={','.join(features)}"]
        target = self.flutter_target_var.get()
        if target == "web":
            args = [flutter_exe, "run", "-d", "chrome", *dart_defines]
        else:
            device = self.device_var.get().strip()
            if not device:
                messagebox.showerror("校验失败", "请先点击「flutter devices」获取设备列表，再选择设备")
                return
            args = [flutter_exe, "run", "-d", device, *dart_defines]
        self._start_commands("Flutter 运行", [([flutter_exe, "pub", "get"], flutter_dir, None), (args, flutter_dir, None)])

    def _stop_task(self) -> None:
        if not self.worker.running():
            self._log("当前没有正在执行的任务")
            return
        self._log("正在停止任务...")
        self.stop_button.configure(state=tk.DISABLED)
        self.worker.stop()

    def _open_project(self) -> None:
        project = self._selected_project()
        if project:
            open_path(project)

    def _start_commands(self, title: str, commands: Sequence[CommandSpec]) -> None:
        if self.worker.running():
            messagebox.showwarning("任务执行中", "已有任务正在执行")
            return
        self._set_buttons(False)
        self._log(f"开始: {title}")
        self.worker.start(title, commands)

    def _set_buttons(self, enabled: bool) -> None:
        task_state = tk.NORMAL if enabled else tk.DISABLED
        stop_state = tk.DISABLED if enabled else tk.NORMAL
        self.build_button.configure(state=task_state)
        self.flash_button.configure(state=task_state)
        self.flutter_button.configure(state=task_state)
        self.stop_button.configure(state=stop_state)

    def _log(self, message: str) -> None:
        append_log(self.log_text, message)
        self._consume_status_line(message)

    def _consume_status_line(self, message: str) -> None:
        if "device" in message and "\t" in message:
            devices = parse_device_rows(message)
            if devices:
                self.device_combo.configure(values=devices)
                if not self.device_var.get():
                    self.device_var.set(devices[0])
        if "connected" in message.lower() or "already connected" in message.lower():
            self.adb_status_var.set(f"ADB 状态：{message.strip()}")

    def _done(self, payload: object) -> None:
        self._set_buttons(True)
        self._log(str(payload))

    def _error(self, message: str) -> None:
        self._set_buttons(True)
        self._log(message)
        messagebox.showerror("任务失败", message.splitlines()[0] if message else "任务失败")

    def _clear_log(self) -> None:
        self.log_text.configure(state=tk.NORMAL)
        self.log_text.delete("1.0", tk.END)
        self.log_text.configure(state=tk.DISABLED)


def main() -> None:
    enable_high_dpi_awareness()
    root = tk.Tk()
    configure_tk_scaling(root)
    ProjectRunGui(root)
    root.mainloop()


if __name__ == "__main__":
    main()
