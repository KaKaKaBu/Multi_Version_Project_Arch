#ifndef DRIVER_CONFIGS_H
#define DRIVER_CONFIGS_H

#include <stdint.h>
#include "gpio_hal.h"
#include "i2c_hal.h"
#include "spi_hal.h"
#include "timer_hal.h"
#include "usart_hal.h"

typedef struct gpio_output_driver_config {
    hal_pin_t pin;
    uint8_t active_high;
} gpio_output_driver_config_t;

typedef struct gpio_input_driver_config {
    const hal_pin_t *pins;
    const hal_pin_t *const *pin_refs;
    uint8_t count;
    uint8_t active_low;
} gpio_input_driver_config_t;

typedef struct gpio_probe_driver_config {
    hal_pin_t pin;
    uint8_t active_low;
} gpio_probe_driver_config_t;

typedef struct one_wire_sensor_config {
    hal_pin_t pin;
} one_wire_sensor_config_t;

typedef struct i2c_device_config {
    hal_i2c_id_t instance;
    uint32_t speed_hz;
    hal_pin_t scl;
    hal_pin_t sda;
    gpio_hal_remap_t remap;
    uint8_t address;
} i2c_device_config_t;

typedef struct usart_device_config {
    hal_usart_id_t instance;
    uint32_t baudrate;
    hal_pin_t tx;
    hal_pin_t rx;
    gpio_hal_remap_t remap;
    usart_hal_tx_mode_t tx_mode;
    hal_dma_channel_id_t tx_dma_channel;
    uint8_t instance_id;
} usart_device_config_t;

typedef struct esp8266_driver_config {
    usart_device_config_t usart;
    hal_pin_t ch_pd;
    hal_pin_t rst;
    uint8_t debug_trace_enable;
} esp8266_driver_config_t;

typedef struct adc_channel_driver_config {
    hal_adc_id_t instance;
    hal_adc_channel_t channel;
    hal_pin_t pin;
    uint8_t invert;
} adc_channel_driver_config_t;

typedef struct pm25_driver_config {
    adc_channel_driver_config_t adc;
    hal_pin_t led_pin;
    uint8_t led_active_level;
    float vref_mv;
    float vout_scale;
    float density_slope;
    float density_offset;
    float density_max;
} pm25_driver_config_t;

typedef struct ph_driver_config {
    adc_channel_driver_config_t adc;
    float voltage_coeff;
    float voltage_offset;
} ph_driver_config_t;

typedef struct msp20_driver_config {
    adc_channel_driver_config_t adc;
    float sys_k;
    float sys_offset;
    float dia_k;
    float dia_offset;
    float dia_ratio;
} msp20_driver_config_t;

typedef struct hx711_driver_config {
    hal_pin_t sck;
    hal_pin_t data;
    float scale;
    unsigned long offset;
} hx711_driver_config_t;

typedef struct hcsr04_driver_config {
    hal_pin_t trig;
    hal_pin_t echo;
} hcsr04_driver_config_t;

typedef struct stepmotor_driver_config {
    hal_pin_t phase_a;
    hal_pin_t phase_b;
    hal_pin_t phase_c;
    hal_pin_t phase_d;
} stepmotor_driver_config_t;

typedef struct servo_driver_config {
    hal_timer_id_t timer;
    uint16_t channel;
    uint16_t prescaler;
    uint16_t period;
    hal_pin_t pin;
    gpio_hal_remap_t remap;
} servo_driver_config_t;

typedef struct spi_device_config {
    uint8_t bus_id;
    hal_spi_id_t instance;
    hal_pin_t sck;
    hal_pin_t mosi;
    hal_pin_t miso;
    gpio_hal_remap_t remap;
    uint16_t prescaler;
    uint8_t cpol;
    uint8_t cpha;
    hal_dma_channel_id_t tx_dma_channel;
    hal_dma_channel_id_t rx_dma_channel;
    hal_pin_t cs;
    hal_pin_t aux;
} spi_device_config_t;

typedef struct adc0832_driver_config {
    hal_pin_t cs;
    hal_pin_t clk;
    hal_pin_t dio;
} adc0832_driver_config_t;

typedef struct mq_adc0832_driver_config {
    adc0832_driver_config_t adc;
    uint8_t channel;
    uint16_t max_ppm;
} mq_adc0832_driver_config_t;

typedef struct ds1302_driver_config {
    hal_pin_t ce;
    hal_pin_t data;
    hal_pin_t sclk;
} ds1302_driver_config_t;

typedef struct hc595_driver_config {
    hal_pin_t load;
    hal_pin_t sclk;
    hal_pin_t sdi;
} hc595_driver_config_t;

typedef struct bmp180_driver_config {
    i2c_device_config_t i2c;
    int32_t pressure_revise;
} bmp180_driver_config_t;

#endif
