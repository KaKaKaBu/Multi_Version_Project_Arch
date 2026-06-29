/* ZNLYJ_001 标准外设库裁剪配置：声明本项目需要包含的 SPL 头文件。 */
#ifndef STM32F10X_CONF_H
#define STM32F10X_CONF_H

#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_adc.h"
#include "misc.h"

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0)
#endif

#endif
