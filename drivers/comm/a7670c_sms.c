/**
 * @file a7670c_sms.c
 * @brief A7670C 4G 模块 USART/AT 短信驱动。
 */

#include "a7670c_sms.h"
#include "comm_if.h"
#include "usart_hal.h"
#include "hal_common.h"
#include "driver_configs.h"
#include "driver_core.h"

#include <string.h>

#define A7670C_RX_STREAM_MAX 512U
#define A7670C_CMD_BUF_SIZE 64U
#define A7670C_DEFAULT_TIMEOUT_MS 2000U
#define A7670C_SMS_TIMEOUT_MS 10000U
#define A7670C_CTRL_Z 0x1AU

static comm_rx_callback_t a7670c_rx_callback;
static uint8_t a7670c_rx_stream[A7670C_RX_STREAM_MAX];
static uint16_t a7670c_rx_stream_len;
static uint8_t a7670c_ready;
static const usart_device_config_t *a7670c_config;

static void a7670c_rx_clear(void)
{
    a7670c_rx_stream_len = 0U;
    if (a7670c_config != 0) {
        usart_hal_flush_rx(a7670c_config->instance);
    }
}

static void a7670c_rx_push(const uint8_t *data, uint16_t len)
{
    uint16_t i;

    if ((data == 0) || (len == 0U)) {
        return;
    }

    for (i = 0U; i < len; ++i) {
        if (a7670c_rx_stream_len >= A7670C_RX_STREAM_MAX) {
            memmove(a7670c_rx_stream,
                    a7670c_rx_stream + 1U,
                    (size_t)(A7670C_RX_STREAM_MAX - 1U));
            a7670c_rx_stream_len = A7670C_RX_STREAM_MAX - 1U;
        }
        a7670c_rx_stream[a7670c_rx_stream_len++] = data[i];
    }
}

void a7670c_sms_poll(void)
{
    uint8_t byte;

    if (a7670c_config == 0) {
        return;
    }

    while (usart_hal_recv_byte(a7670c_config->instance, &byte) == 1) {
        a7670c_rx_push(&byte, 1U);
        if (a7670c_rx_callback != 0) {
            a7670c_rx_callback(&byte, 1U);
        }
    }
}

static int a7670c_contains(const char *token)
{
    char stream[A7670C_RX_STREAM_MAX + 1U];

    if (token == 0) {
        return 0;
    }

    memcpy(stream, a7670c_rx_stream, a7670c_rx_stream_len);
    stream[a7670c_rx_stream_len] = '\0';

    return strstr(stream, token) != 0;
}

static int a7670c_wait_token(const char *token, uint32_t timeout_ms)
{
    uint32_t elapsed_ms;

    elapsed_ms = 0U;
    while (elapsed_ms < timeout_ms) {
        a7670c_sms_poll();
        if (a7670c_contains(token) != 0) {
            return 1;
        }
        hal_delay_us(1000U);
        ++elapsed_ms;
    }

    return 0;
}

static int a7670c_send_raw(const uint8_t *data, uint16_t len)
{
    if ((data == 0) || (len == 0U)) {
        return -1;
    }

    if ((a7670c_config == 0) || (usart_hal_send_buffer(a7670c_config->instance, data, len) != HAL_OK)) {
        return -1;
    }

    return (int)len;
}

static int a7670c_send_str(const char *text)
{
    if (text == 0) {
        return -1;
    }

    return a7670c_send_raw((const uint8_t *)text, (uint16_t)strlen(text));
}

static int a7670c_send_cmd_wait(const char *cmd, const char *token, uint32_t timeout_ms)
{
    a7670c_rx_clear();
    if (a7670c_send_str(cmd) < 0) {
        return -1;
    }

    return a7670c_wait_token(token, timeout_ms) != 0 ? 0 : -1;
}

static uint16_t a7670c_append_char(char *buf, uint16_t pos, uint16_t max_len, char ch)
{
    if ((buf != 0) && (pos < (uint16_t)(max_len - 1U))) {
        buf[pos++] = ch;
        buf[pos] = '\0';
    }
    return pos;
}

static uint16_t a7670c_append_str(char *buf, uint16_t pos, uint16_t max_len, const char *text)
{
    if ((buf == 0) || (text == 0) || (max_len == 0U)) {
        return pos;
    }

    while ((*text != '\0') && (pos < (uint16_t)(max_len - 1U))) {
        buf[pos++] = *text++;
    }
    buf[pos] = '\0';

    return pos;
}

static int a7670c_build_cmgs(char *buf, uint16_t max_len, const char *phone)
{
    uint16_t pos;

    if ((buf == 0) || (max_len < 16U) || (phone == 0) || (*phone == '\0')) {
        return -1;
    }

    pos = 0U;
    pos = a7670c_append_str(buf, pos, max_len, "AT+CMGS=\"");
    pos = a7670c_append_str(buf, pos, max_len, phone);
    pos = a7670c_append_char(buf, pos, max_len, '"');
    pos = a7670c_append_str(buf, pos, max_len, "\r\n");

    if (pos >= (uint16_t)(max_len - 1U)) {
        return -1;
    }

    return 0;
}

static void a7670c_hw_init(void)
{
    usart_hal_config_t cfg;

    if (a7670c_config == 0) {
        return;
    }

    cfg.instance = a7670c_config->instance;
    cfg.baudrate = a7670c_config->baudrate;
    cfg.tx = a7670c_config->tx;
    cfg.rx = a7670c_config->rx;
    cfg.remap = a7670c_config->remap;
    cfg.rx_buf_size = 256U;
    cfg.tx_timeout_us = USART_HAL_DEFAULT_TX_TIMEOUT_US;
    cfg.tx_mode = a7670c_config->tx_mode;
#if HAL_USART_ENABLE_DMA
    cfg.tx_dma_channel = a7670c_config->tx_dma_channel;
#endif
    (void)usart_hal_init(&cfg);
    usart_hal_enable_rx_irq(a7670c_config->instance);
}

static void a7670c_init(const void *config)
{
    a7670c_config = (const usart_device_config_t *)config;
    if (a7670c_config == 0) {
        return;
    }
    a7670c_rx_callback = 0;
    a7670c_rx_stream_len = 0U;
    a7670c_ready = 0U;
    a7670c_hw_init();
}

static int a7670c_send(const unsigned char *data, unsigned short len)
{
    return a7670c_send_raw(data, len);
}

static int a7670c_recv(unsigned char *buf, unsigned short max_len)
{
    uint8_t data;

    if ((buf == 0) || (max_len == 0U)) {
        return -1;
    }

    if ((a7670c_config != 0) && (usart_hal_recv_byte(a7670c_config->instance, &data) == 1)) {
        buf[0] = data;
        a7670c_rx_push(&data, 1U);
        if (a7670c_rx_callback != 0) {
            a7670c_rx_callback(buf, 1U);
        }
        return 1;
    }

    return 0;
}

static void a7670c_register_rx_callback(comm_rx_callback_t callback)
{
    a7670c_rx_callback = callback;
}

int a7670c_sms_setup(void)
{
    if (a7670c_send_cmd_wait("AT\r\n", "OK", A7670C_DEFAULT_TIMEOUT_MS) < 0) {
        return -1;
    }
    (void)a7670c_send_cmd_wait("ATE0\r\n", "OK", A7670C_DEFAULT_TIMEOUT_MS);
    if (a7670c_send_cmd_wait("AT+CPIN?\r\n", "READY", A7670C_DEFAULT_TIMEOUT_MS) < 0) {
        return -1;
    }
    (void)a7670c_send_cmd_wait("AT+CREG?\r\n", "OK", A7670C_DEFAULT_TIMEOUT_MS);
    (void)a7670c_send_cmd_wait("AT+CGREG?\r\n", "OK", A7670C_DEFAULT_TIMEOUT_MS);
    if (a7670c_send_cmd_wait("AT+CMGF=1\r\n", "OK", A7670C_DEFAULT_TIMEOUT_MS) < 0) {
        return -1;
    }
    if (a7670c_send_cmd_wait("AT+CSCS=\"GSM\"\r\n", "OK", A7670C_DEFAULT_TIMEOUT_MS) < 0) {
        return -1;
    }

    a7670c_ready = 1U;
    return 0;
}

int a7670c_sms_is_ready(void)
{
    return a7670c_ready != 0U;
}

int a7670c_sms_send_text(const char *phone, const char *text)
{
    char cmd[A7670C_CMD_BUF_SIZE];
    uint8_t end;

    if ((phone == 0) || (text == 0) || (*phone == '\0') || (*text == '\0')) {
        return -1;
    }

    if (a7670c_ready == 0U) {
        if (a7670c_sms_setup() < 0) {
            return -1;
        }
    }

    if (a7670c_build_cmgs(cmd, sizeof(cmd), phone) < 0) {
        return -1;
    }

    a7670c_rx_clear();
    if (a7670c_send_str(cmd) < 0) {
        return -1;
    }
    if (a7670c_wait_token(">", A7670C_DEFAULT_TIMEOUT_MS) == 0) {
        return -1;
    }

    if (a7670c_send_str(text) < 0) {
        return -1;
    }
    end = A7670C_CTRL_Z;
    if (a7670c_send_raw(&end, 1U) < 0) {
        return -1;
    }

    return a7670c_wait_token("OK", A7670C_SMS_TIMEOUT_MS) != 0 ? 0 : -1;
}

static const comm_driver_t a7670c_drv = {
    "a7670c",
    COMM_KIND_STREAM,
    a7670c_init,
    a7670c_send,
    a7670c_recv,
    a7670c_register_rx_callback
};

REGISTER_DRIVER(COMM, a7670c_drv);
