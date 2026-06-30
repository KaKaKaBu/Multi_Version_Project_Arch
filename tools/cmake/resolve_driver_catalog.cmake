# 由 export_project.py 以脚本模式调用，解析 driver_catalog.cmake 中的版本驱动列表。
# 用法：
#   cmake -DTEMPLATE_ROOT=... -DCATALOG_NAME=RTJK_001 -DAPP_VERSION=10 -P resolve_driver_catalog.cmake

cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED TEMPLATE_ROOT)
    message(FATAL_ERROR "TEMPLATE_ROOT is required")
endif()

if(NOT DEFINED CATALOG_NAME)
    message(FATAL_ERROR "CATALOG_NAME is required")
endif()

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
