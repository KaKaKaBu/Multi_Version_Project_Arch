#ifndef HAL_FEATURES_H
#define HAL_FEATURES_H

/*
 * HAL 功能开关：可在 board_config.h 或 CMake target_compile_definitions 中覆盖。
 *
 * HAL_I2C_USE_SOFT       1 = 软件 I2C（GPIO bit-bang），0 = 硬件 I2C（SPL）
 * HAL_SPI_USE_SOFT       1 = 软件 SPI（GPIO bit-bang）
 * HAL_SPI_USE_HW         1 = 硬件 SPI（SPL）；与 HAL_SPI_USE_SOFT 互斥
 * HAL_SPI_ENABLE_DMA     1 = 硬件 SPI DMA TX/RX（需 HAL_SPI_USE_HW）
 * HAL_USART_ENABLE_DMA   1 = USART DMA TX
 * HAL_ADC_ENABLE         1 = ADC 轮询采集
 * HAL_ADC_ENABLE_DMA     1 = ADC DMA 连续采集（需 HAL_ADC_ENABLE）
 * HAL_DEBUG_UART_ENABLE  1 = GPIO 软串口调试输出（printf → PC13 等）
 */

#ifndef HAL_I2C_USE_SOFT
#define HAL_I2C_USE_SOFT 0
#endif

#ifndef HAL_SPI_USE_SOFT
#define HAL_SPI_USE_SOFT 0
#endif

#ifndef HAL_SPI_USE_HW
#define HAL_SPI_USE_HW 0
#endif

#ifndef HAL_SPI_ENABLE_DMA
#define HAL_SPI_ENABLE_DMA 0
#endif

#ifndef HAL_USART_ENABLE_DMA
#define HAL_USART_ENABLE_DMA 0
#endif

#ifndef HAL_ADC_ENABLE
#define HAL_ADC_ENABLE 0
#endif

#ifndef HAL_ADC_ENABLE_DMA
#define HAL_ADC_ENABLE_DMA 0
#endif

#ifndef HAL_DEBUG_UART_ENABLE
#define HAL_DEBUG_UART_ENABLE 0
#endif

#endif
