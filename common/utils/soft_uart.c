/**
 * @file soft_uart.c
 * @brief GPIO 位bang UART：DWT 忙等位时间，8N1 帧格式，临界区保护整帧发送。
 *
 * soft_uart_bit_cycles = SystemCoreClock / baudrate；发送期间 cpsid i 防止 ISR 拉长位宽。
 */

#include "soft_uart.h"
#include "hal_common.h"
#include "hal_features.h"

#if HAL_DEBUG_UART_ENABLE

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

/** @brief 用于位时间延时的 DWT 周期计数寄存器。 */
#define SOFT_UART_DWT_CYCCNT (*(volatile uint32_t *)0xE0001004U)

static GPIO_TypeDef *soft_uart_port;  ///< 初始化后的 TX GPIO 端口。
static uint16_t soft_uart_pin;        ///< 初始化后的 TX 引脚掩码。
static uint32_t soft_uart_bit_cycles; ///< 当前波特率下每位对应的 DWT 周期数。

/**
 * @brief 关闭 IRQ 并返回先前的 PRIMASK 值。
 * @return 进入临界区前的 PRIMASK 寄存器值。
 */
static uint32_t soft_uart_enter_critical(void)
{
    uint32_t primask;

    __asm volatile ("mrs %0, primask" : "=r" (primask));
    __asm volatile ("cpsid i" : : : "memory");
    return primask;
}

/**
 * @brief 根据 soft_uart_enter_critical 保存的值恢复 IRQ 使能状态。
 * @param[in] primask soft_uart_enter_critical 返回的 PRIMASK 值。
 */
static void soft_uart_exit_critical(uint32_t primask)
{
    if ((primask & 1U) == 0U) {
        __asm volatile ("cpsie i" : : : "memory");
    }
}

/**
 * @brief 驱动已配置的 TX GPIO 输出高或低。
 * @param[in] level 非零为 mark（高电平），零为 space（低电平）。
 */
static void soft_uart_pin_write(uint8_t level)
{
    if (level != 0U) {
        GPIO_SetBits(soft_uart_port, soft_uart_pin);
    } else {
        GPIO_ResetBits(soft_uart_port, soft_uart_pin);
    }
}

/**
 * @brief 使用 DWT 周期计数器忙等一个 UART 位时间。
 */
static void soft_uart_delay_one_bit(void)
{
    uint32_t start = SOFT_UART_DWT_CYCCNT;

    while ((SOFT_UART_DWT_CYCCNT - start) < soft_uart_bit_cycles) {
    }
}

/**
 * @brief 将 TX 引脚配置为推挽输出并使能端口时钟。
 * @param[in] pin TX 的 HAL 引脚描述符。
 */
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

/**
 * @brief 在指定 TX 引脚与波特率上初始化软件 UART。
 * @param[in] cfg 配置结构体；为 null 或 baudrate 为 0 时忽略。
 */
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

/**
 * @brief 发送一个 8N1 字节（LSB 优先），发送期间屏蔽中断。
 * @param[in] byte 待发送数据字节。
 */
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

/**
 * @brief 发送缓冲区中的多个字节。
 * @param[in] data 字节缓冲区；为 null 时忽略。
 * @param[in] len 待发送字节数。
 */
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

/**
 * @brief 发送以 null 结尾的 ASCII 字符串。
 * @param[in] str 待发送字符串；为 null 时忽略。
 */
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

/**
 * @brief HAL_DEBUG_UART_ENABLE 关闭时的空 init。
 * @param[in] cfg 忽略。
 */
void soft_uart_init(const soft_uart_config_t *cfg)
{
    (void)cfg;
}

/**
 * @brief 软 UART 禁用时的空字节发送。
 * @param[in] byte 忽略。
 */
void soft_uart_write_byte(uint8_t byte)
{
    (void)byte;
}

/**
 * @brief 软 UART 禁用时的空缓冲区发送。
 * @param[in] data 忽略。
 * @param[in] len 忽略。
 */
void soft_uart_write(const uint8_t *data, unsigned short len)
{
    (void)data;
    (void)len;
}

/**
 * @brief 软 UART 禁用时的空字符串发送。
 * @param[in] str 忽略。
 */
void soft_uart_write_str(const char *str)
{
    (void)str;
}

#endif
