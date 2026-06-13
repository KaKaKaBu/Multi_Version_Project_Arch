#!/usr/bin/env python3
from __future__ import annotations

import queue
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, scrolledtext, ttk
from typing import Dict, List, Optional

try:
    from export_project import (
        ProjectExportInfo,
        discover_projects,
        export_project_versions,
        get_project_export_info,
        parse_extras_list,
        parse_versions,
        read_text,
        resolve_toolchain_bin,
    )
    from gui_common import (
        GuiLogSink,
        ThreadedWorker,
        append_log,
        drain_events,
        open_path,
        parse_comma_list,
        validate_relative_paths,
    )
except ImportError:
    from tools.export_project import (
        ProjectExportInfo,
        discover_projects,
        export_project_versions,
        get_project_export_info,
        parse_extras_list,
        parse_versions,
        read_text,
        resolve_toolchain_bin,
    )
    from tools.gui_common import (
        GuiLogSink,
        ThreadedWorker,
        append_log,
        drain_events,
        open_path,
        parse_comma_list,
        validate_relative_paths,
    )

TEMPLATE_ROOT = Path(__file__).resolve().parents[1]


class ExportProjectGui:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("export_project GUI")
        self.events: "queue.Queue[tuple[str, object]]" = queue.Queue()
        self.worker = ThreadedWorker(self.events)
        self.projects: Dict[str, Path] = {}
        self.info: Optional[ProjectExportInfo] = None
        self.last_output: Optional[Path] = None

        self.template_root_var = tk.StringVar(value=str(TEMPLATE_ROOT))
        self.project_var = tk.StringVar()
        self.project_dir_var = tk.StringVar()
        self.project_info_var = tk.StringVar(value="未选择工程")
        self.mode_var = tk.StringVar(value="single")
        self.single_version_var = tk.StringVar(value="1")
        self.batch_versions_var = tk.StringVar(value="1-1")
        self.output_root_var = tk.StringVar(value=str(TEMPLATE_ROOT.parent / "exports"))
        self.clean_var = tk.BooleanVar(value=True)
        self.prune_var = tk.BooleanVar(value=True)
        self.extras_var = tk.StringVar()
        self.toolchain_var = tk.StringVar()

        self._build_ui()
        self._refresh_projects()
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

        ttk.Label(main, text="工程").grid(row=1, column=0, sticky="w", pady=4)
        self.project_combo = ttk.Combobox(main, textvariable=self.project_var, state="readonly")
        self.project_combo.grid(row=1, column=1, sticky="ew", pady=4)
        self.project_combo.bind("<<ComboboxSelected>>", lambda _event: self._select_project())
        project_buttons = ttk.Frame(main)
        project_buttons.grid(row=1, column=2, sticky="ew", padx=(6, 0), pady=4)
        ttk.Button(project_buttons, text="刷新", command=self._refresh_projects).grid(row=0, column=0, padx=(0, 4))
        ttk.Button(project_buttons, text="浏览工程", command=self._browse_project).grid(row=0, column=1)

        ttk.Label(main, text="工程目录").grid(row=2, column=0, sticky="w", pady=4)
        ttk.Entry(main, textvariable=self.project_dir_var, state="readonly").grid(row=2, column=1, columnspan=2, sticky="ew", pady=4)

        ttk.Label(main, text="工程信息").grid(row=3, column=0, sticky="nw", pady=4)
        ttk.Label(main, textvariable=self.project_info_var, justify="left").grid(row=3, column=1, columnspan=2, sticky="w", pady=4)

        ttk.Label(main, text="导出模式").grid(row=4, column=0, sticky="w", pady=4)
        mode_frame = ttk.Frame(main)
        mode_frame.grid(row=4, column=1, sticky="w", pady=4)
        ttk.Radiobutton(mode_frame, text="单版本", variable=self.mode_var, value="single", command=self._update_mode).grid(row=0, column=0, padx=(0, 12))
        ttk.Radiobutton(mode_frame, text="批量导出", variable=self.mode_var, value="batch", command=self._update_mode).grid(row=0, column=1)

        ttk.Label(main, text="单版本").grid(row=5, column=0, sticky="w", pady=4)
        self.single_entry = ttk.Entry(main, textvariable=self.single_version_var, width=14)
        self.single_entry.grid(row=5, column=1, sticky="w", pady=4)

        ttk.Label(main, text="批量版本").grid(row=6, column=0, sticky="w", pady=4)
        self.batch_entry = ttk.Entry(main, textvariable=self.batch_versions_var, width=24)
        self.batch_entry.grid(row=6, column=1, sticky="w", pady=4)
        ttk.Label(main, text="示例：1-10 或 1,3,7").grid(row=6, column=2, sticky="w", padx=(6, 0), pady=4)

        ttk.Label(main, text="输出目录").grid(row=7, column=0, sticky="w", pady=4)
        ttk.Entry(main, textvariable=self.output_root_var).grid(row=7, column=1, sticky="ew", pady=4)
        ttk.Button(main, text="浏览", command=self._browse_output).grid(row=7, column=2, padx=(6, 0), pady=4)

        options = ttk.Frame(main)
        options.grid(row=8, column=1, columnspan=2, sticky="w", pady=4)
        ttk.Checkbutton(options, text="导出前清空 export_<项目>", variable=self.clean_var).grid(row=0, column=0, padx=(0, 12))
        ttk.Checkbutton(options, text="裁剪未启用条件编译", variable=self.prune_var).grid(row=0, column=1)

        ttk.Label(main, text="附加文件").grid(row=9, column=0, sticky="w", pady=4)
        ttk.Entry(main, textvariable=self.extras_var).grid(row=9, column=1, sticky="ew", pady=4)
        ttk.Label(main, text="工程内相对路径，逗号分隔").grid(row=9, column=2, sticky="w", padx=(6, 0), pady=4)

        ttk.Label(main, text="工具链 bin").grid(row=10, column=0, sticky="w", pady=4)
        ttk.Entry(main, textvariable=self.toolchain_var).grid(row=10, column=1, sticky="ew", pady=4)
        ttk.Button(main, text="浏览", command=self._browse_toolchain).grid(row=10, column=2, padx=(6, 0), pady=4)

        button_frame = ttk.Frame(main)
        button_frame.grid(row=11, column=0, columnspan=3, sticky="ew", pady=(8, 4))
        self.export_button = ttk.Button(button_frame, text="导出", command=self._start_export)
        self.export_button.grid(row=0, column=0, padx=(0, 8))
        ttk.Button(button_frame, text="打开输出目录", command=self._open_output).grid(row=0, column=1, padx=(0, 8))
        ttk.Button(button_frame, text="清空日志", command=self._clear_log).grid(row=0, column=2)

        self.log_text = scrolledtext.ScrolledText(main, height=18, state=tk.DISABLED)
        self.log_text.grid(row=12, column=0, columnspan=3, sticky="nsew", pady=(8, 0))
        main.rowconfigure(12, weight=1)
        self._update_mode()

    def _browse_root(self) -> None:
        path = filedialog.askdirectory(initialdir=self.template_root_var.get() or str(TEMPLATE_ROOT))
        if path:
            self.template_root_var.set(path)
            self._refresh_projects()

    def _browse_project(self) -> None:
        path = filedialog.askdirectory(initialdir=str(Path(self.template_root_var.get()) / "projects"))
        if path:
            self._load_project(Path(path))

    def _browse_output(self) -> None:
        path = filedialog.askdirectory(initialdir=self.output_root_var.get() or str(TEMPLATE_ROOT.parent / "exports"))
        if path:
            self.output_root_var.set(path)

    def _browse_toolchain(self) -> None:
        path = filedialog.askdirectory(initialdir=self.toolchain_var.get() or str(TEMPLATE_ROOT))
        if path:
            self.toolchain_var.set(path)

    def _refresh_projects(self) -> None:
        template_root = Path(self.template_root_var.get().strip() or TEMPLATE_ROOT)
        self.projects = {project.name: project for project in discover_projects(template_root)}
        names = sorted(self.projects)
        self.project_combo.configure(values=names)
        if names and self.project_var.get() not in self.projects:
            self.project_var.set(names[0])
            self._select_project()

    def _select_project(self) -> None:
        project = self.projects.get(self.project_var.get())
        if project:
            self._load_project(project)

    def _load_project(self, project_dir: Path) -> None:
        try:
            info = get_project_export_info(project_dir, Path(self.template_root_var.get().strip() or TEMPLATE_ROOT))
        except Exception as exc:  # noqa: BLE001 - GUI should display parser errors
            messagebox.showerror("读取失败", str(exc))
            return
        self.info = info
        self.project_dir_var.set(str(project_dir))
        version_text = "default"
        if info.version_range:
            version_text = f"{info.version_range[0]}-{info.version_range[1]}"
            self.single_version_var.set(str(info.default_version or info.version_range[0]))
            self.batch_versions_var.set(version_text)
        else:
            self.single_version_var.set("0")
            self.batch_versions_var.set("0")
        self.project_info_var.set(
            "\n".join(
                [
                    f"工程名: {info.project_name}",
                    f"版本变量: {info.version_var or '无'}",
                    f"默认版本: {info.default_version if info.default_version is not None else 'default'}",
                    f"版本范围: {version_text}",
                    f"Flutter 上位机: {'存在' if info.has_flutter_upper_ui else '无'}",
                    f"uni-app 上位机: {'存在' if info.has_uniapp_upper_ui else '无'}",
                ]
            )
        )
        self._update_mode()

    def _update_mode(self) -> None:
        single_state = tk.NORMAL if self.mode_var.get() == "single" else tk.DISABLED
        batch_state = tk.NORMAL if self.mode_var.get() == "batch" else tk.DISABLED
        self.single_entry.configure(state=single_state)
        self.batch_entry.configure(state=batch_state)

    def _parse_selected_versions(self) -> Optional[List[int]]:
        if self.info is None:
            messagebox.showerror("校验失败", "请先选择工程")
            return None
        if not self.info.version_var:
            if self.mode_var.get() == "batch":
                messagebox.showerror("校验失败", "该工程没有版本变量，不能批量导出")
                return None
            return [0]
        if self.mode_var.get() == "single":
            try:
                versions = [int(self.single_version_var.get().strip())]
            except ValueError:
                messagebox.showerror("校验失败", "单版本必须是整数")
                return None
        else:
            try:
                versions = parse_versions(self.batch_versions_var.get().strip())
            except ValueError as exc:
                messagebox.showerror("校验失败", f"批量版本格式错误: {exc}")
                return None
        if not versions:
            messagebox.showerror("校验失败", "版本不能为空")
            return None
        if self.info.version_range:
            lo, hi = self.info.version_range
            out_of_range = [version for version in versions if version < lo or version > hi]
            if out_of_range:
                messagebox.showerror("校验失败", f"版本超出范围 {lo}-{hi}: {out_of_range}")
                return None
        return versions

    def _validate(self) -> Optional[tuple[Path, Path, List[int], List[str], str]]:
        if self.info is None:
            messagebox.showerror("校验失败", "请先选择工程")
            return None
        project_dir = Path(self.project_dir_var.get())
        if not (project_dir / "CMakeLists.txt").exists():
            messagebox.showerror("校验失败", "工程目录下未找到 CMakeLists.txt")
            return None
        versions = self._parse_selected_versions()
        if versions is None:
            return None
        output_text = self.output_root_var.get().strip()
        if not output_text:
            messagebox.showerror("校验失败", "输出目录不能为空")
            return None
        output_root = Path(output_text)
        extras_items = parse_comma_list(self.extras_var.get())
        error = validate_relative_paths(project_dir, extras_items)
        if error:
            messagebox.showerror("校验失败", error)
            return None
        extras = parse_extras_list([self.extras_var.get()] if extras_items else None, no_extras=False)
        toolchain_text = self.toolchain_var.get().strip()
        if toolchain_text and not Path(toolchain_text).exists():
            messagebox.showerror("校验失败", "工具链 bin 目录不存在")
            return None
        try:
            cmake_text = read_text(project_dir / "CMakeLists.txt")
            toolchain_bin = resolve_toolchain_bin(cmake_text, toolchain_text or None)
        except Exception as exc:  # noqa: BLE001 - show validation errors in GUI
            messagebox.showerror("校验失败", str(exc))
            return None
        return project_dir, output_root, versions, extras, toolchain_bin

    def _start_export(self) -> None:
        values = self._validate()
        if values is None:
            return
        if self.worker.running():
            messagebox.showwarning("任务执行中", "已有导出任务正在执行")
            return
        project_dir, output_root, versions, extras, toolchain_bin = values
        template_root = Path(self.template_root_var.get().strip() or TEMPLATE_ROOT)
        export_root_name = f"export_{project_dir.name}"
        self.last_output = output_root / export_root_name
        self.export_button.configure(state=tk.DISABLED)
        self._log(f"开始导出: {project_dir.name} -> {self.last_output}")

        def run() -> None:
            export_project_versions(
                project_dir,
                template_root,
                output_root,
                versions,
                extras,
                toolchain_bin,
                self.clean_var.get(),
                self.prune_var.get(),
                layout="gui_package",
                export_root_name=export_root_name,
                log=GuiLogSink(self.events),
            )

        self.worker.start(run)

    def _open_output(self) -> None:
        path = self.last_output or Path(self.output_root_var.get().strip())
        if path.exists():
            open_path(path)
        else:
            messagebox.showwarning("目录不存在", f"输出目录不存在: {path}")

    def _log(self, message: str) -> None:
        append_log(self.log_text, message)

    def _done(self, _payload: object) -> None:
        self.export_button.configure(state=tk.NORMAL)
        self._log("导出完成")
        messagebox.showinfo("完成", "工程导出完成")

    def _error(self, message: str) -> None:
        self.export_button.configure(state=tk.NORMAL)
        self._log(message)
        messagebox.showerror("导出失败", message.splitlines()[0] if message else "导出失败")

    def _clear_log(self) -> None:
        self.log_text.configure(state=tk.NORMAL)
        self.log_text.delete("1.0", tk.END)
        self.log_text.configure(state=tk.DISABLED)


def main() -> None:
    root = tk.Tk()
    ExportProjectGui(root)
    root.mainloop()


if __name__ == "__main__":
    main()
