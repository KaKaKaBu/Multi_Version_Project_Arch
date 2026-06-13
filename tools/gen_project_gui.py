#!/usr/bin/env python3
from __future__ import annotations

import queue
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, scrolledtext, ttk
from typing import List, Optional

try:
    from gen_project import ProjectCreateOptions, create_project_with_options, default_app_package_name, default_app_title
    from gui_common import (
        GuiLogSink,
        ThreadedWorker,
        append_log,
        command_exists,
        drain_events,
        validate_android_package_name,
        validate_project_name,
    )
except ImportError:
    from tools.gen_project import ProjectCreateOptions, create_project_with_options, default_app_package_name, default_app_title
    from tools.gui_common import (
        GuiLogSink,
        ThreadedWorker,
        append_log,
        command_exists,
        drain_events,
        validate_android_package_name,
        validate_project_name,
    )

TEMPLATE_ROOT = Path(__file__).resolve().parents[1]


class GenProjectGui:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("gen_project GUI")
        self.events: "queue.Queue[tuple[str, object]]" = queue.Queue()
        self.worker = ThreadedWorker(self.events)

        self.template_root_var = tk.StringVar(value=str(TEMPLATE_ROOT))
        self.name_var = tk.StringVar()
        self.version_count_var = tk.StringVar(value="1")
        self.flutter_var = tk.BooleanVar(value=True)
        self.mini_program_var = tk.BooleanVar(value=False)
        self.package_var = tk.StringVar()
        self.title_var = tk.StringVar()

        self._build_ui()
        self._bind_defaults()
        drain_events(self.root, self.events, self._log, self._done, self._error)

    def _build_ui(self) -> None:
        main = ttk.Frame(self.root, padding=12)
        main.grid(row=0, column=0, sticky="nsew")
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main.columnconfigure(1, weight=1)

        ttk.Label(main, text="project_template 根目录").grid(row=0, column=0, sticky="w", pady=4)
        ttk.Entry(main, textvariable=self.template_root_var).grid(row=0, column=1, sticky="ew", pady=4)
        ttk.Button(main, text="浏览", command=self._browse_root).grid(row=0, column=2, padx=(6, 0), pady=4)

        ttk.Label(main, text="下位机工程名").grid(row=1, column=0, sticky="w", pady=4)
        ttk.Entry(main, textvariable=self.name_var).grid(row=1, column=1, sticky="ew", pady=4)

        ttk.Label(main, text="版本数量").grid(row=2, column=0, sticky="w", pady=4)
        ttk.Spinbox(main, from_=1, to=999, textvariable=self.version_count_var, width=8).grid(row=2, column=1, sticky="w", pady=4)

        ttk.Label(main, text="上位机客户端").grid(row=3, column=0, sticky="nw", pady=4)
        client_frame = ttk.Frame(main)
        client_frame.grid(row=3, column=1, sticky="w", pady=4)
        ttk.Checkbutton(client_frame, text="Flutter Android/Web", variable=self.flutter_var).grid(row=0, column=0, sticky="w")
        ttk.Checkbutton(client_frame, text="uni-app / 微信小程序", variable=self.mini_program_var).grid(row=1, column=0, sticky="w")

        ttk.Label(main, text="App 包名").grid(row=4, column=0, sticky="w", pady=4)
        ttk.Entry(main, textvariable=self.package_var).grid(row=4, column=1, sticky="ew", pady=4)

        ttk.Label(main, text="App 标题").grid(row=5, column=0, sticky="w", pady=4)
        ttk.Entry(main, textvariable=self.title_var).grid(row=5, column=1, sticky="ew", pady=4)

        button_frame = ttk.Frame(main)
        button_frame.grid(row=6, column=0, columnspan=3, sticky="ew", pady=(8, 4))
        self.create_button = ttk.Button(button_frame, text="生成工程", command=self._start_create)
        self.create_button.grid(row=0, column=0, padx=(0, 8))
        ttk.Button(button_frame, text="清空日志", command=self._clear_log).grid(row=0, column=1)

        self.log_text = scrolledtext.ScrolledText(main, height=18, state=tk.DISABLED)
        self.log_text.grid(row=7, column=0, columnspan=3, sticky="nsew", pady=(8, 0))
        main.rowconfigure(7, weight=1)

    def _bind_defaults(self) -> None:
        self.name_var.trace_add("write", lambda *_: self._refresh_defaults())

    def _browse_root(self) -> None:
        path = filedialog.askdirectory(initialdir=self.template_root_var.get() or str(TEMPLATE_ROOT))
        if path:
            self.template_root_var.set(path)

    def _refresh_defaults(self) -> None:
        name = self.name_var.get().strip()
        if not name:
            return
        self.package_var.set(default_app_package_name(name))
        self.title_var.set(default_app_title(name))

    def _validate(self) -> Optional[ProjectCreateOptions]:
        template_root = Path(self.template_root_var.get().strip())
        if not template_root.exists():
            messagebox.showerror("校验失败", "project_template 根目录不存在")
            return None

        name = self.name_var.get().strip()
        error = validate_project_name(name)
        if error:
            messagebox.showerror("校验失败", error)
            return None
        if (template_root / "projects" / name).exists():
            messagebox.showerror("校验失败", f"工程已存在: {template_root / 'projects' / name}")
            return None

        try:
            version_count = int(self.version_count_var.get().strip())
        except ValueError:
            messagebox.showerror("校验失败", "版本数量必须是整数")
            return None
        if version_count < 1:
            messagebox.showerror("校验失败", "版本数量必须大于 0")
            return None

        upper_ui_types: List[str] = []
        if self.flutter_var.get():
            upper_ui_types.append("flutter")
        if self.mini_program_var.get():
            upper_ui_types.append("mini_program")
        if not upper_ui_types:
            messagebox.showerror("校验失败", "至少选择一种上位机客户端")
            return None

        app_package_name = self.package_var.get().strip()
        error = validate_android_package_name(app_package_name)
        if error:
            messagebox.showerror("校验失败", error)
            return None

        app_title = self.title_var.get().strip()
        if not app_title:
            messagebox.showerror("校验失败", "App 标题不能为空")
            return None

        if "flutter" in upper_ui_types and not command_exists("flutter"):
            messagebox.showerror("校验失败", "未找到 flutter 命令，请先安装 Flutter 并加入 PATH")
            return None
        if "mini_program" in upper_ui_types and (not command_exists("npm") or not command_exists("npx")):
            messagebox.showerror("校验失败", "未找到 npm/npx 命令，请先安装 Node.js 并加入 PATH")
            return None

        return ProjectCreateOptions(
            name=name,
            version_count=version_count,
            upper_ui_types=upper_ui_types,
            app_package_name=app_package_name,
            app_title=app_title,
            log=GuiLogSink(self.events),
        )

    def _start_create(self) -> None:
        options = self._validate()
        if options is None:
            return
        if self.worker.running():
            messagebox.showwarning("任务执行中", "已有生成任务正在执行")
            return
        template_root = Path(self.template_root_var.get().strip())
        self.create_button.configure(state=tk.DISABLED)
        self._log(f"开始生成工程: {options.name}")
        self.worker.start(lambda: create_project_with_options(template_root, options))

    def _log(self, message: str) -> None:
        append_log(self.log_text, message)

    def _done(self, _payload: object) -> None:
        self.create_button.configure(state=tk.NORMAL)
        self._log("生成完成")
        messagebox.showinfo("完成", "工程生成完成")

    def _error(self, message: str) -> None:
        self.create_button.configure(state=tk.NORMAL)
        self._log(message)
        messagebox.showerror("生成失败", message.splitlines()[0] if message else "生成失败")

    def _clear_log(self) -> None:
        self.log_text.configure(state=tk.NORMAL)
        self.log_text.delete("1.0", tk.END)
        self.log_text.configure(state=tk.DISABLED)


def main() -> None:
    root = tk.Tk()
    GenProjectGui(root)
    root.mainloop()


if __name__ == "__main__":
    main()
