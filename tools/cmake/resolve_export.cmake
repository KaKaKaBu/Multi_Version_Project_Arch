# 统一导出解析入口：HAL 源/定义 + driver_catalog 驱动列表
# 由 export_project.py 调用，避免 Python 侧重复 CMake 逻辑。
#
# 必需：TEMPLATE_ROOT
# 可选：CATALOG_NAME, PROJECT_HAL_PREAMBLE, 以及工程版本/cache 变量（-DAPP_VERSION=10 等）

cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED TEMPLATE_ROOT)
    message(FATAL_ERROR "TEMPLATE_ROOT is required")
endif()

set(_hal_utils "${CMAKE_CURRENT_LIST_DIR}/hal_export_utils.cmake")
if(NOT EXISTS "${_hal_utils}")
    message(FATAL_ERROR "Missing hal_export_utils.cmake: ${_hal_utils}")
endif()
include("${_hal_utils}")

set(_hal_options_file "${TEMPLATE_ROOT}/cmake/hal_options.cmake")
hal_export_scan_metadata("${_hal_options_file}" _hal_options _hal_src_groups)

if(DEFINED PROJECT_HAL_PREAMBLE)
    if(EXISTS "${PROJECT_HAL_PREAMBLE}")
        include("${PROJECT_HAL_PREAMBLE}")
    else()
        message(FATAL_ERROR "PROJECT_HAL_PREAMBLE not found: ${PROJECT_HAL_PREAMBLE}")
    endif()
endif()

include("${_hal_options_file}")

hal_export_emit("${_hal_options}" "${_hal_src_groups}")

if(DEFINED CATALOG_NAME)
    include(${TEMPLATE_ROOT}/cmake/driver_catalog.cmake)
    set(_catalog_var "DRIVER_CATALOG_${CATALOG_NAME}")
    if(NOT DEFINED ${_catalog_var})
        message(FATAL_ERROR "Unknown driver catalog: DRIVER_CATALOG_${CATALOG_NAME}")
    endif()
    set(_drivers ${${_catalog_var}})
    list(REMOVE_DUPLICATES _drivers)
    foreach(_item IN LISTS _drivers)
        message("EXPORT_DRIVER:${_item}")
    endforeach()
endif()
