#!/usr/bin/env python3
"""按行号范围插入 #if guard（1-based，含首尾）。"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CJSON_C = ROOT / "common" / "utils" / "cJSON.c"

# (start_line, end_line, macro)  1-based inclusive
RANGES: list[tuple[int, int, str]] = [
    (94, 117, "CJSON_DISABLE_GET_HELPERS"),
    (146, 153, "CJSON_DISABLE_VERSION"),
    (430, 490, "CJSON_DISABLE_SET_HELPERS"),
    (603, 607, "CJSON_DISABLE_COMPARE"),
    (1137, 1150, "CJSON_DISABLE_PARSE_API"),
    (1233, 1236, "CJSON_DISABLE_PARSE_API"),
    (1313, 1316, "CJSON_DISABLE_FORMATTED_PRINT"),
    (1323, 1371, "CJSON_DISABLE_PRINT_BUFFERED"),
    (1502, 1660, "CJSON_DISABLE_ARRAYS"),
    (1895, 1945, "CJSON_DISABLE_ARRAY_API"),
    (1984, 1992, "CJSON_DISABLE_GET_HELPERS"),
    (2002, 2021, "CJSON_DISABLE_REFERENCE"),
    (2057, 2060, "CJSON_DISABLE_ARRAY_API"),
    (2121, 2144, "CJSON_DISABLE_REFERENCE"),
    (2146, 2193, "CJSON_DISABLE_CREATE_EXTRAS"),
    (2218, 2252, "CJSON_DISABLE_CREATE_EXTRAS"),
    (2254, 2454, "CJSON_DISABLE_UTILS"),
    (2457, 2499, "CJSON_DISABLE_CREATE_EXTRAS"),
    (2544, 2603, "CJSON_DISABLE_CREATE_EXTRAS"),
    (2617, 2775, "CJSON_DISABLE_CREATE_EXTRAS"),
    (2777, 2869, "CJSON_DISABLE_DUPLICATE"),
    (2871, 2966, "CJSON_DISABLE_MINIFY"),
    (2968, 3026, "CJSON_DISABLE_IS_HELPERS"),
    (3038, 3066, "CJSON_DISABLE_IS_HELPERS"),
    (3068, 3191, "CJSON_DISABLE_COMPARE"),
]


def main() -> int:
    lines = CJSON_C.read_text(encoding="utf-8").splitlines(keepends=True)
    inserts: dict[int, list[str]] = {}

    for start, end, macro in RANGES:
        if start < 1 or end > len(lines):
            raise SystemExit(f"range {start}-{end} out of file ({len(lines)} lines)")
        block = "".join(lines[start - 1 : end])
        if "#if !" + macro in block:
            continue
        inserts.setdefault(start - 1, []).insert(0, f"#if !{macro}\n")
        inserts.setdefault(end, []).append(f"#endif /* {macro} */\n")

    out: list[str] = []
    for i, line in enumerate(lines):
        if i in inserts:
            out.extend(inserts[i])
        out.append(line)

    CJSON_C.write_text("".join(out), encoding="utf-8", newline="\n")
    print(f"Inserted {len(RANGES)} guard ranges into {CJSON_C}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
