#!/usr/bin/env python3
"""在 cJSON.c / cJSON.h 中插入 CJSON_DISABLE_* 条件编译 guard。"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CJSON_C = ROOT / "common" / "utils" / "cJSON.c"
CJSON_H = ROOT / "common" / "utils" / "cJSON.h"

# (function_name_pattern, disable_macro)
C_GUARDS: list[tuple[str, str]] = [
    (r"cJSON_GetErrorPtr", "CJSON_DISABLE_GET_HELPERS"),
    (r"cJSON_GetStringValue", "CJSON_DISABLE_GET_HELPERS"),
    (r"cJSON_GetNumberValue", "CJSON_DISABLE_GET_HELPERS"),
    (r"cJSON_Version", "CJSON_DISABLE_VERSION"),
    (r"cJSON_SetNumberHelper", "CJSON_DISABLE_SET_HELPERS"),
    (r"cJSON_SetValuestring", "CJSON_DISABLE_SET_HELPERS"),
    (r"compare_double", "CJSON_DISABLE_COMPARE"),
    (r"cJSON_ParseWithOpts\b", "CJSON_DISABLE_PARSE_API"),
    (r"cJSON_Parse\b", "CJSON_DISABLE_PARSE_API"),
    (r"cJSON_Print\b", "CJSON_DISABLE_FORMATTED_PRINT"),
    (r"cJSON_PrintBuffered", "CJSON_DISABLE_PRINT_BUFFERED"),
    (r"cJSON_PrintPreallocated", "CJSON_DISABLE_PRINT_BUFFERED"),
    (r"parse_array", "CJSON_DISABLE_ARRAYS"),
    (r"print_array", "CJSON_DISABLE_ARRAYS"),
    (r"cJSON_GetArraySize", "CJSON_DISABLE_ARRAY_API"),
    (r"cJSON_GetArrayItem", "CJSON_DISABLE_ARRAY_API"),
    (r"get_array_item", "CJSON_DISABLE_ARRAY_API"),
    (r"add_item_to_array", "CJSON_DISABLE_ARRAY_API"),
    (r"cJSON_AddItemToArray", "CJSON_DISABLE_ARRAY_API"),
    (r"cJSON_AddItemReferenceToArray", "CJSON_DISABLE_REFERENCE"),
    (r"cJSON_AddItemReferenceToObject", "CJSON_DISABLE_REFERENCE"),
    (r"cJSON_DetachItemViaPointer", "CJSON_DISABLE_UTILS"),
    (r"cJSON_DetachItemFromArray", "CJSON_DISABLE_UTILS"),
    (r"cJSON_DeleteItemFromArray", "CJSON_DISABLE_UTILS"),
    (r"cJSON_DetachItemFromObject", "CJSON_DISABLE_UTILS"),
    (r"cJSON_DetachItemFromObjectCaseSensitive", "CJSON_DISABLE_UTILS"),
    (r"cJSON_DeleteItemFromObject", "CJSON_DISABLE_UTILS"),
    (r"cJSON_InsertItemInArray", "CJSON_DISABLE_UTILS"),
    (r"cJSON_ReplaceItemViaPointer", "CJSON_DISABLE_UTILS"),
    (r"cJSON_ReplaceItemInArray", "CJSON_DISABLE_UTILS"),
    (r"replace_item_in_object", "CJSON_DISABLE_UTILS"),
    (r"cJSON_ReplaceItemInObject", "CJSON_DISABLE_UTILS"),
    (r"cJSON_CreateNull", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_AddNullToObject", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_CreateTrue", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_AddTrueToObject", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_CreateFalse", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_AddFalseToObject", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_CreateBool", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_AddBoolToObject", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_CreateStringReference", "CJSON_DISABLE_REFERENCE"),
    (r"cJSON_CreateObjectReference", "CJSON_DISABLE_REFERENCE"),
    (r"cJSON_CreateArrayReference", "CJSON_DISABLE_REFERENCE"),
    (r"cJSON_CreateRaw", "CJSON_DISABLE_RAW"),
    (r"cJSON_AddRawToObject", "CJSON_DISABLE_RAW"),
    (r"cJSON_CreateArray\b", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_AddArrayToObject", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_AddObjectToObject", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_CreateIntArray", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_CreateFloatArray", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_CreateDoubleArray", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_CreateStringArray", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_Duplicate_rec", "CJSON_DISABLE_DUPLICATE"),
    (r"cJSON_Duplicate\b", "CJSON_DISABLE_DUPLICATE"),
    (r"skip_oneline_comment", "CJSON_DISABLE_MINIFY"),
    (r"skip_multiline_comment", "CJSON_DISABLE_MINIFY"),
    (r"minify_string", "CJSON_DISABLE_MINIFY"),
    (r"cJSON_Minify", "CJSON_DISABLE_MINIFY"),
    (r"cJSON_IsInvalid", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_IsFalse", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_IsTrue", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_IsBool", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_IsNull", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_IsNumber", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_IsArray", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_IsObject", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_IsRaw", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_Compare", "CJSON_DISABLE_COMPARE"),
    (r"create_reference", "CJSON_DISABLE_REFERENCE"),
]

H_GUARDS: list[tuple[str, str]] = [
    (r"cJSON_Version", "CJSON_DISABLE_VERSION"),
    (r"cJSON_Parse\b", "CJSON_DISABLE_PARSE_API"),
    (r"cJSON_ParseWithOpts", "CJSON_DISABLE_PARSE_API"),
    (r"cJSON_Print\b", "CJSON_DISABLE_FORMATTED_PRINT"),
    (r"cJSON_PrintBuffered", "CJSON_DISABLE_PRINT_BUFFERED"),
    (r"cJSON_PrintPreallocated", "CJSON_DISABLE_PRINT_BUFFERED"),
    (r"cJSON_GetArraySize", "CJSON_DISABLE_ARRAY_API"),
    (r"cJSON_GetArrayItem", "CJSON_DISABLE_ARRAY_API"),
    (r"cJSON_GetErrorPtr", "CJSON_DISABLE_GET_HELPERS"),
    (r"cJSON_GetStringValue", "CJSON_DISABLE_GET_HELPERS"),
    (r"cJSON_GetNumberValue", "CJSON_DISABLE_GET_HELPERS"),
    (r"cJSON_IsInvalid", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_IsFalse", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_IsTrue", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_IsBool", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_IsNull", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_IsNumber", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_IsArray", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_IsObject", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_IsRaw", "CJSON_DISABLE_IS_HELPERS"),
    (r"cJSON_CreateNull", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_CreateTrue", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_CreateFalse", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_CreateBool", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_CreateRaw", "CJSON_DISABLE_RAW"),
    (r"cJSON_CreateArray\b", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_CreateStringReference", "CJSON_DISABLE_REFERENCE"),
    (r"cJSON_CreateObjectReference", "CJSON_DISABLE_REFERENCE"),
    (r"cJSON_CreateArrayReference", "CJSON_DISABLE_REFERENCE"),
    (r"cJSON_CreateIntArray", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_CreateFloatArray", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_CreateDoubleArray", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_CreateStringArray", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_AddItemToArray", "CJSON_DISABLE_ARRAY_API"),
    (r"cJSON_AddItemReferenceToArray", "CJSON_DISABLE_REFERENCE"),
    (r"cJSON_AddItemReferenceToObject", "CJSON_DISABLE_REFERENCE"),
    (r"cJSON_DetachItemViaPointer", "CJSON_DISABLE_UTILS"),
    (r"cJSON_DetachItemFromArray", "CJSON_DISABLE_UTILS"),
    (r"cJSON_DeleteItemFromArray", "CJSON_DISABLE_UTILS"),
    (r"cJSON_DetachItemFromObject", "CJSON_DISABLE_UTILS"),
    (r"cJSON_DetachItemFromObjectCaseSensitive", "CJSON_DISABLE_UTILS"),
    (r"cJSON_DeleteItemFromObject", "CJSON_DISABLE_UTILS"),
    (r"cJSON_InsertItemInArray", "CJSON_DISABLE_UTILS"),
    (r"cJSON_ReplaceItemViaPointer", "CJSON_DISABLE_UTILS"),
    (r"cJSON_ReplaceItemInArray", "CJSON_DISABLE_UTILS"),
    (r"cJSON_ReplaceItemInObject", "CJSON_DISABLE_UTILS"),
    (r"cJSON_Duplicate", "CJSON_DISABLE_DUPLICATE"),
    (r"cJSON_Compare", "CJSON_DISABLE_COMPARE"),
    (r"cJSON_Minify", "CJSON_DISABLE_MINIFY"),
    (r"cJSON_AddNullToObject", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_AddTrueToObject", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_AddFalseToObject", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_AddBoolToObject", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_AddRawToObject", "CJSON_DISABLE_RAW"),
    (r"cJSON_AddObjectToObject", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_AddArrayToObject", "CJSON_DISABLE_CREATE_EXTRAS"),
    (r"cJSON_SetNumberHelper", "CJSON_DISABLE_SET_HELPERS"),
    (r"cJSON_SetValuestring", "CJSON_DISABLE_SET_HELPERS"),
]


def find_matching_brace(text: str, open_index: int) -> int:
    depth = 0
    i = open_index
    in_string = False
    escape = False
    while i < len(text):
        ch = text[i]
        if in_string:
            if escape:
                escape = False
            elif ch == "\\":
                escape = True
            elif ch == '"':
                in_string = False
        else:
            if ch == '"':
                in_string = True
            elif ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1
                if depth == 0:
                    return i
        i += 1
    raise ValueError(f"unmatched brace at {open_index}")


def find_function_start(text: str, name_pattern: str, start_at: int = 0) -> int | None:
    patterns = [
        re.compile(
            rf"^(?:static\s+)?(?:[\w\s\*]+?\s+)?(?:CJSON_PUBLIC\s*\([^)]*\)\s*)?{name_pattern}\s*\(",
            re.MULTILINE,
        ),
        re.compile(
            rf"^cJSON \* {name_pattern}\s*\(",
            re.MULTILINE,
        ),
    ]
    for pat in patterns:
        m = pat.search(text, start_at)
        if m:
            return m.start()
    return None


def wrap_function(text: str, name_pattern: str, macro: str) -> str:
    pos = find_function_start(text, name_pattern)
    if pos is None:
        return text
    if pos > 0 and text[pos - 1] == "\n":
        before = text[:pos]
        if before.rstrip().endswith(f"#endif /* {macro} */"):
            return text
    brace = text.find("{", pos)
    if brace < 0:
        return text
    end = find_matching_brace(text, brace)
    block = text[pos : end + 1]
    if block.lstrip().startswith("#if"):
        return text
    wrapped = (
        f"#if !{macro}\n"
        f"{block}\n"
        f"#endif /* {macro} */\n"
    )
    return text[:pos] + wrapped + text[end + 1 :]


def wrap_header_declarations(text: str, guards: list[tuple[str, str]]) -> str:
    lines = text.splitlines(keepends=True)
    out: list[str] = []
    active: list[str] = []
    i = 0
    while i < len(lines):
        line = lines[i]
        matched = None
        for name, macro in guards:
            if re.search(name, line):
                matched = macro
                break
        if matched is None:
            out.append(line)
            i += 1
            continue

        block = [line]
        i += 1
        while i < len(lines):
            nxt = lines[i]
            if re.match(r"^\s*CJSON_PUBLIC", nxt) and not re.search(
                guards[0][0], nxt
            ):
                break
            block.append(nxt)
            if ";" in nxt:
                i += 1
                break
            i += 1
        body = "".join(block).rstrip()
        out.append(f"#if !{matched}\n")
        out.append(body + "\n")
        out.append(f"#endif /* {matched} */\n")
    return "".join(out)


def patch_parse_print_value(text: str) -> str:
    marker = "static cJSON_bool parse_value(cJSON * const item, parse_buffer * const input_buffer)"
    if "CJSON_DISABLE_ARRAYS" in text.split(marker, 1)[0][-200:]:
        return text

    text = text.replace(
        "    /* array */\n"
        "    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '['))\n"
        "    {\n"
        "        return parse_array(item, input_buffer);\n"
        "    }\n",
        "    /* array */\n"
        "#if !CJSON_DISABLE_ARRAYS\n"
        "    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '['))\n"
        "    {\n"
        "        return parse_array(item, input_buffer);\n"
        "    }\n"
        "#endif /* !CJSON_DISABLE_ARRAYS */\n",
    )

    replacements = [
        (
            "        case cJSON_NULL:\n"
            "            output = ensure(output_buffer, 5);\n"
            "            if (output == NULL)\n"
            "            {\n"
            "                return false;\n"
            "            }\n"
            "            strcpy((char*)output, \"null\");\n"
            "            return true;\n\n"
            "        case cJSON_False:\n"
            "            output = ensure(output_buffer, 6);\n"
            "            if (output == NULL)\n"
            "            {\n"
            "                return false;\n"
            "            }\n"
            "            strcpy((char*)output, \"false\");\n"
            "            return true;\n\n"
            "        case cJSON_True:\n"
            "            output = ensure(output_buffer, 5);\n"
            "            if (output == NULL)\n"
            "            {\n"
            "                return false;\n"
            "            }\n"
            "            strcpy((char*)output, \"true\");\n"
            "            return true;\n\n",
            "#if !CJSON_DISABLE_BOOL_NULL_PRINT\n"
            "        case cJSON_NULL:\n"
            "            output = ensure(output_buffer, 5);\n"
            "            if (output == NULL)\n"
            "            {\n"
            "                return false;\n"
            "            }\n"
            "            strcpy((char*)output, \"null\");\n"
            "            return true;\n\n"
            "        case cJSON_False:\n"
            "            output = ensure(output_buffer, 6);\n"
            "            if (output == NULL)\n"
            "            {\n"
            "                return false;\n"
            "            }\n"
            "            strcpy((char*)output, \"false\");\n"
            "            return true;\n\n"
            "        case cJSON_True:\n"
            "            output = ensure(output_buffer, 5);\n"
            "            if (output == NULL)\n"
            "            {\n"
            "                return false;\n"
            "            }\n"
            "            strcpy((char*)output, \"true\");\n"
            "            return true;\n\n"
            "#endif /* !CJSON_DISABLE_BOOL_NULL_PRINT */\n\n",
        ),
        (
            "        case cJSON_Raw:\n"
            "        {\n"
            "            size_t raw_length = 0;\n"
            "            if (item->valuestring == NULL)\n"
            "            {\n"
            "                return false;\n"
            "            }\n\n"
            "            raw_length = strlen(item->valuestring) + sizeof(\"\");\n"
            "            output = ensure(output_buffer, raw_length);\n"
            "            if (output == NULL)\n"
            "            {\n"
            "                return false;\n"
            "            }\n"
            "            memcpy(output, item->valuestring, raw_length);\n"
            "            return true;\n"
            "        }\n\n",
            "#if !CJSON_DISABLE_RAW\n"
            "        case cJSON_Raw:\n"
            "        {\n"
            "            size_t raw_length = 0;\n"
            "            if (item->valuestring == NULL)\n"
            "            {\n"
            "                return false;\n"
            "            }\n\n"
            "            raw_length = strlen(item->valuestring) + sizeof(\"\");\n"
            "            output = ensure(output_buffer, raw_length);\n"
            "            if (output == NULL)\n"
            "            {\n"
            "                return false;\n"
            "            }\n"
            "            memcpy(output, item->valuestring, raw_length);\n"
            "            return true;\n"
            "        }\n\n"
            "#endif /* !CJSON_DISABLE_RAW */\n\n",
        ),
        (
            "        case cJSON_Array:\n"
            "            return print_array(item, output_buffer);\n\n",
            "#if !CJSON_DISABLE_ARRAYS\n"
            "        case cJSON_Array:\n"
            "            return print_array(item, output_buffer);\n\n"
            "#endif /* !CJSON_DISABLE_ARRAYS */\n",
        ),
    ]
    for old, new in replacements:
        if old in text:
            text = text.replace(old, new)
    return text


def patch_print_object(text: str) -> str:
    if "CJSON_DISABLE_FORMATTED_PRINT" in text.split("static cJSON_bool print_object", 1)[1][:400]:
        return text
    return text.replace(
        "    length = (size_t) (output_buffer->format ? 2 : 1); /* fmt: {\\n */",
        "#if CJSON_DISABLE_FORMATTED_PRINT\n"
        "    length = 1;\n"
        "#else\n"
        "    length = (size_t) (output_buffer->format ? 2 : 1); /* fmt: {\\n */\n"
        "#endif",
        1,
    ).replace(
        "    if (output_buffer->format)\n"
        "    {\n"
        "        *output_pointer++ = '\\n';\n"
        "    }\n"
        "    output_buffer->offset += length;\n\n"
        "    while (current_item)\n"
        "    {\n"
        "        if (output_buffer->format)\n"
        "        {\n"
        "            size_t i;\n"
        "            output_pointer = ensure(output_buffer, output_buffer->depth);\n"
        "            if (output_pointer == NULL)\n"
        "            {\n"
        "                return false;\n"
        "            }\n"
        "            for (i = 0; i < output_buffer->depth; i++)\n"
        "            {\n"
        "                *output_pointer++ = '\\t';\n"
        "            }\n"
        "            output_buffer->offset += output_buffer->depth;\n"
        "        }\n\n",
        "#if !CJSON_DISABLE_FORMATTED_PRINT\n"
        "    if (output_buffer->format)\n"
        "    {\n"
        "        *output_pointer++ = '\\n';\n"
        "    }\n"
        "#endif\n"
        "    output_buffer->offset += length;\n\n"
        "    while (current_item)\n"
        "    {\n"
        "#if !CJSON_DISABLE_FORMATTED_PRINT\n"
        "        if (output_buffer->format)\n"
        "        {\n"
        "            size_t i;\n"
        "            output_pointer = ensure(output_buffer, output_buffer->depth);\n"
        "            if (output_pointer == NULL)\n"
        "            {\n"
        "                return false;\n"
        "            }\n"
        "            for (i = 0; i < output_buffer->depth; i++)\n"
        "            {\n"
        "                *output_pointer++ = '\\t';\n"
        "            }\n"
        "            output_buffer->offset += output_buffer->depth;\n"
        "        }\n"
        "#endif\n\n",
        1,
    )


def ensure_config_include_h(text: str) -> str:
    needle = '#ifndef cJSON__h\n#define cJSON__h\n'
    insert = (
        '#ifndef cJSON__h\n#define cJSON__h\n\n#include "cJSON_config.h"\n'
    )
    if '#include "cJSON_config.h"' in text:
        return text
    return text.replace(needle, insert, 1)


def ensure_config_nesting_h(text: str) -> str:
    old = (
        "#ifndef CJSON_NESTING_LIMIT\n"
        "#define CJSON_NESTING_LIMIT 1000\n"
        "#endif\n"
    )
    if old in text:
        return text.replace(old, "/* CJSON_NESTING_LIMIT from cJSON_config.h */\n", 1)
    return text


def patch_forward_decls(text: str) -> str:
    old = (
        "static cJSON_bool parse_array(cJSON * const item, parse_buffer * const input_buffer);\n"
        "static cJSON_bool print_array(const cJSON * const item, printbuffer * const output_buffer);\n"
    )
    new = (
        "#if !CJSON_DISABLE_ARRAYS\n"
        "static cJSON_bool parse_array(cJSON * const item, parse_buffer * const input_buffer);\n"
        "static cJSON_bool print_array(const cJSON * const item, printbuffer * const output_buffer);\n"
        "#endif /* !CJSON_DISABLE_ARRAYS */\n"
    )
    if old in text and new not in text:
        text = text.replace(old, new)
    return text


def main() -> int:
    c_text = CJSON_C.read_text(encoding="utf-8")
    h_text = CJSON_H.read_text(encoding="utf-8")

    h_text = ensure_config_include_h(h_text)
    h_text = ensure_config_nesting_h(h_text)

    c_text = patch_forward_decls(c_text)
    c_text = patch_parse_print_value(c_text)
    c_text = patch_print_object(c_text)

    for name, macro in reversed(C_GUARDS):
        c_text = wrap_function(c_text, name, macro)

    h_text = wrap_header_declarations(h_text, H_GUARDS)

    CJSON_C.write_text(c_text, encoding="utf-8", newline="\n")
    CJSON_H.write_text(h_text, encoding="utf-8", newline="\n")
    print(f"Patched {CJSON_C.name} and {CJSON_H.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
