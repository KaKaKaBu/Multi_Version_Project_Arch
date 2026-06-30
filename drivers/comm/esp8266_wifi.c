/**
 * @file esp8266_wifi.c
 * @brief ESP8266 USART transport and comm_if.h stream driver.
 */

#include "comm_if.h"
#include "esp8266_port.h"
#include "usart_hal.h"
#include "gpio_hal.h"
#include "hal_common.h"
#include "hal_features.h"
#include "debug_uart.h"
#include "debug_log.h"
#include "driver_configs.h"
#include "driver_core.h"

#include <string.h>

/** @brief Maximum capacity of the software USART RX stream buffer. */
#define ESP8266_RX_STREAM_MAX 768U
/** @brief Non-zero when esp8266_port_drain_rx() should log drained bytes. */
#define ESP8266_DRAIN_DEBUG DEBUG_LOG_ESP8266_DRAIN_ENABLE

/**
 * @brief Forwards raw TX bytes to the debug UART trace when enabled.
 * @param data Pointer to transmitted bytes.
 * @param len Number of bytes to trace.
 */
static void esp8266_debug_trace(const uint8_t *data, uint16_t len)
{
#if HAL_DEBUG_UART_ENABLE
    debug_uart_trace_bytes(data, len);
#else
    (void)data;
    (void)len;
#endif
}

/** @brief Optional comm_if raw RX callback for single-byte notifications. */
static comm_rx_callback_t esp8266_rx_callback;
static const esp8266_driver_config_t *esp8266_config;
/** @brief Software ring-style buffer holding drained USART bytes for AT parsing. */
static uint8_t esp8266_rx_stream[ESP8266_RX_STREAM_MAX];
/** @brief Number of valid bytes currently stored in esp8266_rx_stream. */
static uint16_t esp8266_rx_stream_len;

/** @brief Asserts CH_PD to enable the ESP8266 module. */
static void esp8266_ch_pd_enable(void)
{
    gpio_hal_write(esp8266_config->ch_pd.port, esp8266_config->ch_pd.pin, 1U);
}

/** @brief Deasserts the ESP8266 hardware reset line. */
static void esp8266_rst_high(void)
{
    gpio_hal_write(esp8266_config->rst.port, esp8266_config->rst.pin, 1U);
}

/** @brief Asserts the ESP8266 hardware reset line. */
static void esp8266_rst_low(void)
{
    gpio_hal_write(esp8266_config->rst.port, esp8266_config->rst.pin, 0U);
}

/** @brief Configures USART and control GPIO, then powers the module. */
static void esp8266_hw_init(void)
{
    usart_hal_config_t cfg;

    if (esp8266_config == 0) {
        return;
    }

    gpio_hal_config_pin(&esp8266_config->ch_pd);
    gpio_hal_config_pin(&esp8266_config->rst);

    cfg.instance = esp8266_config->usart.instance;
    cfg.baudrate = esp8266_config->usart.baudrate;
    cfg.tx = esp8266_config->usart.tx;
    cfg.rx = esp8266_config->usart.rx;
    cfg.remap = esp8266_config->usart.remap;
    cfg.rx_buf_size = 256U;
    cfg.tx_timeout_us = USART_HAL_DEFAULT_TX_TIMEOUT_US;
    cfg.tx_mode = esp8266_config->usart.tx_mode;
#if HAL_USART_ENABLE_DMA
    cfg.tx_dma_channel = esp8266_config->usart.tx_dma_channel;
#endif
    (void)usart_hal_init(&cfg);
    usart_hal_enable_rx_irq(esp8266_config->usart.instance);

    esp8266_rst_high();
    esp8266_ch_pd_enable();
}

/** @brief Pulses RST low for 500 ms to reset the ESP8266. */
static void esp8266_reset(void)
{
    esp8266_rst_low();
    hal_delay_us(500000U);
    esp8266_rst_high();
}

/** @brief Clears the software RX stream buffer and USART hardware FIFO. */
void esp8266_port_rx_clear(void)
{
    DEBUG_LOG_ESP8266("[ESP8266] rx_clear old_stream_len=%u\r\n", (unsigned int)esp8266_rx_stream_len);
    esp8266_rx_stream_len = 0U;
    if (esp8266_config != 0) {
        usart_hal_flush_rx(esp8266_config->usart.instance);
    }
}

/**
 * @brief Appends bytes to the bounded software RX stream buffer.
 * @param data Pointer to incoming bytes; ignored when null or len is zero.
 * @param len Number of bytes to append.
 */
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

/**
 * @brief Returns the number of bytes currently stored in the RX stream buffer.
 * @return RX stream length in bytes.
 */
uint16_t esp8266_port_rx_length(void)
{
    return esp8266_rx_stream_len;
}

/**
 * @brief Returns a pointer to the RX stream buffer contents.
 * @return Read-only pointer valid until the next discard or clear.
 */
const uint8_t *esp8266_port_rx_data(void)
{
    return esp8266_rx_stream;
}

/**
 * @brief Removes len bytes from the front of the RX stream buffer.
 * @param len Number of bytes to discard; clears all when len exceeds length.
 */
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

/**
 * @brief Sends a raw byte buffer over the ESP8266 USART.
 * @param data Pointer to bytes to transmit.
 * @param len Number of bytes to send.
 * @return Number of bytes sent on success, or -1 on error.
 */
int esp8266_port_send(const uint8_t *data, uint16_t len)
{
    if ((data == 0) || (len == 0U)) {
        return -1;
    }

    if ((esp8266_config == 0) || (usart_hal_send_buffer(esp8266_config->usart.instance, data, len) != HAL_OK)) {
        return -1;
    }

    if (esp8266_config->debug_trace_enable != 0U) {
        esp8266_debug_trace(data, len);
    }
    return (int)len;
}

/**
 * @brief Sends a null-terminated string over the ESP8266 USART.
 * @param text C string to transmit.
 * @return Number of bytes sent on success, or -1 on error.
 */
int esp8266_port_send_str(const char *text)
{
    if (text == 0) {
        return -1;
    }

    return esp8266_port_send((const uint8_t *)text, (uint16_t)strlen(text));
}

/**
 * @brief Reads one byte from the USART hardware FIFO if available.
 * @param byte Output byte storage; ignored when null.
 * @return 1 if a byte was read, 0 if the FIFO is empty.
 */
int esp8266_port_recv_byte(uint8_t *byte)
{
    if (byte == 0) {
        return 0;
    }

    if ((esp8266_config != 0) && (usart_hal_recv_byte(esp8266_config->usart.instance, byte) == 1)) {
        return 1;
    }

    return 0;
}

/** @brief Drains the USART FIFO into the RX stream and invokes any raw RX callback. */
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

/**
 * @brief Returns 1 if token appears anywhere in the RX stream buffer.
 * @param token Substring to search for.
 * @return 1 if found, 0 otherwise.
 */
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

/**
 * @brief Polls the RX stream until token appears or timeout expires.
 * @param token Substring to search for in the RX stream.
 * @param timeout_ms Maximum wait time in milliseconds.
 * @return 1 if token was found, 0 on timeout.
 */
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

/**
 * @brief Waits for the ESP8266 '>' prompt used before raw payload uploads.
 * @param timeout_ms Maximum wait time in milliseconds.
 * @return 1 if the prompt appeared, 0 on timeout.
 */
int esp8266_port_wait_prompt(uint32_t timeout_ms)
{
    return esp8266_port_wait_token(">", timeout_ms);
}

/** @brief Initializes USART, clears RX state, and resets the module. */
static void esp8266_init(const void *config)
{
    esp8266_config = (const esp8266_driver_config_t *)config;
    if (esp8266_config == 0) {
        return;
    }

    DEBUG_LOG_ESP8266("[ESP8266] init start usart_id=%u baud=%lu\r\n",
                      (unsigned int)esp8266_config->usart.instance_id,
                      (unsigned long)esp8266_config->usart.baudrate);
    esp8266_rx_callback = 0;
    esp8266_port_rx_clear();
    esp8266_hw_init();
    esp8266_reset();
    hal_delay_us(2000000U);
    DEBUG_LOG_ESP8266("[ESP8266] init done\r\n");
}

/**
 * @brief Sends raw bytes through the comm_if send hook.
 * @param data Outbound byte buffer.
 * @param len Number of bytes to send.
 * @return Number of bytes sent, or -1 on error.
 */
static int esp8266_send(const unsigned char *data, unsigned short len)
{
    return esp8266_port_send((const uint8_t *)data, (uint16_t)len);
}

/**
 * @brief Receives one byte, optionally invoking the registered RX callback.
 * @param buf Output buffer for received data.
 * @param max_len Maximum bytes to receive (must be at least 1).
 * @return 1 if a byte was read, 0 if none available, -1 on error.
 */
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

/**
 * @brief Stores the comm_if raw RX callback for byte notifications.
 * @param callback Function to invoke on receive, or null to disable.
 */
static void esp8266_register_rx_callback(comm_rx_callback_t callback)
{
    esp8266_rx_callback = callback;
    DEBUG_LOG_ESP8266("[ESP8266] raw rx callback=%s\r\n", (callback != 0) ? "set" : "null");
}

/** @brief comm_if.h stream driver instance registered as COMM. */
static const comm_driver_t esp8266_drv = {
    "esp8266",
    COMM_KIND_STREAM,
    esp8266_init,
    esp8266_send,
    esp8266_recv,
    esp8266_register_rx_callback
};

REGISTER_DRIVER(COMM, esp8266_drv);
