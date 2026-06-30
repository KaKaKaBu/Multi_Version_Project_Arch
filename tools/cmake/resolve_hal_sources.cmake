# 由 export_project.py 以脚本模式调用，解析 hal_options.cmake 中的 HAL 源文件与开关。
# 可选 PROJECT_HAL_PREAMBLE：工程 CMakeLists 在 include(hal_options) 之前的条件逻辑片段。
#
# 用法：
#   cmake -DTEMPLATE_ROOT=... -DAPP_VERSION=10 \
#         -DPROJECT_HAL_PREAMBLE=.../hal_preamble.cmake \
#         -P resolve_hal_sources.cmake

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
