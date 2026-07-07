#include "comm_if.h"
#include "esp8266_port.h"
#include "usart_hal.h"
#include "gpio_hal.h"
#include "hal_common.h"
#include "driver_configs.h"
#include "driver_core.h"
#include "mcs51_memory.h"

#define ESP8266_MCS51_RX_STREAM_MAX 192U

static comm_rx_callback_t esp8266_rx_callback;
static const esp8266_driver_config_t *esp8266_config;
static MCS51_XDATA uint8_t esp8266_rx_stream[ESP8266_MCS51_RX_STREAM_MAX];
static uint16_t esp8266_rx_stream_len;

static uint16_t esp8266_strlen(const char *text)
{
    uint16_t len = 0U;

    if (text == 0) {
        return 0U;
    }
    while (text[len] != '\0') {
        ++len;
    }
    return len;
}

static uint8_t esp8266_match_at(uint16_t pos, const char *token, uint16_t token_len)
{
    uint16_t i;

    for (i = 0U; i < token_len; ++i) {
        if (esp8266_rx_stream[pos + i] != (uint8_t)token[i]) {
            return 0U;
        }
    }
    return 1U;
}

static void esp8266_ch_pd_enable(void)
{
    gpio_hal_write(esp8266_config->ch_pd.port, esp8266_config->ch_pd.pin, 1U);
}

static void esp8266_rst_high(void)
{
    gpio_hal_write(esp8266_config->rst.port, esp8266_config->rst.pin, 1U);
}

static void esp8266_rst_low(void)
{
    gpio_hal_write(esp8266_config->rst.port, esp8266_config->rst.pin, 0U);
}

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
    cfg.rx_buf_size = 0U;
    cfg.tx_timeout_us = USART_HAL_DEFAULT_TX_TIMEOUT_US;
    cfg.tx_mode = esp8266_config->usart.tx_mode;
    (void)usart_hal_init(&cfg);
    usart_hal_enable_rx_irq(esp8266_config->usart.instance);

    esp8266_rst_high();
    esp8266_ch_pd_enable();
}

static void esp8266_reset(void)
{
    esp8266_rst_low();
    hal_delay_us(300000U);
    esp8266_rst_high();
}

void esp8266_port_rx_clear(void)
{
    esp8266_rx_stream_len = 0U;
    if (esp8266_config != 0) {
        usart_hal_flush_rx(esp8266_config->usart.instance);
    }
}

void esp8266_port_rx_push(const uint8_t *data, uint16_t len)
{
    uint16_t i;
    uint16_t j;

    if ((data == 0) || (len == 0U)) {
        return;
    }

    for (i = 0U; i < len; ++i) {
        if (esp8266_rx_stream_len >= ESP8266_MCS51_RX_STREAM_MAX) {
            for (j = 1U; j < ESP8266_MCS51_RX_STREAM_MAX; ++j) {
                esp8266_rx_stream[j - 1U] = esp8266_rx_stream[j];
            }
            esp8266_rx_stream_len = ESP8266_MCS51_RX_STREAM_MAX - 1U;
        }
        esp8266_rx_stream[esp8266_rx_stream_len] = data[i];
        ++esp8266_rx_stream_len;
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
    uint16_t i;

    if (len >= esp8266_rx_stream_len) {
        esp8266_rx_stream_len = 0U;
        return;
    }

    for (i = len; i < esp8266_rx_stream_len; ++i) {
        esp8266_rx_stream[i - len] = esp8266_rx_stream[i];
    }
    esp8266_rx_stream_len = (uint16_t)(esp8266_rx_stream_len - len);
}

int esp8266_port_send(const uint8_t *data, uint16_t len)
{
    if ((data == 0) || (len == 0U) || (esp8266_config == 0)) {
        return -1;
    }

    return (usart_hal_send_buffer(esp8266_config->usart.instance, data, len) == HAL_OK) ? (int)len : -1;
}

int esp8266_port_send_str(const char *text)
{
    return esp8266_port_send((const uint8_t *)text, esp8266_strlen(text));
}

int esp8266_port_recv_byte(uint8_t *byte)
{
    if ((byte == 0) || (esp8266_config == 0)) {
        return 0;
    }

    return (usart_hal_recv_byte(esp8266_config->usart.instance, byte) == 1) ? 1 : 0;
}

void esp8266_port_drain_rx(void)
{
    uint8_t byte;

    while (esp8266_port_recv_byte(&byte) == 1) {
        esp8266_port_rx_push(&byte, 1U);
        if (esp8266_rx_callback != 0) {
            esp8266_rx_callback(&byte, 1U);
        }
    }
}

static int esp8266_port_buffer_contains(const char *token)
{
    uint16_t i;
    uint16_t token_len;

    token_len = esp8266_strlen(token);
    if ((token_len == 0U) || (esp8266_rx_stream_len < token_len)) {
        return 0;
    }

    for (i = 0U; i <= (uint16_t)(esp8266_rx_stream_len - token_len); ++i) {
        if (esp8266_match_at(i, token, token_len) != 0U) {
            return 1;
        }
    }
    return 0;
}

int esp8266_port_wait_token(const char *token, uint32_t timeout_ms)
{
    uint32_t elapsed_ms = 0UL;

    while (elapsed_ms < timeout_ms) {
        esp8266_port_drain_rx();
        if (esp8266_port_buffer_contains(token) != 0) {
            return 1;
        }
        if (esp8266_port_buffer_contains("ERROR") != 0) {
            return 0;
        }
        hal_delay_us(10000UL);
        elapsed_ms += 10UL;
    }
    return 0;
}

int esp8266_port_wait_prompt(uint32_t timeout_ms)
{
    return esp8266_port_wait_token(">", timeout_ms);
}

static void esp8266_init(const void *config)
{
    esp8266_config = (const esp8266_driver_config_t *)config;
    if (esp8266_config == 0) {
        return;
    }

    esp8266_rx_callback = 0;
    esp8266_rx_stream_len = 0U;
    esp8266_hw_init();
    esp8266_reset();
    hal_delay_us(1200000UL);
    esp8266_port_rx_clear();
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
}

const comm_driver_t esp8266_drv = {
    "esp8266",
    COMM_KIND_STREAM,
    esp8266_init,
    esp8266_send,
    esp8266_recv,
    esp8266_register_rx_callback
};

REGISTER_DRIVER(COMM, esp8266_drv);
