/**
 * @file ds1302_rtc.c
 * @brief DS1302 three-wire RTC driver registered as RTC via rtc_if.h.
 */

#include "rtc_if.h"
#include "gpio_hal.h"
#include "hal_common.h"
#include "board_config.h"
#include "debug_log.h"
#include "driver_core.h"

/** @brief Cached calendar time updated by refresh and set operations. */
static rtc_time_t ds1302_time_cache;

/** @brief Forward declaration: programs chip registers from rtc_time_t. */
static void ds1302_set_time(const rtc_time_t *time);
/** @brief Forward declaration: writes one DS1302 register over the serial bus. */
static void ds1302_write_reg(uint8_t address, uint8_t data);
/** @brief Forward declaration: reads one DS1302 register over the serial bus. */
static uint8_t ds1302_read_reg(uint8_t address);

/**
 * @brief Drives the DS1302 chip-enable (CE) line.
 * @param level Non-zero for high, zero for low.
 */
static void ds1302_ce_write(uint8_t level)
{
    gpio_hal_write(board_ds1302_ce_pin.port, board_ds1302_ce_pin.pin, level);
}

/**
 * @brief Drives the DS1302 serial clock (SCLK) line.
 * @param level Non-zero for high, zero for low.
 */
static void ds1302_sclk_write(uint8_t level)
{
    gpio_hal_write(board_ds1302_sclk_pin.port, board_ds1302_sclk_pin.pin, level);
}

/**
 * @brief Drives the DS1302 bidirectional data line as output.
 * @param level Non-zero for high, zero for low.
 */
static void ds1302_data_write(uint8_t level)
{
    gpio_hal_write(board_ds1302_data_pin.port, board_ds1302_data_pin.pin, level);
}

/** @brief Configures the DS1302 data pin as push-pull output. */
static void ds1302_data_set_output(void)
{
    gpio_hal_set_mode(board_ds1302_data_pin.port, board_ds1302_data_pin.pin, GPIO_HAL_MODE_OUT_PP);
}

/** @brief Configures the DS1302 data pin as floating input. */
static void ds1302_data_set_input(void)
{
    gpio_hal_set_mode(board_ds1302_data_pin.port, board_ds1302_data_pin.pin, GPIO_HAL_MODE_IN_FLOATING);
}

/**
 * @brief Reads the current level on the DS1302 data pin.
 * @return Non-zero if the line is high; zero if low.
 */
static uint8_t ds1302_data_read(void)
{
    return gpio_hal_read(board_ds1302_data_pin.port, board_ds1302_data_pin.pin);
}

/**
 * @brief Converts a decimal value to BCD for DS1302 register writes.
 * @param value Decimal value 0-99.
 * @return Packed BCD byte.
 */
static uint8_t ds1302_dec_to_bcd(uint8_t value)
{
    return (uint8_t)(((value / 10U) << 4U) | (value % 10U));
}

/**
 * @brief Converts a BCD register byte to decimal.
 * @param value Packed BCD byte.
 * @return Decimal value 0-99.
 */
static uint8_t ds1302_bcd_to_dec(uint8_t value)
{
    return (uint8_t)(((value >> 4U) * 10U) + (value & 0x0FU));
}

/**
 * @brief Checks that a BCD nibble pair is valid and within max_value.
 * @param bcd Packed BCD byte to validate.
 * @param max_value Maximum allowed decimal value after conversion.
 * @return 1 if valid; 0 otherwise.
 */
static uint8_t ds1302_bcd_byte_is_valid(uint8_t bcd, uint8_t max_value)
{
    uint8_t hi = (uint8_t)(bcd >> 4U);
    uint8_t lo = (uint8_t)(bcd & 0x0FU);

    if ((lo > 9U) || (hi > 9U)) {
        return 0U;
    }

    return (ds1302_bcd_to_dec(bcd) <= max_value) ? 1U : 0U;
}

/**
 * @brief Enables or disables DS1302 write protection via register 0x8E.
 * @param enable Non-zero to enable write protect; zero to allow writes.
 */
static void ds1302_write_protect(uint8_t enable)
{
    ds1302_write_reg(0x8EU, enable != 0U ? 0x80U : 0x00U);
}

/** @brief Clears the clock-halt (CH) bit so the oscillator runs. */
static void ds1302_start_clock(void)
{
    uint8_t sec;

    ds1302_write_protect(0U);
    sec = ds1302_read_reg(0x81U);
    DEBUG_LOG_RTC("[RTC] start_clock sec_raw=0x%02X CH=%u\r\n",
                  (unsigned int)sec,
                  (unsigned int)(((sec & 0x80U) != 0U) ? 1U : 0U));
    if ((sec & 0x80U) != 0U) {
        ds1302_write_reg(0x80U, (uint8_t)(sec & 0x7FU));
        DEBUG_LOG_RTC("[RTC] start_clock CH cleared\r\n");
    }
    ds1302_write_protect(1U);
}

/**
 * @brief Validates calendar field ranges in an rtc_time_t.
 * @param time Pointer to the time structure; must not be null.
 * @return 1 if all fields are in range; 0 otherwise.
 */
static uint8_t ds1302_time_is_valid(const rtc_time_t *time)
{
    if (time == 0) {
        return 0U;
    }

    if ((time->year < 2000U) || (time->month < 1U) || (time->month > 12U) ||
        (time->day < 1U) || (time->day > 31U) || (time->hour > 23U) ||
        (time->minute > 59U) || (time->second > 59U)) {
        return 0U;
    }

    return 1U;
}

/** @brief Writes a fixed default calendar when the chip RAM is invalid. */
static void ds1302_apply_default_time(void)
{
    rtc_time_t default_time;

    default_time.year = 2026U;
    default_time.month = 5U;
    default_time.day = 25U;
    default_time.hour = 12U;
    default_time.minute = 0U;
    default_time.second = 0U;
    default_time.week = 1U;
    ds1302_set_time(&default_time);
}

/**
 * @brief Shifts one byte out on the DS1302 data line LSB first.
 * @param data Byte value to shift out.
 */
static void ds1302_write_byte(uint8_t data)
{
    uint8_t count;

    ds1302_sclk_write(0U);
    ds1302_data_set_output();

    for (count = 0U; count < 8U; ++count) {
        ds1302_sclk_write(0U);
        if ((data & 0x01U) != 0U) {
            ds1302_data_write(1U);
        } else {
            ds1302_data_write(0U);
        }
        ds1302_sclk_write(1U);
        data = (uint8_t)(data >> 1U);
    }
}

/**
 * @brief Performs a DS1302 write transaction for one register.
 * @param address DS1302 register address with write bit set.
 * @param data Value to write to the register.
 */
static void ds1302_write_reg(uint8_t address, uint8_t data)
{
    ds1302_ce_write(0U);
    ds1302_sclk_write(0U);
    hal_delay_us(1U);
    ds1302_ce_write(1U);
    hal_delay_us(3U);
    ds1302_write_byte(address);
    ds1302_write_byte(data);
    ds1302_ce_write(0U);
    ds1302_sclk_write(0U);
    hal_delay_us(3U);
}

/**
 * @brief Performs a DS1302 read transaction for one register.
 * @param address DS1302 register address with read bit set.
 * @return Register value read from the chip.
 */
static uint8_t ds1302_read_reg(uint8_t address)
{
    uint8_t count;
    uint8_t return_data = 0U;

    ds1302_ce_write(0U);
    ds1302_sclk_write(0U);
    hal_delay_us(3U);
    ds1302_ce_write(1U);
    hal_delay_us(3U);
    ds1302_write_byte(address);
    ds1302_data_set_input();
    hal_delay_us(3U);

    for (count = 0U; count < 8U; ++count) {
        hal_delay_us(3U);
        return_data = (uint8_t)(return_data >> 1U);
        ds1302_sclk_write(1U);
        hal_delay_us(5U);
        ds1302_sclk_write(0U);
        hal_delay_us(30U);
        if (ds1302_data_read() != 0U) {
            return_data = (uint8_t)(return_data | 0x80U);
        }
    }

    hal_delay_us(2U);
    ds1302_ce_write(0U);
    ds1302_data_set_output();
    ds1302_data_write(0U);
    return return_data;
}

/** @brief Reads time/date registers into ds1302_time_cache when BCD is valid. */
static void ds1302_refresh_cache(void)
{
    uint8_t raw[7];

    raw[0] = ds1302_read_reg(0x81U);
    raw[1] = ds1302_read_reg(0x83U);
    raw[2] = ds1302_read_reg(0x85U);
    raw[3] = ds1302_read_reg(0x87U);
    raw[4] = ds1302_read_reg(0x89U);
    raw[5] = ds1302_read_reg(0x8BU);
    raw[6] = ds1302_read_reg(0x8DU);

    if ((ds1302_bcd_byte_is_valid((uint8_t)(raw[0] & 0x7FU), 59U) == 0U) ||
        (ds1302_bcd_byte_is_valid(raw[1], 59U) == 0U) ||
        (ds1302_bcd_byte_is_valid((uint8_t)(raw[2] & 0x3FU), 23U) == 0U)) {
        DEBUG_LOG_RTC("[RTC] refresh fail raw sec=0x%02X min=0x%02X hr=0x%02X CH=%u cache=%02u:%02u:%02u\r\n",
                      (unsigned int)raw[0],
                      (unsigned int)raw[1],
                      (unsigned int)raw[2],
                      (unsigned int)(((raw[0] & 0x80U) != 0U) ? 1U : 0U),
                      (unsigned int)ds1302_time_cache.hour,
                      (unsigned int)ds1302_time_cache.minute,
                      (unsigned int)ds1302_time_cache.second);
        return;
    }

    ds1302_time_cache.second = ds1302_bcd_to_dec((uint8_t)(raw[0] & 0x7FU));
    ds1302_time_cache.minute = ds1302_bcd_to_dec(raw[1]);
    ds1302_time_cache.hour = ds1302_bcd_to_dec((uint8_t)(raw[2] & 0x3FU));
    ds1302_time_cache.day = ds1302_bcd_to_dec(raw[3]);
    ds1302_time_cache.month = ds1302_bcd_to_dec(raw[4]);
    ds1302_time_cache.week = raw[5];
    ds1302_time_cache.year = (unsigned short)(ds1302_bcd_to_dec(raw[6]) + 2000U);
}

/** @brief Initializes GPIO, starts the clock, and seeds cache or default time. */
static void ds1302_init(void)
{
    gpio_hal_config_pin(&board_ds1302_ce_pin);
    gpio_hal_config_pin(&board_ds1302_sclk_pin);
    ds1302_ce_write(0U);
    ds1302_sclk_write(0U);

    ds1302_start_clock();
    ds1302_refresh_cache();

    if (ds1302_time_is_valid(&ds1302_time_cache) == 0U) {
        DEBUG_LOG_RTC("[RTC] init cache invalid, apply default\r\n");
        ds1302_apply_default_time();
        ds1302_refresh_cache();
    }

    DEBUG_LOG_RTC("[RTC] init done %04u-%02u-%02u %02u:%02u:%02u valid=%u\r\n",
                  (unsigned int)ds1302_time_cache.year,
                  (unsigned int)ds1302_time_cache.month,
                  (unsigned int)ds1302_time_cache.day,
                  (unsigned int)ds1302_time_cache.hour,
                  (unsigned int)ds1302_time_cache.minute,
                  (unsigned int)ds1302_time_cache.second,
                  (unsigned int)ds1302_time_is_valid(&ds1302_time_cache));
}

/**
 * @brief Copies the cached calendar into caller storage.
 * @param time Output rtc_time_t; ignored when null.
 */
static void ds1302_read_time(rtc_time_t *time)
{
    if (time == 0) {
        return;
    }

    ds1302_refresh_cache();
    *time = ds1302_time_cache;
}

/**
 * @brief Writes calendar fields to the chip and updates the cache.
 * @param time Source rtc_time_t; ignored when null.
 */
static void ds1302_set_time(const rtc_time_t *time)
{
    uint8_t year_bcd;

    if (time == 0) {
        return;
    }

    ds1302_write_protect(0U);
    ds1302_write_reg(0x80U, ds1302_dec_to_bcd(time->second));
    ds1302_write_reg(0x82U, ds1302_dec_to_bcd(time->minute));
    ds1302_write_reg(0x84U, ds1302_dec_to_bcd(time->hour));
    ds1302_write_reg(0x86U, ds1302_dec_to_bcd(time->day));
    ds1302_write_reg(0x88U, ds1302_dec_to_bcd(time->month));
    ds1302_write_reg(0x8AU, time->week);
    year_bcd = ds1302_dec_to_bcd((uint8_t)(time->year >= 2000U ? (time->year - 2000U) : time->year));
    ds1302_write_reg(0x8CU, year_bcd);
    ds1302_write_protect(1U);

    ds1302_time_cache = *time;
}

/** @brief rtc_if.h DS1302 driver instance registered as RTC. */
static const rtc_driver_t ds1302_drv = {
    "ds1302",
    ds1302_init,
    ds1302_read_time,
    ds1302_set_time
};

REGISTER_DRIVER(RTC, ds1302_drv);
