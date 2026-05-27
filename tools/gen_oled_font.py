#!/usr/bin/env python3
"""Generate OLED column-major bitmap font (oled_font.c) for SSD1306 drivers."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

FIRST_CHAR = 0x20
LAST_CHAR = 0x7E
CHAR_COUNT = LAST_CHAR - FIRST_CHAR + 1

# Classic embedded 6x8 ASCII font (column-major, LSB = top row).
FONT6X8_BUILTIN: list[list[int]] = [
    [0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
    [0x00, 0x00, 0x5F, 0x00, 0x00, 0x00],
    [0x00, 0x07, 0x00, 0x07, 0x00, 0x00],
    [0x14, 0x7F, 0x14, 0x7F, 0x14, 0x00],
    [0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x00],
    [0x23, 0x13, 0x08, 0x64, 0x62, 0x00],
    [0x36, 0x49, 0x55, 0x22, 0x50, 0x00],
    [0x00, 0x05, 0x03, 0x00, 0x00, 0x00],
    [0x00, 0x1C, 0x22, 0x41, 0x00, 0x00],
    [0x00, 0x41, 0x22, 0x1C, 0x00, 0x00],
    [0x14, 0x08, 0x3E, 0x08, 0x14, 0x00],
    [0x08, 0x08, 0x3E, 0x08, 0x08, 0x00],
    [0x00, 0x50, 0x30, 0x00, 0x00, 0x00],
    [0x08, 0x08, 0x08, 0x08, 0x08, 0x00],
    [0x00, 0x60, 0x60, 0x00, 0x00, 0x00],
    [0x20, 0x10, 0x08, 0x04, 0x02, 0x00],
    [0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00],
    [0x00, 0x42, 0x7F, 0x40, 0x00, 0x00],
    [0x42, 0x61, 0x51, 0x49, 0x46, 0x00],
    [0x21, 0x41, 0x45, 0x4B, 0x31, 0x00],
    [0x18, 0x14, 0x12, 0x7F, 0x10, 0x00],
    [0x27, 0x45, 0x45, 0x45, 0x39, 0x00],
    [0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00],
    [0x01, 0x71, 0x09, 0x05, 0x03, 0x00],
    [0x36, 0x49, 0x49, 0x49, 0x36, 0x00],
    [0x06, 0x49, 0x49, 0x29, 0x1E, 0x00],
    [0x00, 0x36, 0x36, 0x00, 0x00, 0x00],
    [0x00, 0x56, 0x36, 0x00, 0x00, 0x00],
    [0x08, 0x14, 0x22, 0x41, 0x00, 0x00],
    [0x14, 0x14, 0x14, 0x14, 0x14, 0x00],
    [0x00, 0x41, 0x22, 0x14, 0x08, 0x00],
    [0x02, 0x01, 0x51, 0x09, 0x06, 0x00],
    [0x32, 0x49, 0x79, 0x41, 0x3E, 0x00],
    [0x7E, 0x11, 0x11, 0x11, 0x7E, 0x00],
    [0x7F, 0x49, 0x49, 0x49, 0x36, 0x00],
    [0x3E, 0x41, 0x41, 0x41, 0x22, 0x00],
    [0x7F, 0x41, 0x41, 0x22, 0x1C, 0x00],
    [0x7F, 0x49, 0x49, 0x49, 0x41, 0x00],
    [0x7F, 0x09, 0x09, 0x09, 0x01, 0x00],
    [0x3E, 0x41, 0x49, 0x49, 0x7A, 0x00],
    [0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00],
    [0x00, 0x41, 0x7F, 0x41, 0x00, 0x00],
    [0x20, 0x40, 0x41, 0x3F, 0x01, 0x00],
    [0x7F, 0x08, 0x14, 0x22, 0x41, 0x00],
    [0x7F, 0x40, 0x40, 0x40, 0x40, 0x00],
    [0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x00],
    [0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00],
    [0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00],
    [0x7F, 0x09, 0x09, 0x09, 0x06, 0x00],
    [0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00],
    [0x7F, 0x09, 0x19, 0x29, 0x46, 0x00],
    [0x46, 0x49, 0x49, 0x49, 0x31, 0x00],
    [0x01, 0x01, 0x7F, 0x01, 0x01, 0x00],
    [0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00],
    [0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00],
    [0x3F, 0x40, 0x38, 0x40, 0x3F, 0x00],
    [0x63, 0x14, 0x08, 0x14, 0x63, 0x00],
    [0x07, 0x08, 0x70, 0x08, 0x07, 0x00],
    [0x61, 0x51, 0x49, 0x45, 0x43, 0x00],
    [0x00, 0x7F, 0x41, 0x41, 0x00, 0x00],
    [0x02, 0x04, 0x08, 0x10, 0x20, 0x00],
    [0x00, 0x41, 0x41, 0x7F, 0x00, 0x00],
    [0x04, 0x02, 0x01, 0x02, 0x04, 0x00],
    [0x40, 0x40, 0x40, 0x40, 0x40, 0x00],
    [0x00, 0x01, 0x02, 0x04, 0x00, 0x00],
    [0x20, 0x54, 0x54, 0x54, 0x78, 0x00],
    [0x7F, 0x48, 0x44, 0x44, 0x38, 0x00],
    [0x38, 0x44, 0x44, 0x44, 0x20, 0x00],
    [0x38, 0x44, 0x44, 0x48, 0x7F, 0x00],
    [0x38, 0x54, 0x54, 0x54, 0x18, 0x00],
    [0x08, 0x7E, 0x09, 0x01, 0x02, 0x00],
    [0x0C, 0x52, 0x52, 0x52, 0x3E, 0x00],
    [0x7F, 0x08, 0x04, 0x04, 0x78, 0x00],
    [0x00, 0x44, 0x7D, 0x40, 0x00, 0x00],
    [0x20, 0x40, 0x44, 0x3D, 0x00, 0x00],
    [0x7F, 0x10, 0x28, 0x44, 0x00, 0x00],
    [0x00, 0x41, 0x7F, 0x40, 0x00, 0x00],
    [0x7C, 0x04, 0x18, 0x04, 0x78, 0x00],
    [0x7C, 0x08, 0x04, 0x04, 0x78, 0x00],
    [0x38, 0x44, 0x44, 0x44, 0x38, 0x00],
    [0x7C, 0x14, 0x14, 0x14, 0x08, 0x00],
    [0x08, 0x14, 0x14, 0x18, 0x7C, 0x00],
    [0x7C, 0x08, 0x04, 0x04, 0x08, 0x00],
    [0x48, 0x54, 0x54, 0x54, 0x24, 0x00],
    [0x04, 0x3F, 0x44, 0x40, 0x20, 0x00],
    [0x3C, 0x40, 0x40, 0x20, 0x7C, 0x00],
    [0x1C, 0x20, 0x40, 0x20, 0x1C, 0x00],
    [0x3C, 0x40, 0x30, 0x40, 0x3C, 0x00],
    [0x44, 0x28, 0x10, 0x28, 0x44, 0x00],
    [0x0C, 0x50, 0x50, 0x50, 0x3C, 0x00],
    [0x44, 0x64, 0x54, 0x4C, 0x44, 0x00],
    [0x08, 0x36, 0x41, 0x41, 0x00, 0x00],
    [0x00, 0x00, 0x7F, 0x00, 0x00, 0x00],
    [0x00, 0x41, 0x41, 0x36, 0x08, 0x00],
    [0x08, 0x04, 0x08, 0x10, 0x08, 0x00],
]


def col_bytes_for_height(height: int) -> int:
    return (height + 7) // 8


def unpack_columns(glyph_cols: list[int], src_height: int = 8) -> list[list[int]]:
    columns: list[list[int]] = []
    for col_byte in glyph_cols:
        column = [(col_byte >> bit) & 1 for bit in range(src_height)]
        columns.append(column)
    return columns


def pack_columns(columns: list[list[int]], width: int, height: int) -> list[int]:
    col_bytes = col_bytes_for_height(height)
    packed: list[int] = []
    for col in range(width):
        for byte_row in range(col_bytes):
            value = 0
            for bit in range(8):
                row = byte_row * 8 + bit
                if row < height and col < len(columns) and row < len(columns[col]):
                    if columns[col][row]:
                        value |= 1 << bit
            packed.append(value)
    return packed


def bitmap_to_columns(bitmap: list[list[int]], width: int, height: int) -> list[int]:
    columns = [[bitmap[row][col] for row in range(height)] for col in range(width)]
    return pack_columns(columns, width, height)


def scale_columns(columns: list[list[int]], src_w: int, src_h: int, dst_w: int, dst_h: int) -> list[int]:
    dst_columns: list[list[int]] = []
    for col in range(dst_w):
        src_col = min(src_w - 1, (col * src_w) // dst_w)
        column: list[int] = []
        for row in range(dst_h):
            src_row = min(src_h - 1, (row * src_h) // dst_h)
            column.append(columns[src_col][src_row])
        dst_columns.append(column)
    return pack_columns(dst_columns, dst_w, dst_h)


def load_builtin_glyphs(width: int, height: int) -> list[list[int]]:
    if len(FONT6X8_BUILTIN) != CHAR_COUNT:
        raise RuntimeError("builtin font entry count mismatch")

    glyphs: list[list[int]] = []
    for entry in FONT6X8_BUILTIN:
        columns = unpack_columns(entry, 8)
        if width == 6 and height == 8:
            glyphs.append(entry[:6])
        else:
            glyphs.append(scale_columns(columns, 6, 8, width, height))
    return glyphs


def load_ttf_glyphs(font_path: Path, width: int, height: int, font_size: int) -> list[list[int]]:
    try:
        from PIL import Image, ImageDraw, ImageFont
    except ImportError as exc:
        raise RuntimeError("TTF mode requires Pillow: pip install pillow") from exc

    font = ImageFont.truetype(str(font_path), font_size)
    glyphs: list[list[int]] = []

    for code in range(FIRST_CHAR, LAST_CHAR + 1):
        ch = chr(code)
        image = Image.new("1", (width, height), 0)
        draw = ImageDraw.Draw(image)
        bbox = draw.textbbox((0, 0), ch, font=font)
        text_w = bbox[2] - bbox[0]
        text_h = bbox[3] - bbox[1]
        x = max(0, (width - text_w) // 2 - bbox[0])
        y = max(0, (height - text_h) // 2 - bbox[1])
        draw.text((x, y), ch, font=font, fill=1)

        bitmap = [[1 if image.getpixel((col, row)) else 0 for col in range(width)] for row in range(height)]
        glyphs.append(bitmap_to_columns(bitmap, width, height))

    return glyphs


def format_row(values: list[int]) -> str:
    return "{ " + ", ".join(f"0x{value:02X}" for value in values) + " }"


def render_source(glyphs: list[list[int]], width: int, height: int, source: str) -> str:
    col_bytes = col_bytes_for_height(height)
    glyph_stride = width * col_bytes
    lines: list[str] = [
        "/* Auto-generated by tools/gen_oled_font.py. Do not edit manually. */",
        f"/* Source: {source}, cell {width}x{height}, ASCII 0x{FIRST_CHAR:02X}-0x{LAST_CHAR:02X} */",
        "",
        '#include "oled_font.h"',
        "",
        f"#define OLED_FONT_FIRST_CHAR 0x{FIRST_CHAR:02X}U",
        f"#define OLED_FONT_LAST_CHAR  0x{LAST_CHAR:02X}U",
        f"#define OLED_FONT_CHAR_COUNT {CHAR_COUNT}U",
        f"#define OLED_FONT_WIDTH      {width}U",
        f"#define OLED_FONT_HEIGHT     {height}U",
        f"#define OLED_FONT_COL_BYTES  {col_bytes}U",
        f"#define OLED_FONT_GLYPH_SIZE {glyph_stride}U",
        "",
        f"static const uint8_t oled_font_table[OLED_FONT_CHAR_COUNT][OLED_FONT_GLYPH_SIZE] = {{",
    ]

    for index, glyph in enumerate(glyphs):
        ch = FIRST_CHAR + index
        lines.append(f"    /* 0x{ch:02X} '{chr(ch) if 32 <= ch <= 126 else '?'}' */")
        lines.append(f"    {format_row(glyph)},")

    lines.extend(
        [
            "};",
            "",
            "uint8_t oled_font_get_width(void)",
            "{",
            "    return OLED_FONT_WIDTH;",
            "}",
            "",
            "uint8_t oled_font_get_height(void)",
            "{",
            "    return OLED_FONT_HEIGHT;",
            "}",
            "",
            "uint8_t oled_font_get_col_bytes(void)",
            "{",
            "    return OLED_FONT_COL_BYTES;",
            "}",
            "",
            "const uint8_t *oled_font_get_glyph(char ch)",
            "{",
            "    if ((ch < (char)OLED_FONT_FIRST_CHAR) || (ch > (char)OLED_FONT_LAST_CHAR)) {",
            "        ch = '?';",
            "    }",
            "",
            "    return oled_font_table[(uint8_t)ch - OLED_FONT_FIRST_CHAR];",
            "}",
            "",
        ]
    )
    return "\n".join(lines)


def parse_args() -> argparse.Namespace:
    default_output = Path(__file__).resolve().parent.parent / "drivers" / "displays" / "oled_font.c"

    parser = argparse.ArgumentParser(description="Generate SSD1306 OLED bitmap font source file.")
    parser.add_argument(
        "-W",
        "--width",
        type=int,
        default=6,
        help="Glyph cell width in pixels (default: 6)",
    )
    parser.add_argument(
        "-H",
        "--height",
        type=int,
        default=8,
        help="Glyph cell height in pixels (default: 8)",
    )
    parser.add_argument(
        "-s",
        "--font-size",
        type=int,
        default=None,
        help="TTF render size in pixels (default: same as --height)",
    )
    parser.add_argument(
        "-f",
        "--font",
        type=Path,
        default=None,
        help="Optional TTF/OTF font path; omit to use built-in 6x8 and scale to target size",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=default_output,
        help=f"Output C source path (default: {default_output})",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if args.width <= 0 or args.height <= 0:
        print("width and height must be positive", file=sys.stderr)
        return 1

    font_size = args.font_size if args.font_size is not None else args.height

    try:
        if args.font is not None:
            glyphs = load_ttf_glyphs(args.font, args.width, args.height, font_size)
            source = f"TTF {args.font.name} size {font_size}"
        else:
            glyphs = load_builtin_glyphs(args.width, args.height)
            source = "builtin 6x8 scaled" if (args.width, args.height) != (6, 8) else "builtin 6x8"

        content = render_source(glyphs, args.width, args.height, source)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(content, encoding="utf-8", newline="\n")
    except RuntimeError as exc:
        print(str(exc), file=sys.stderr)
        return 1

    print(f"Generated {args.output} ({args.width}x{args.height}, {CHAR_COUNT} glyphs)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
