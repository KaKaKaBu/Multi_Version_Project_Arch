include_guard(GLOBAL)

get_filename_component(MVP_TEMPLATE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

function(mvp_resolve_app_version)
    set(options FORCE_DEFAULT)
    set(one_value_args DEFAULT MIN MAX)
    cmake_parse_arguments(MVP "${options}" "${one_value_args}" "" ${ARGN})

    if(NOT DEFINED MVP_DEFAULT OR NOT DEFINED MVP_MIN OR NOT DEFINED MVP_MAX)
        message(FATAL_ERROR "mvp_resolve_app_version requires DEFAULT, MIN and MAX")
    endif()

    if(MVP_FORCE_DEFAULT)
        set(_mvp_app_version "${MVP_DEFAULT}")
    elseif(NOT DEFINED APP_VERSION OR APP_VERSION STREQUAL "" OR APP_VERSION STREQUAL "OFF" OR APP_VERSION STREQUAL "ON")
        if(DEFINED MVP_APP_VERSION AND NOT MVP_APP_VERSION STREQUAL "" AND NOT MVP_APP_VERSION STREQUAL "OFF" AND NOT MVP_APP_VERSION STREQUAL "ON")
            set(_mvp_app_version "${MVP_APP_VERSION}")
        else()
            set(_mvp_app_version "${MVP_DEFAULT}")
        endif()
    else()
        set(_mvp_app_version "${APP_VERSION}")
    endif()

    if(NOT _mvp_app_version MATCHES "^[0-9]+$")
        message(FATAL_ERROR "APP_VERSION must be ${MVP_MIN}-${MVP_MAX}, got '${_mvp_app_version}'")
    endif()

    if(_mvp_app_version LESS MVP_MIN OR _mvp_app_version GREATER MVP_MAX)
        message(FATAL_ERROR "APP_VERSION must be ${MVP_MIN}-${MVP_MAX}, got '${_mvp_app_version}'")
    endif()

    if(DEFINED CACHE{APP_VERSION})
        unset(APP_VERSION CACHE)
    endif()
    set(APP_VERSION "${_mvp_app_version}" PARENT_SCOPE)
endfunction()

function(mvp_add_stm32f1_project target_name)
    set(options ENABLE_APP_FRAMEWORK)
    set(one_value_args DRIVER_CATALOG_VAR)
    set(multi_value_args EXTRA_DEFINES)
    cmake_parse_arguments(MVP "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT DEFINED MVP_DRIVER_CATALOG_VAR)
        message(FATAL_ERROR "mvp_add_stm32f1_project requires DRIVER_CATALOG_VAR")
    endif()

    set(TEMPLATE_ROOT "${MVP_TEMPLATE_ROOT}")
    include("${TEMPLATE_ROOT}/cmake/hal_options.cmake")
    include("${TEMPLATE_ROOT}/cmake/driver_catalog.cmake")

    if(NOT DEFINED ${MVP_DRIVER_CATALOG_VAR})
        message(FATAL_ERROR "Driver catalog variable '${MVP_DRIVER_CATALOG_VAR}' is not defined")
    endif()

    set(LINKER_SCRIPT "${TEMPLATE_ROOT}/bsp/stm32/linker_scripts/STM32F103XX_FLASH.ld")
    set(STARTUP_FILE "${TEMPLATE_ROOT}/bsp/stm32/startup/startup_stm32f103xb.s")

    set(CPU_FLAGS
        -mcpu=cortex-m3
        -mthumb
    )

    set(COMMON_SRCS
        "${TEMPLATE_ROOT}/common/device_manager/devmgr.c"
        "${TEMPLATE_ROOT}/common/device_manager/driver_registry_gcc.c"
        "${TEMPLATE_ROOT}/common/scheduler/scheduler.c"
        "${TEMPLATE_ROOT}/common/scheduler/sched_loop.c"
        "${TEMPLATE_ROOT}/common/irq_event/irq_event.c"
        "${TEMPLATE_ROOT}/common/utils/tiny_printf.c"
        "${TEMPLATE_ROOT}/common/utils/mem_pool.c"
        "${TEMPLATE_ROOT}/common/utils/cJSON.c"
        "${TEMPLATE_ROOT}/common/utils/cjson_port.c"
        "${TEMPLATE_ROOT}/common/utils/soft_uart.c"
        "${TEMPLATE_ROOT}/common/utils/debug_uart.c"
        "${TEMPLATE_ROOT}/common/utils/comm_port.c"
        "${TEMPLATE_ROOT}/common/interfaces/display_font.c"
    )

    if(MVP_ENABLE_APP_FRAMEWORK)
        list(APPEND COMMON_SRCS "${TEMPLATE_ROOT}/common/app_framework/app_fsm.c")
    endif()

    set(BSP_SRCS
        "${TEMPLATE_ROOT}/bsp/stm32/syscalls/syscalls.c"
        "${TEMPLATE_ROOT}/bsp/stm32/hal/stm32f1_hal_map.c"
        "${TEMPLATE_ROOT}/bsp/stm32/hal/hal_common.c"
        "${TEMPLATE_ROOT}/bsp/stm32/hal/gpio_hal.c"
        "${TEMPLATE_ROOT}/bsp/stm32/hal/exti_hal.c"
        ${BSP_HAL_I2C_SRC}
        ${BSP_HAL_SPI_SRC}
        "${TEMPLATE_ROOT}/bsp/stm32/hal/usart_hal.c"
        ${BSP_HAL_USART_DMA_SRC}
        ${BSP_HAL_ADC_SRC}
        ${BSP_HAL_ADC_DMA_SRC}
        "${TEMPLATE_ROOT}/bsp/stm32/hal/timer_hal.c"
        "${TEMPLATE_ROOT}/bsp/stm32/irq_handlers/irq_handlers.c"
    )

    set(SPL_SRCS
        "${TEMPLATE_ROOT}/bsp/stm32/vendor/Libraries/CMSIS/system_stm32f10x.c"
        "${TEMPLATE_ROOT}/bsp/stm32/vendor/Libraries/FWlib/src/misc.c"
        "${TEMPLATE_ROOT}/bsp/stm32/vendor/Libraries/FWlib/src/stm32f10x_gpio.c"
        "${TEMPLATE_ROOT}/bsp/stm32/vendor/Libraries/FWlib/src/stm32f10x_exti.c"
        ${SPL_HAL_I2C_SRC}
        ${SPL_HAL_SPI_SRC}
        "${TEMPLATE_ROOT}/bsp/stm32/vendor/Libraries/FWlib/src/stm32f10x_rcc.c"
        "${TEMPLATE_ROOT}/bsp/stm32/vendor/Libraries/FWlib/src/stm32f10x_tim.c"
        "${TEMPLATE_ROOT}/bsp/stm32/vendor/Libraries/FWlib/src/stm32f10x_usart.c"
        ${SPL_HAL_ADC_SRC}
        ${SPL_HAL_DMA_SRC}
    )

    set(APP_SRCS
        app/app_main.c
        app/app_callbacks.c
        app/app_logic.c
        board/board_devices.c
    )

    add_executable(${target_name}.elf
        ${APP_SRCS}
        ${COMMON_SRCS}
        ${BSP_SRCS}
        ${${MVP_DRIVER_CATALOG_VAR}}
        ${SPL_SRCS}
        ${STARTUP_FILE}
    )

    set_target_properties(${target_name}.elf PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
        SUFFIX ""
    )

    target_include_directories(${target_name}.elf PRIVATE
        board
        app
        "${TEMPLATE_ROOT}/common/interfaces"
        "${TEMPLATE_ROOT}/common/driver_core"
        "${TEMPLATE_ROOT}/common/device_manager"
        "${TEMPLATE_ROOT}/common/scheduler"
        "${TEMPLATE_ROOT}/common/app_framework"
        "${TEMPLATE_ROOT}/common/irq_event"
        "${TEMPLATE_ROOT}/common/utils"
        "${TEMPLATE_ROOT}/drivers/comm"
        "${TEMPLATE_ROOT}/drivers/displays"
        "${TEMPLATE_ROOT}/drivers/config"
        "${TEMPLATE_ROOT}/bsp/board"
        "${TEMPLATE_ROOT}/hal_wrapper"
        "${TEMPLATE_ROOT}/bsp/stm32/hal"
        "${TEMPLATE_ROOT}/bsp/stm32/vendor/Libraries/CMSIS"
        "${TEMPLATE_ROOT}/bsp/stm32/vendor/Libraries/FWlib/inc"
    )

    target_compile_definitions(${target_name}.elf PRIVATE
        STM32F10X_MD
        USE_STDPERIPH_DRIVER
        CJSON_NESTING_LIMIT=32
        APP_VERSION=${APP_VERSION}
        ${MVP_EXTRA_DEFINES}
    )

    hal_apply_compile_definitions(${target_name}.elf)

    target_compile_options(${target_name}.elf PRIVATE
        ${CPU_FLAGS}
        $<$<COMPILE_LANGUAGE:C>:-Wall>
        $<$<COMPILE_LANGUAGE:C>:-Wextra>
        $<$<COMPILE_LANGUAGE:C>:-Os>
        $<$<COMPILE_LANGUAGE:C>:-ffunction-sections>
        $<$<COMPILE_LANGUAGE:C>:-fdata-sections>
        $<$<COMPILE_LANGUAGE:ASM>:-x>
        $<$<COMPILE_LANGUAGE:ASM>:assembler-with-cpp>
    )

    target_link_libraries(${target_name}.elf PRIVATE m)

    target_link_options(${target_name}.elf PRIVATE
        ${CPU_FLAGS}
        -T${LINKER_SCRIPT}
        -Wl,--gc-sections
        -Wl,-Map=${target_name}.map
        -Wl,--print-memory-usage
        -Wl,-u,malloc
        -Wl,-u,_malloc_r
        -Wl,-u,cjson_port_init
    )

    add_custom_command(TARGET ${target_name}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target_name}.elf> ${target_name}.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${target_name}.elf>
        BYPRODUCTS ${target_name}.bin ${target_name}.map
    )
endfunction()

function(mvp_add_mcs51_polling_project target_name)
    set(one_value_args DRIVER_CATALOG_VAR)
    cmake_parse_arguments(MVP "" "${one_value_args}" "" ${ARGN})

    if(NOT DEFINED MVP_DRIVER_CATALOG_VAR)
        message(FATAL_ERROR "mvp_add_mcs51_polling_project requires DRIVER_CATALOG_VAR")
    endif()

    set(TEMPLATE_ROOT "${MVP_TEMPLATE_ROOT}")
    include("${TEMPLATE_ROOT}/cmake/driver_catalog.cmake")

    if(NOT DEFINED ${MVP_DRIVER_CATALOG_VAR})
        message(FATAL_ERROR "Driver catalog variable '${MVP_DRIVER_CATALOG_VAR}' is not defined")
    endif()

    add_executable(${target_name}
        app/app_main_mcs51.c
        app/app_logic.c
        app/driver_registry_mcs51.c
        "${TEMPLATE_ROOT}/common/device_manager/devmgr.c"
        "${TEMPLATE_ROOT}/bsp/mcs51/hal/hal_common.c"
        "${TEMPLATE_ROOT}/bsp/mcs51/hal/gpio_hal.c"
        "${TEMPLATE_ROOT}/bsp/mcs51/hal/i2c_hal_soft.c"
        ${${MVP_DRIVER_CATALOG_VAR}}
    )

    set_target_properties(${target_name} PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
    )

    target_include_directories(${target_name} PRIVATE
        board
        app
        "${TEMPLATE_ROOT}/common/interfaces"
        "${TEMPLATE_ROOT}/common/driver_core"
        "${TEMPLATE_ROOT}/common/device_manager"
        "${TEMPLATE_ROOT}/common/scheduler"
        "${TEMPLATE_ROOT}/common/irq_event"
        "${TEMPLATE_ROOT}/common/utils"
        "${TEMPLATE_ROOT}/drivers/displays"
        "${TEMPLATE_ROOT}/drivers/sensors"
        "${TEMPLATE_ROOT}/drivers/actuators"
        "${TEMPLATE_ROOT}/drivers/misc"
        "${TEMPLATE_ROOT}/drivers/config"
        "${TEMPLATE_ROOT}/hal_wrapper"
        "${TEMPLATE_ROOT}/bsp/mcs51/hal"
    )

    target_compile_definitions(${target_name} PRIVATE
        PLATFORM_MCS51=1
        DRIVER_REGISTRY_STATIC=1
        APP_VERSION=${APP_VERSION}
        HAL_I2C_USE_SOFT=1
        HAL_DEBUG_UART_ENABLE=0
        HAL_USART_ENABLE=0
        HAL_ADC_ENABLE=0
        HAL_ADC_ENABLE_DMA=0
        HAL_USART_ENABLE_DMA=0
        HAL_SPI_ENABLE=0
    )

    target_compile_options(${target_name} PRIVATE
        $<$<COMPILE_LANGUAGE:C>:--model-small>
        $<$<COMPILE_LANGUAGE:C>:--stack-auto>
        $<$<COMPILE_LANGUAGE:C>:--opt-code-size>
    )

    target_link_options(${target_name} PRIVATE
        --model-small
        --stack-auto
        --iram-size
        256
        --stack-size
        64
    )
endfunction()
