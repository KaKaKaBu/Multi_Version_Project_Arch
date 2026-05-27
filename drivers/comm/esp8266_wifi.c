#include "comm_if.h"
#include "esp8266_port.h"
#include "usart_hal.h"
#include "gpio_hal.h"
#include "hal_common.h"
#include "hal_features.h"
#include "debug_uart.h"
#include "debug_log.h"
#include "board_config.h"
#include "driver_core.h"
#include "stm32f10x.h"

#include <string.h>

#define ESP8266_RX_STREAM_MAX 768U
#define ESP8266_DRAIN_DEBUG DEBUG_LOG_ESP8266_DRAIN_ENABLE

#if HAL_DEBUG_UART_ENABLE && defined(BOARD_DEBUG_UART_PASSTHROUGH_USART3) && \
    BOARD_DEBUG_UART_PASSTHROUGH_USART3 && (BOARD_ESP8266_USART_ID == 3U)
#define ESP8266_DEBUG_TRACE_USART3 1
#endif

#ifndef ESP8266_DEBUG_TRACE_USART3
#define ESP8266_DEBUG_TRACE_USART3 0
#endif

#if ESP8266_DEBUG_TRACE_USART3
static void esp8266_debug_trace(const uint8_t *data, uint16_t len)
{
    debug_uart_trace_bytes(data, len);
}
#else
static void esp8266_debug_trace(const uint8_t *data, uint16_t len)
{
    (void)data;
    (void)len;
}
#endif

static comm_rx_callback_t esp8266_rx_callback;
static uint8_t esp8266_rx_stream[ESP8266_RX_STREAM_MAX];
static uint16_t esp8266_rx_stream_len;

static void esp8266_ch_pd_enable(void)
{
    gpio_hal_write(board_esp8266_ch_pd_pin.port, board_esp8266_ch_pd_pin.pin, 1U);
}

static void esp8266_rst_high(void)
{
    gpio_hal_write(board_esp8266_rst_pin.port, board_esp8266_rst_pin.pin, 1U);
}

static void esp8266_rst_low(void)
{
    gpio_hal_write(board_esp8266_rst_pin.port, board_esp8266_rst_pin.pin, 0U);
}

static void esp8266_hw_init(void)
{
    usart_hal_config_t cfg;

    gpio_hal_config_pin(&board_esp8266_ch_pd_pin);
    gpio_hal_config_pin(&board_esp8266_rst_pin);

    cfg.instance = BOARD_ESP8266_USART;
    cfg.baudrate = BOARD_ESP8266_BAUDRATE;
#if (BOARD_ESP8266_USART_ID == 1U)
    cfg.tx = board_usart1_tx;
    cfg.rx = board_usart1_rx;
#elif (BOARD_ESP8266_USART_ID == 2U)
    cfg.tx = board_usart2_tx;
    cfg.rx = board_usart2_rx;
#else
    cfg.tx = board_usart3_tx;
    cfg.rx = board_usart3_rx;
#endif
    cfg.remap = BOARD_ESP8266_USART_REMAP;
    cfg.rx_buf_size = 256U;
    cfg.tx_timeout_us = USART_HAL_DEFAULT_TX_TIMEOUT_US;
    cfg.tx_mode = BOARD_ESP8266_USART_TX_MODE;
#if HAL_USART_ENABLE_DMA
    cfg.tx_dma_channel = BOARD_ESP8266_USART_TX_DMA;
#endif
    (void)usart_hal_init(&cfg);
    usart_hal_enable_rx_irq(BOARD_ESP8266_USART);

    esp8266_rst_high();
    esp8266_ch_pd_enable();
}

static void esp8266_reset(void)
{
    esp8266_rst_low();
    hal_delay_us(500000U);
    esp8266_rst_high();
}

void esp8266_port_rx_clear(void)
{
    DEBUG_LOG_ESP8266("[ESP8266] rx_clear old_stream_len=%u\r\n", (unsigned int)esp8266_rx_stream_len);
    esp8266_rx_stream_len = 0U;
    usart_hal_flush_rx(BOARD_ESP8266_USART);
}

void esp8266_port_rx_push(const uint8_t *data, uint16_t len)
{
    uint16_t i;

    if ((data == 0) || (len == 0U)) {
        return;
    }

    for (i = 0U; i < len; ++i) {
        if (esp8266_rx_stream_len >= ESP8266_RX_STREAM_MAX) {
            memmove(esp8266_rx_stream,
                    esp8266_rx_stream + 1U,
                    (size_t)(ESP8266_RX_STREAM_MAX - 1U));
            esp8266_rx_stream_len = ESP8266_RX_STREAM_MAX - 1U;
        }
        esp8266_rx_stream[esp8266_rx_stream_len++] = data[i];
    }
}

uint16_t esp8266_port_rx_length(void)
{
    return esp8266_rx_stream_len;
}

const uint8_t *esp8266_port_rx_data(void)
{
    return esp8266_rx_stream;
}

void esp8266_port_rx_discard(uint16_t len)
{
    if (len >= esp8266_rx_stream_len) {
        esp8266_rx_stream_len = 0U;
        return;
    }

    memmove(esp8266_rx_stream,
            esp8266_rx_stream + len,
            (size_t)(esp8266_rx_stream_len - len));
    esp8266_rx_stream_len = (uint16_t)(esp8266_rx_stream_len - len);
}

int esp8266_port_send(const uint8_t *data, uint16_t len)
{
    if ((data == 0) || (len == 0U)) {
        return -1;
    }

    if (usart_hal_send_buffer(BOARD_ESP8266_USART, data, len) != HAL_OK) {
        return -1;
    }

    esp8266_debug_trace(data, len);
    return (int)len;
}

int esp8266_port_send_str(const char *text)
{
    if (text == 0) {
        return -1;
    }

    return esp8266_port_send((const uint8_t *)text, (uint16_t)strlen(text));
}

int esp8266_port_recv_byte(uint8_t *byte)
{
    if (byte == 0) {
        return 0;
    }

    if (usart_hal_recv_byte(BOARD_ESP8266_USART, byte) == 1) {
        return 1;
    }

    return 0;
}

void esp8266_port_drain_rx(void)
{
    uint8_t byte;
#if ESP8266_DRAIN_DEBUG
    char preview[17];
#endif
    uint16_t drained = 0U;
#if ESP8266_DRAIN_DEBUG
    uint16_t preview_len = 0U;
#endif

    while (esp8266_port_recv_byte(&byte) == 1) {
        esp8266_port_rx_push(&byte, 1U);
#if ESP8266_DRAIN_DEBUG
        if (preview_len < 16U) {
            preview[preview_len] = ((byte >= 32U) && (byte <= 126U)) ? (char)byte : '.';
            ++preview_len;
        }
#endif
        ++drained;
        if (esp8266_rx_callback != 0) {
            esp8266_rx_callback(&byte, 1U);
        }
    }

#if ESP8266_DRAIN_DEBUG
    if (drained != 0U) {
        preview[preview_len] = '\0';
        DEBUG_LOG_ESP8266_DRAIN("[ESP8266] drain bytes=%u stream_len=%u cb=%s data=%s\r\n",
                                (unsigned int)drained,
                                (unsigned int)esp8266_rx_stream_len,
                                (esp8266_rx_callback != 0) ? "yes" : "no",
                                preview);
    }
#else
    (void)drained;
#endif
}

static int esp8266_port_buffer_contains(const char *token)
{
    uint16_t i;
    uint16_t token_len;

    if (token == 0) {
        return 0;
    }

    token_len = (uint16_t)strlen(token);
    if ((token_len == 0U) || (esp8266_rx_stream_len < token_len)) {
        return 0;
    }

    for (i = 0U; i <= (uint16_t)(esp8266_rx_stream_len - token_len); ++i) {
        if (memcmp(esp8266_rx_stream + i, token, token_len) == 0) {
            return 1;
        }
    }

    return 0;
}

int esp8266_port_wait_token(const char *token, uint32_t timeout_ms)
{
    uint32_t elapsed_ms = 0U;

    while (elapsed_ms < timeout_ms) {
        esp8266_port_drain_rx();
        if (esp8266_port_buffer_contains(token) != 0) {
            return 1;
        }
        hal_delay_us(10000U);
        elapsed_ms += 10U;
    }

    return 0;
}

int esp8266_port_wait_prompt(uint32_t timeout_ms)
{
    return esp8266_port_wait_token(">", timeout_ms);
}

static void esp8266_init(void)
{
    DEBUG_LOG_ESP8266("[ESP8266] init start usart_id=%u baud=%lu\r\n",
                      (unsigned int)BOARD_ESP8266_USART_ID,
                      (unsigned long)BOARD_ESP8266_BAUDRATE);
    esp8266_rx_callback = 0;
    esp8266_port_rx_clear();
    esp8266_hw_init();
    esp8266_reset();
    hal_delay_us(2000000U);
    DEBUG_LOG_ESP8266("[ESP8266] init done\r\n");
}

static int esp8266_send(const unsigned char *data, unsigned short len)
{
    return esp8266_port_send((const uint8_t *)data, (uint16_t)len);
}

static int esp8266_recv(unsigned char *buf, unsigned short max_len)
{
    uint8_t data;

    if ((buf == 0) || (max_len == 0U)) {
        return -1;
    }

    esp8266_port_drain_rx();

    if (esp8266_port_recv_byte(&data) == 1) {
        buf[0] = data;
        esp8266_port_rx_push(&data, 1U);
        if (esp8266_rx_callback != 0) {
            esp8266_rx_callback(buf, 1U);
        }
        return 1;
    }

    return 0;
}

static void esp8266_register_rx_callback(comm_rx_callback_t callback)
{
    esp8266_rx_callback = callback;
    DEBUG_LOG_ESP8266("[ESP8266] raw rx callback=%s\r\n", (callback != 0) ? "set" : "null");
}

static const comm_driver_t esp8266_drv = {
    "esp8266",
    COMM_KIND_STREAM,
    esp8266_init,
    esp8266_send,
    esp8266_recv,
    esp8266_register_rx_callback
};

REGISTER_DRIVER(COMM, esp8266_drv);
