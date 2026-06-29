cmake_minimum_required(VERSION 3.20)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/stm32-gcc-toolchain.cmake)

project(RTJK_001 C ASM)

set(TEMPLATE_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
option(HAL_DEBUG_UART_ENABLE "Enable GPIO bit-bang debug UART TX on PC13" ON)

# 必须用 CACHE STRING，不能用 option()——option 只支持 ON/OFF，CLion 会把版本解析成 BOOL=OFF
if(NOT DEFINED APP_VERSION AND DEFINED RTJK_VERSION)
    set(APP_VERSION ${RTJK_VERSION} CACHE STRING "Application version (1-10)")
else()
    set(APP_VERSION 10 CACHE STRING "Application version (1-10)")
endif()
if((APP_VERSION STREQUAL "OFF" OR APP_VERSION STREQUAL "ON" OR APP_VERSION STREQUAL "") AND DEFINED RTJK_VERSION)
    set(APP_VERSION ${RTJK_VERSION} CACHE STRING "Application version (1-10)" FORCE)
endif()
if(APP_VERSION STREQUAL "OFF" OR APP_VERSION STREQUAL "ON" OR APP_VERSION STREQUAL "")
    set(APP_VERSION 10 CACHE STRING "Application version (1-10)" FORCE)
endif()
set(RTJK_VERSION ${APP_VERSION} CACHE STRING "Legacy RTJK firmware version alias" FORCE)
if(APP_VERSION LESS 1 OR APP_VERSION GREATER 10)
    message(FATAL_ERROR "APP_VERSION must be 1-10, got '${APP_VERSION}'")
endif()

if(APP_VERSION GREATER_EQUAL 8)
    set(HAL_ADC_ENABLE ON CACHE BOOL "Enable ADC for MSP20 blood pressure" FORCE)
endif()

include(${TEMPLATE_ROOT}/cmake/hal_options.cmake)
include(${TEMPLATE_ROOT}/cmake/driver_catalog.cmake)

set(LINKER_SCRIPT ${TEMPLATE_ROOT}/bsp/linker_scripts/STM32F103XX_FLASH.ld)
set(STARTUP_FILE ${TEMPLATE_ROOT}/bsp/startup/startup_stm32f103xb.s)

set(CPU_FLAGS
    -mcpu=cortex-m3
    -mthumb
)

set(COMMON_SRCS
    ${TEMPLATE_ROOT}/common/device_manager/devmgr.c
    ${TEMPLATE_ROOT}/common/device_manager/driver_registry_gcc.c
    ${TEMPLATE_ROOT}/common/scheduler/scheduler.c
    ${TEMPLATE_ROOT}/common/scheduler/sched_loop.c
    ${TEMPLATE_ROOT}/common/irq_event/irq_event.c
    ${TEMPLATE_ROOT}/common/utils/tiny_printf.c
    ${TEMPLATE_ROOT}/common/utils/soft_uart.c
    ${TEMPLATE_ROOT}/common/utils/debug_uart.c
    ${TEMPLATE_ROOT}/common/utils/mem_pool.c
    ${TEMPLATE_ROOT}/common/utils/cJSON.c
    ${TEMPLATE_ROOT}/common/utils/cjson_port.c
    ${TEMPLATE_ROOT}/common/utils/comm_port.c
)

set(BSP_SRCS
    ${TEMPLATE_ROOT}/bsp/syscalls/syscalls.c
    ${TEMPLATE_ROOT}/bsp/hal_wrapper/hal_common.c
    ${TEMPLATE_ROOT}/bsp/hal_wrapper/gpio_hal.c
    ${TEMPLATE_ROOT}/bsp/hal_wrapper/exti_hal.c
    ${BSP_HAL_I2C_SRC}
    ${TEMPLATE_ROOT}/bsp/hal_wrapper/usart_hal.c
    ${BSP_HAL_USART_DMA_SRC}
    ${BSP_HAL_SPI_SRC}
    ${BSP_HAL_SPI_DMA_SRC}
    ${BSP_HAL_ADC_SRC}
    ${BSP_HAL_ADC_DMA_SRC}
    ${TEMPLATE_ROOT}/bsp/hal_wrapper/timer_hal.c
    ${TEMPLATE_ROOT}/bsp/irq_handlers/irq_handlers.c
)

set(DRIVER_SRCS ${DRIVER_CATALOG_RTJK_001})

set(SPL_SRCS
    ${TEMPLATE_ROOT}/bsp/stm32f10x_std_lib/Libraries/CMSIS/system_stm32f10x.c
    ${TEMPLATE_ROOT}/bsp/stm32f10x_std_lib/Libraries/FWlib/src/misc.c
    ${TEMPLATE_ROOT}/bsp/stm32f10x_std_lib/Libraries/FWlib/src/stm32f10x_gpio.c
    ${TEMPLATE_ROOT}/bsp/stm32f10x_std_lib/Libraries/FWlib/src/stm32f10x_exti.c
    ${SPL_HAL_I2C_SRC}
    ${SPL_HAL_SPI_SRC}
    ${SPL_HAL_ADC_SRC}
    ${SPL_HAL_DMA_SRC}
    ${TEMPLATE_ROOT}/bsp/stm32f10x_std_lib/Libraries/FWlib/src/stm32f10x_rcc.c
    ${TEMPLATE_ROOT}/bsp/stm32f10x_std_lib/Libraries/FWlib/src/stm32f10x_tim.c
    ${TEMPLATE_ROOT}/bsp/stm32f10x_std_lib/Libraries/FWlib/src/stm32f10x_usart.c
)

set(APP_SRCS
    app/app_main.c
    app/app_logic.c
    app/app_callbacks.c
)

add_executable(${PROJECT_NAME}.elf
    ${APP_SRCS}
    ${COMMON_SRCS}
    ${BSP_SRCS}
    ${DRIVER_SRCS}
    ${SPL_SRCS}
    ${STARTUP_FILE}
)

set_target_properties(${PROJECT_NAME}.elf PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    SUFFIX ""
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    app
    ${TEMPLATE_ROOT}/common/interfaces
    ${TEMPLATE_ROOT}/common/driver_core
    ${TEMPLATE_ROOT}/common/device_manager
    ${TEMPLATE_ROOT}/common/scheduler
    ${TEMPLATE_ROOT}/common/irq_event
    ${TEMPLATE_ROOT}/common/utils
    ${TEMPLATE_ROOT}/drivers/comm
    ${TEMPLATE_ROOT}/bsp/board
    ${TEMPLATE_ROOT}/bsp/hal_wrapper
    ${TEMPLATE_ROOT}/bsp/stm32f10x_std_lib/Libraries/CMSIS
    ${TEMPLATE_ROOT}/bsp/stm32f10x_std_lib/Libraries/FWlib/inc
)

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    STM32F10X_MD
    USE_STDPERIPH_DRIVER
    CJSON_NESTING_LIMIT=32
    APP_VERSION=${APP_VERSION}
    RTJK_VERSION=${APP_VERSION}
)

hal_apply_compile_definitions(${PROJECT_NAME}.elf)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    ${CPU_FLAGS}
    $<$<COMPILE_LANGUAGE:C>:-Wall>
    $<$<COMPILE_LANGUAGE:C>:-Wextra>
    $<$<COMPILE_LANGUAGE:C>:-Os>
    $<$<COMPILE_LANGUAGE:C>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:C>:-fdata-sections>
    $<$<COMPILE_LANGUAGE:ASM>:-x>
    $<$<COMPILE_LANGUAGE:ASM>:assembler-with-cpp>
)

target_link_libraries(${PROJECT_NAME}.elf PRIVATE m)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    ${CPU_FLAGS}
    -T${LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--print-memory-usage
    -Wl,-u,malloc
    -Wl,-u,_malloc_r
    -Wl,-u,cjson_port_init
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
    BYPRODUCTS ${PROJECT_NAME}.bin ${PROJECT_NAME}.map
)
