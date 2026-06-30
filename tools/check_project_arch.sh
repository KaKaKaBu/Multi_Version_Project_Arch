#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

if command -v rg >/dev/null 2>&1; then
    SEARCH="rg"
else
    SEARCH="grep -R"
fi

status=0

for spec in \
    "$ROOT_DIR/projects/WXCS_001/PRODUCT_SPEC.md" \
    "$ROOT_DIR/projects/ZNLYJ_001/PRODUCT_SPEC.md" \
    "$ROOT_DIR/projects/SMZL_001/PRODUCT_SPEC.md"
do
    if ! grep -q "引脚定义" "$spec"; then
        echo "missing pin definition chapter: $spec" >&2
        status=1
    fi
done

if $SEARCH -n "#include[[:space:]]+[<\"]stm32|stm32f10x_|GPIO_TypeDef|GPIO_Pin_|GPIO_PortSource|ADC_Channel_|ADC_SampleTime|RCC_|\\bUSART[0-9]\\b|\\bI2C[0-9]\\b|\\bTIM[0-9]\\b|\\bDMA[0-9]_" "$ROOT_DIR/drivers"; then
    echo "drivers contain chip-specific STM32F1 headers or symbols" >&2
    status=1
fi

if $SEARCH -n "#include[[:space:]]+[<\"]board_config\\.h" "$ROOT_DIR/drivers"; then
    echo "drivers include board_config.h; board-specific config must stay under projects/<name>/board" >&2
    status=1
fi

if find "$ROOT_DIR/projects" -path "*/app/board_config.h" -type f | grep -q .; then
    find "$ROOT_DIR/projects" -path "*/app/board_config.h" -type f >&2
    echo "board_config.h must live under projects/<name>/board, not app/" >&2
    status=1
fi

for cmake_file in "$ROOT_DIR"/projects/*/CMakeLists.txt; do
    [ -f "$cmake_file" ] || continue
    project_dir=$(dirname "$cmake_file")
    if grep -q "mvp_add_stm32f1_project" "$cmake_file" && [ ! -f "$project_dir/board/board_devices.c" ]; then
        echo "missing explicit board device table: $project_dir/board/board_devices.c" >&2
        status=1
    fi
done

if $SEARCH -n "board_devices_template|bsp/hal_wrapper|bsp/irq_handlers|stm32f10x_std_lib" "$ROOT_DIR/projects" "$ROOT_DIR/cmake"; then
    echo "found legacy board template or old BSP path references" >&2
    status=1
fi

exit "$status"
