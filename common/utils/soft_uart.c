#include "soft_uart.h"
#include "hal_common.h"
#include "hal_features.h"

#if HAL_DEBUG_UART_ENABLE

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

#define SOFT_UART_DWT_CYCCNT (*(volatile uint32_t *)0xE0001004U)

static GPIO_TypeDef *soft_uart_port;
static uint16_t soft_uart_pin;
static uint32_t soft_uart_bit_cycles;

static uint32_t soft_uart_enter_critical(void)
{
    uint32_t primask;

    __asm volatile ("mrs %0, primask" : "=r" (primask));
    __asm volatile ("cpsid i" : : : "memory");
    return primask;
}

static void soft_uart_exit_critical(uint32_t primask)
{
    if ((primask & 1U) == 0U) {
        __asm volatile ("cpsie i" : : : "memory");
    }
}

static void soft_uart_pin_write(uint8_t level)
{
    if (level != 0U) {
        GPIO_SetBits(soft_uart_port, soft_uart_pin);
    } else {
        GPIO_ResetBits(soft_uart_port, soft_uart_pin);
    }
}

static void soft_uart_delay_one_bit(void)
{
    uint32_t start = SOFT_UART_DWT_CYCCNT;

    while ((SOFT_UART_DWT_CYCCNT - start) < soft_uart_bit_cycles) {
    }
}

static void soft_uart_config_tx_pin(const hal_pin_t *pin)
{
    GPIO_InitTypeDef gpio;

    gpio_hal_clock_enable(pin->port);
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = pin->pin;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = (pin->port == GPIOC && pin->pin == GPIO_Pin_13) ?
                      GPIO_Speed_2MHz : GPIO_Speed_50MHz;
    GPIO_Init(pin->port, &gpio);
}

void soft_uart_init(const soft_uart_config_t *cfg)
{
    if ((cfg == 0) || (cfg->baudrate == 0U)) {
        return;
    }

    soft_uart_port = cfg->tx_pin.port;
    soft_uart_pin = cfg->tx_pin.pin;
    soft_uart_bit_cycles = SystemCoreClock / cfg->baudrate;
    if (soft_uart_bit_cycles == 0U) {
        soft_uart_bit_cycles = 1U;
    }

    soft_uart_config_tx_pin(&cfg->tx_pin);
    soft_uart_pin_write(1U);
}

void soft_uart_write_byte(uint8_t byte)
{
    uint32_t primask;
    uint8_t bit_index;

    if (soft_uart_port == 0) {
        return;
    }

    primask = soft_uart_enter_critical();

    soft_uart_pin_write(0U);
    soft_uart_delay_one_bit();

    for (bit_index = 0U; bit_index < 8U; ++bit_index) {
        soft_uart_pin_write((uint8_t)(byte & 0x01U));
        soft_uart_delay_one_bit();
        byte = (uint8_t)(byte >> 1U);
    }

    soft_uart_pin_write(1U);
    soft_uart_delay_one_bit();

    soft_uart_exit_critical(primask);
}

void soft_uart_write(const uint8_t *data, unsigned short len)
{
    unsigned short index;

    if ((data == 0) || (len == 0U)) {
        return;
    }

    for (index = 0U; index < len; ++index) {
        soft_uart_write_byte(data[index]);
    }
}

void soft_uart_write_str(const char *str)
{
    if (str == 0) {
        return;
    }

    while (*str != '\0') {
        soft_uart_write_byte((uint8_t)*str);
        ++str;
    }
}

#else

void soft_uart_init(const soft_uart_config_t *cfg)
{
    (void)cfg;
}

void soft_uart_write_byte(uint8_t byte)
{
    (void)byte;
}

void soft_uart_write(const uint8_t *data, unsigned short len)
{
    (void)data;
    (void)len;
}

void soft_uart_write_str(const char *str)
{
    (void)str;
}

#endif
