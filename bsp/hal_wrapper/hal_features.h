/**
 * @file hal_features.h
 * @brief Compile-time HAL feature toggles (overridable via CMake or board_config.h).
 *
 * HAL_I2C_USE_SOFT       1 = software I2C (GPIO bit-bang), 0 = hardware I2C (SPL).
 * HAL_SPI_USE_SOFT       1 = software SPI (GPIO bit-bang).
 * HAL_SPI_USE_HW         1 = hardware SPI (SPL); mutually exclusive with HAL_SPI_USE_SOFT.
 * HAL_SPI_ENABLE_DMA     1 = hardware SPI DMA TX/RX (requires HAL_SPI_USE_HW).
 * HAL_USART_ENABLE_DMA   1 = USART DMA TX.
 * HAL_ADC_ENABLE         1 = ADC polling acquisition.
 * HAL_ADC_ENABLE_DMA     1 = ADC DMA continuous acquisition (requires HAL_ADC_ENABLE).
 * HAL_DEBUG_UART_ENABLE  1 = GPIO bit-bang debug UART TX (printf output).
 */

#ifndef HAL_FEATURES_H
#define HAL_FEATURES_H

/** @def HAL_I2C_USE_SOFT
 *  @brief 1 = software I2C (GPIO bit-bang), 0 = hardware I2C (SPL). */
#ifndef HAL_I2C_USE_SOFT
#define HAL_I2C_USE_SOFT 0
#endif

/** @def HAL_SPI_USE_SOFT
 *  @brief 1 = compile software SPI (GPIO bit-bang). */
#ifndef HAL_SPI_USE_SOFT
#define HAL_SPI_USE_SOFT 0
#endif

/** @def HAL_SPI_USE_HW
 *  @brief 1 = compile hardware SPI (SPL); mutually exclusive with HAL_SPI_USE_SOFT. */
#ifndef HAL_SPI_USE_HW
#define HAL_SPI_USE_HW 0
#endif

/** @def HAL_SPI_ENABLE_DMA
 *  @brief 1 = hardware SPI DMA TX/RX (requires HAL_SPI_USE_HW). */
#ifndef HAL_SPI_ENABLE_DMA
#define HAL_SPI_ENABLE_DMA 0
#endif

/** @def HAL_USART_ENABLE_DMA
 *  @brief 1 = USART DMA TX support. */
#ifndef HAL_USART_ENABLE_DMA
#define HAL_USART_ENABLE_DMA 0
#endif

/** @def HAL_ADC_ENABLE
 *  @brief 1 = ADC polling acquisition. */
#ifndef HAL_ADC_ENABLE
#define HAL_ADC_ENABLE 0
#endif

/** @def HAL_ADC_ENABLE_DMA
 *  @brief 1 = ADC DMA continuous acquisition (requires HAL_ADC_ENABLE). */
#ifndef HAL_ADC_ENABLE_DMA
#define HAL_ADC_ENABLE_DMA 0
#endif

/** @def HAL_DEBUG_UART_ENABLE
 *  @brief 1 = GPIO bit-bang debug UART TX (printf via _write). */
#ifndef HAL_DEBUG_UART_ENABLE
#define HAL_DEBUG_UART_ENABLE 0
#endif

#endif
