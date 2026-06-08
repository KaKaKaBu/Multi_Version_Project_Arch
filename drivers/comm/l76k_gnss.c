#include "gnss_if.h"
#include "usart_hal.h"
#include "board_config.h"
#include "driver_core.h"
#include "stm32f10x.h"

#include <string.h>

#define L76K_GPS_BUF_SIZE 128U

static gnss_fix_t l76k_fix;
static uint8_t l76k_rx_buf[L76K_GPS_BUF_SIZE];
static uint16_t l76k_rx_len;
static uint8_t l76k_rx_complete;

static int l76k_parse_int(const char *s)
{
    int value = 0;
    int sign = 1;

    if (s == 0) {
        return 0;
    }

    if (*s == '-') {
        sign = -1;
        ++s;
    }

    while ((*s >= '0') && (*s <= '9')) {
        value = (value * 10) + (*s - '0');
        ++s;
    }

    return value * sign;
}

static float l76k_parse_float(const char *s)
{
    float value = 0.0f;
    float scale = 0.1f;
    uint8_t in_fraction = 0U;
    int sign = 1;

    if (s == 0) {
        return 0.0f;
    }

    if (*s == '-') {
        sign = -1;
        ++s;
    }

    while ((*s >= '0') && (*s <= '9')) {
        value = (value * 10.0f) + (float)(*s - '0');
        ++s;
    }

    if (*s == '.') {
        ++s;
        in_fraction = 1U;
    }

    while (in_fraction != 0U && (*s >= '0') && (*s <= '9')) {
        value += (float)(*s - '0') * scale;
        scale *= 0.1f;
        ++s;
    }

    return value * (float)sign;
}

static char *l76k_nmea_split(char *str, uint8_t n)
{
    uint8_t cnt = 0U;
    char *p = str;

    while (*p != '\0') {
        if (*p == ',') {
            ++cnt;
            if (cnt == n) {
                return p + 1;
            }
        }
        ++p;
    }

    return 0;
}

static float l76k_ddmm_to_dd(char *ddmm_str)
{
    float ddmm = l76k_parse_float(ddmm_str);
    uint8_t dd = (uint8_t)(ddmm / 100.0f);
    float mm = ddmm - (float)dd * 100.0f;

    return (float)dd + mm / 60.0f;
}

static void l76k_parse_gprmc(char *gprmc_str)
{
    char *p;

    p = l76k_nmea_split(gprmc_str, 2U);
    if ((p == 0) || (*p != 'A')) {
        l76k_fix.valid = 0U;
        return;
    }

    l76k_fix.valid = 1U;

    p = l76k_nmea_split(gprmc_str, 3U);
    if ((p != 0) && (strlen(p) > 0U)) {
        l76k_fix.latitude = l76k_ddmm_to_dd(p);
        p = l76k_nmea_split(gprmc_str, 4U);
        if ((p != 0) && (*p == 'S')) {
            l76k_fix.latitude = -l76k_fix.latitude;
        }
    }

    p = l76k_nmea_split(gprmc_str, 5U);
    if ((p != 0) && (strlen(p) > 0U)) {
        l76k_fix.longitude = l76k_ddmm_to_dd(p);
        p = l76k_nmea_split(gprmc_str, 6U);
        if ((p != 0) && (*p == 'W')) {
            l76k_fix.longitude = -l76k_fix.longitude;
        }
    }

    p = l76k_nmea_split(gprmc_str, 7U);
    if ((p != 0) && (strlen(p) > 0U)) {
        l76k_fix.speed_kmh = l76k_parse_float(p) * 1.852f;
    }

    p = l76k_nmea_split(gprmc_str, 8U);
    if ((p != 0) && (strlen(p) > 0U)) {
        l76k_fix.course = l76k_parse_float(p);
    }
}

static void l76k_parse_gngga(char *gngga_str)
{
    char *p;

    p = l76k_nmea_split(gngga_str, 6U);
    if ((p == 0) || (l76k_parse_int(p) == 0)) {
        l76k_fix.altitude = 0.0f;
        return;
    }

    p = l76k_nmea_split(gngga_str, 9U);
    if ((p != 0) && (strlen(p) > 0U)) {
        l76k_fix.altitude = l76k_parse_float(p);
    }
}

static void l76k_nmea_parse(void)
{
    if (l76k_rx_complete == 0U) {
        return;
    }

    if (strstr((char *)l76k_rx_buf, "$GNRMC") != 0) {
        l76k_parse_gprmc((char *)l76k_rx_buf);
    }

    if (strstr((char *)l76k_rx_buf, "$GNGGA") != 0) {
        l76k_parse_gngga((char *)l76k_rx_buf);
    }

    l76k_rx_complete = 0U;
}

static void l76k_drain_rx(void)
{
    uint8_t byte;

    while (usart_hal_recv_byte(BOARD_L76K_USART, &byte) == 1) {
        if (l76k_rx_len < (L76K_GPS_BUF_SIZE - 1U)) {
            l76k_rx_buf[l76k_rx_len++] = byte;
            if (l76k_rx_buf[l76k_rx_len - 1U] == '\n') {
                l76k_rx_buf[l76k_rx_len] = '\0';
                l76k_rx_complete = 1U;
                l76k_rx_len = 0U;
                l76k_nmea_parse();
            }
        } else {
            l76k_rx_len = 0U;
        }
    }
}

static void l76k_init(void)
{
    usart_hal_config_t cfg;

    memset(&l76k_fix, 0, sizeof(l76k_fix));
    l76k_rx_len = 0U;
    l76k_rx_complete = 0U;
    memset(l76k_rx_buf, 0, sizeof(l76k_rx_buf));

    cfg.instance = BOARD_L76K_USART;
    cfg.baudrate = BOARD_L76K_BAUDRATE;
    cfg.tx = board_l76k_tx;
    cfg.rx = board_l76k_rx;
    cfg.remap = BOARD_L76K_USART_REMAP;
    cfg.rx_buf_size = USART_HAL_DEFAULT_RX_BUF_SIZE;
    cfg.tx_timeout_us = USART_HAL_DEFAULT_TX_TIMEOUT_US;
    cfg.tx_mode = USART_HAL_TX_MODE_IRQ;
    (void)usart_hal_init(&cfg);
    usart_hal_enable_rx_irq(BOARD_L76K_USART);
}

static void l76k_poll(void)
{
    l76k_drain_rx();
}

static void l76k_get_fix(gnss_fix_t *fix)
{
    if (fix != 0) {
        *fix = l76k_fix;
    }
}

static const gnss_driver_t l76k_drv = {
    "l76k",
    l76k_init,
    l76k_poll,
    l76k_get_fix
};

REGISTER_DRIVER(GNSS, l76k_drv);
