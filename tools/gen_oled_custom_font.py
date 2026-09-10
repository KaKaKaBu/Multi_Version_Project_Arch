#!/usr/bin/env python3
"""Generate display_font_t Chinese glyph headers for the SSD1306 driver.

The generator deliberately uses the Windows GDI font rasterizer through ctypes.
That keeps the workflow dependency-free on the Windows build machine while still
allowing Chinese glyphs to be generated from an installed system font.
"""

from __future__ import annotations

import argparse
import ctypes
import re
import sys
from pathlib import Path
from typing import Iterable


BI_RGB = 0
DIB_RGB_COLORS = 0
FW_NORMAL = 400
DEFAULT_CHARSET = 1
OUT_DEFAULT_PRECIS = 0
CLIP_DEFAULT_PRECIS = 0
NONANTIALIASED_QUALITY = 3
DEFAULT_PITCH = 0
FF_DONTCARE = 0
TRANSPARENT = 1


class BitmapInfoHeader(ctypes.Structure):
    _fields_ = [
        ("biSize", ctypes.c_uint32),
        ("biWidth", ctypes.c_int32),
        ("biHeight", ctypes.c_int32),
        ("biPlanes", ctypes.c_uint16),
        ("biBitCount", ctypes.c_uint16),
        ("biCompression", ctypes.c_uint32),
        ("biSizeImage", ctypes.c_uint32),
        ("biXPelsPerMeter", ctypes.c_int32),
        ("biYPelsPerMeter", ctypes.c_int32),
        ("biClrUsed", ctypes.c_uint32),
        ("biClrImportant", ctypes.c_uint32),
    ]


class BitmapInfo(ctypes.Structure):
    _fields_ = [("bmiHeader", BitmapInfoHeader), ("bmiColors", ctypes.c_uint32 * 2)]


class GdiRenderer:
    """Small top-down 32-bit DIB renderer backed by user32/gdi32."""

    def __init__(self, font_name: str, font_size: int, canvas: int):
        if sys.platform != "win32":
            raise RuntimeError("Chinese OLED font generation currently requires Windows GDI")

        self.width = canvas
        self.height = canvas
        self.gdi32 = ctypes.windll.gdi32
        self.user32 = ctypes.windll.user32
        self.hdc = self.gdi32.CreateCompatibleDC(None)
        if not self.hdc:
            raise RuntimeError("CreateCompatibleDC failed")

        bmi = BitmapInfo()
        bmi.bmiHeader.biSize = ctypes.sizeof(BitmapInfoHeader)
        bmi.bmiHeader.biWidth = canvas
        bmi.bmiHeader.biHeight = -canvas  # top-down DIB
        bmi.bmiHeader.biPlanes = 1
        bmi.bmiHeader.biBitCount = 32
        bmi.bmiHeader.biCompression = BI_RGB
        self.bits = ctypes.c_void_p()
        self.bitmap = self.gdi32.CreateDIBSection(
            self.hdc, ctypes.byref(bmi), DIB_RGB_COLORS,
            ctypes.byref(self.bits), None, 0
        )
        if not self.bitmap or not self.bits.value:
            self.close()
            raise RuntimeError("CreateDIBSection failed")

        self.old_bitmap = self.gdi32.SelectObject(self.hdc, self.bitmap)
        self.font = self.gdi32.CreateFontW(
            -font_size, 0, 0, 0, FW_NORMAL, 0, 0, 0,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, font_name
        )
        if not self.font:
            self.close()
            raise RuntimeError(f"CreateFontW failed for {font_name!r}")
        self.old_font = self.gdi32.SelectObject(self.hdc, self.font)
        self.gdi32.SetBkMode(self.hdc, TRANSPARENT)
        self.gdi32.SetTextColor(self.hdc, 0x00FFFFFF)
        self.pixels = (ctypes.c_uint32 * (canvas * canvas)).from_address(self.bits.value)

    def render(self, text: str, target_width: int, target_height: int) -> list[list[int]]:
        for i in range(self.width * self.height):
            self.pixels[i] = 0
        pad = max(2, self.width // 2)
        self.gdi32.TextOutW(self.hdc, pad, pad, text, 1)

        points: list[tuple[int, int]] = []
        for y in range(self.height):
            for x in range(self.width):
                if self.pixels[y * self.width + x] & 0x00FFFFFF:
                    points.append((x, y))
        if not points:
            return [[0] * self.width for _ in range(self.height)]

        min_x = min(p[0] for p in points)
        max_x = max(p[0] for p in points)
        min_y = min(p[1] for p in points)
        max_y = max(p[1] for p in points)
        cropped = [
            [1 if self.pixels[y * self.width + x] & 0x00FFFFFF else 0
             for x in range(min_x, max_x + 1)]
            for y in range(min_y, max_y + 1)
        ]

        src_h = len(cropped)
        src_w = len(cropped[0])
        target_w = min(target_width, src_w)
        target_h = min(target_height, src_h)
        result = [[0] * target_width for _ in range(target_height)]
        left = (target_width - target_w) // 2
        top = (target_height - target_h) // 2
        for dy in range(target_h):
            sy = min(src_h - 1, (dy * src_h) // target_h)
            for dx in range(target_w):
                sx = min(src_w - 1, (dx * src_w) // target_w)
                result[top + dy][left + dx] = cropped[sy][sx]
        return result

    def close(self) -> None:
        if getattr(self, "font", None):
            self.gdi32.SelectObject(self.hdc, self.old_font)
            self.gdi32.DeleteObject(self.font)
            self.font = None
        if getattr(self, "bitmap", None):
            self.gdi32.SelectObject(self.hdc, self.old_bitmap)
            self.gdi32.DeleteObject(self.bitmap)
            self.bitmap = None
        if getattr(self, "hdc", None):
            self.gdi32.DeleteDC(self.hdc)
            self.hdc = None

    def __enter__(self) -> "GdiRenderer":
        return self

    def __exit__(self, *_: object) -> None:
        self.close()


def strip_c_comments(source: str) -> str:
    """Remove C comments without treating // or /* inside strings as comments."""
    out: list[str] = []
    i = 0
    quote: str | None = None
    while i < len(source):
        if quote:
            out.append(source[i])
            if source[i] == "\\" and i + 1 < len(source):
                i += 1
                out.append(source[i])
            elif source[i] == quote:
                quote = None
            i += 1
            continue
        if source[i] in ('"', "'"):
            quote = source[i]
            out.append(source[i])
            i += 1
        elif source.startswith("//", i):
            end = source.find("\n", i)
            i = len(source) if end < 0 else end
        elif source.startswith("/*", i):
            end = source.find("*/", i + 2)
            i = len(source) if end < 0 else end + 2
        else:
            out.append(source[i])
            i += 1
    return "".join(out)


def unescape_c_string(value: str) -> str:
    replacements = {
        "\\n": "\n", "\\r": "\r", "\\t": "\t",
        "\\\\": "\\", '\\"': '"', "\\'": "'",
    }
    for old, new in replacements.items():
        value = value.replace(old, new)
    return re.sub(r"\\x([0-9A-Fa-f]{2})", lambda m: chr(int(m.group(1), 16)), value)


def extract_chars(paths: Iterable[Path], extra_text: Iterable[str]) -> list[str]:
    chars: set[str] = set()
    literal_re = re.compile(r'"(?:\\.|[^"\\])*"')
    for path in paths:
        source = strip_c_comments(path.read_text(encoding="utf-8"))
        for match in literal_re.finditer(source):
            value = unescape_c_string(match.group()[1:-1])
            chars.update(ch for ch in value if ord(ch) >= 0x80)
    for value in extra_text:
        chars.update(ch for ch in value if ord(ch) >= 0x80)
    return sorted(chars, key=ord)


def bitmap_bytes(bitmap: list[list[int]], width: int) -> list[int]:
    bytes_per_row = (width + 7) // 8
    values: list[int] = []
    for row in bitmap:
        for byte_index in range(bytes_per_row):
            value = 0
            for bit in range(8):
                x = byte_index * 8 + bit
                if x < width and x < len(row) and row[x]:
                    value |= 0x80 >> bit
            values.append(value)
    return values


def c_hex(codepoint: int) -> str:
    return f"{codepoint:04X}"


def render_header(symbol: str, width: int, height: int, glyphs: dict[str, list[int]], source: str) -> str:
    guard = re.sub(r"[^A-Za-z0-9_]", "_", symbol).upper() + "_H"
    lines = [
        "/* Auto-generated by tools/gen_oled_custom_font.py. Do not edit manually. */",
        f"/* Source: {source}; cell {width}x{height}; glyphs {len(glyphs)} */",
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        '#include "display_font.h"',
        "",
    ]
    for ch, values in glyphs.items():
        cp = ord(ch)
        values_text = ", ".join(f"0x{value:02X}" for value in values)
        lines.append(f"static const uint8_t {symbol}_u{c_hex(cp)}[] = {{ {values_text} }};")
    lines.extend(["", f"static const display_glyph_t {symbol}_glyphs[] = {{"])
    for ch in glyphs:
        cp = ord(ch)
        lines.append(f"    {{ 0x{c_hex(cp)}U, {width}U, {height}U, {symbol}_u{c_hex(cp)} }}, /* {ch} */")
    lines.extend([
        "};",
        "",
        f"static const display_font_t {symbol} = {{",
        f"    {width}U,",
        f"    {height}U,",
        f"    {(width + 7) // 8}U,",
        "    0U,",
        "    0U,",
        "    0,",
        f"    {symbol}_glyphs,",
        f"    {len(glyphs)}U",
        "};",
        "",
        f"#endif /* {guard} */",
        "",
    ])
    return "\n".join(lines)


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description="Generate UTF-8 Chinese SSD1306 custom glyphs")
    parser.add_argument("--root", type=Path, default=root / "projects", help="Source tree to scan")
    parser.add_argument("--glob", action="append", default=None,
                        help="Source glob, repeatable")
    parser.add_argument("--text", action="append", default=[], help="Additional literal text")
    parser.add_argument("--font", default="SimHei", help="Installed Windows font family")
    parser.add_argument("--font-size", type=int, default=12)
    parser.add_argument("--width", type=int, default=8)
    parser.add_argument("--height", type=int, default=8)
    parser.add_argument("--symbol", default="oled_cn_font")
    parser.add_argument("--output", type=Path, default=root / "drivers" / "displays" / "oled_cn_font.h")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.width <= 0 or args.height <= 0 or args.font_size <= 0:
        print("width, height and font-size must be positive", file=sys.stderr)
        return 2
    patterns = args.glob or ["**/app/app_logic.c", "**/app/version_config.h"]
    paths = sorted({path for pattern in patterns for path in args.root.glob(pattern) if path.is_file()})
    chars = extract_chars(paths, args.text)
    if not chars:
        print("No non-ASCII characters found in the selected source files", file=sys.stderr)
        return 1

    canvas = max(args.width * 4, args.font_size * 4, 32)
    try:
        with GdiRenderer(args.font, args.font_size, canvas) as renderer:
            glyphs = {
                ch: bitmap_bytes(renderer.render(ch, args.width, args.height), args.width)
                for ch in chars
            }
    except (OSError, RuntimeError) as exc:
        print(str(exc), file=sys.stderr)
        return 1

    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(render_header(args.symbol, args.width, args.height, glyphs, ", ".join(map(str, paths))), encoding="utf-8", newline="\n")
    print(f"Generated {output} ({len(chars)} glyphs, {args.width}x{args.height}, font={args.font})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
