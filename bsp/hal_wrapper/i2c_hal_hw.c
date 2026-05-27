#include "i2c_hal.h"
#include "hal_common.h"
#include "stm32f10x_rcc.h"

#if HAL_I2C_USE_SOFT
typedef char i2c_hal_hw_disabled[-1];
#else

typedef struct i2c_hal_runtime {
    I2C_InitTypeDef init;
    uint32_t timeout_us;
    hal_pin_t scl;
    hal_pin_t sda;
    uint8_t configured;
} i2c_hal_runtime_t;

typedef struct i2c_hal_event_ctx {
    I2C_TypeDef *instance;
    uint32_t event;
} i2c_hal_event_ctx_t;

static i2c_hal_runtime_t i2c_hal_runtime[2];

static i2c_hal_runtime_t *i2c_hal_runtime_get(I2C_TypeDef *I2Cx)
{
    if (I2Cx == I2C1) {
        return &i2c_hal_runtime[0];
    }
    if (I2Cx == I2C2) {
        return &i2c_hal_runtime[1];
    }
    return 0;
}

static void i2c_hal_clock_enable(I2C_TypeDef *I2Cx)
{
    if (I2Cx == I2C1) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    } else if (I2Cx == I2C2) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
    }
}

static uint8_t i2c_hal_poll_event(void *ctx)
{
    const i2c_hal_event_ctx_t *event_ctx = (const i2c_hal_event_ctx_t *)ctx;

    if (event_ctx == 0) {
        return 0U;
    }

    return (I2C_CheckEvent(event_ctx->instance, event_ctx->event) != ERROR) ? 1U : 0U;
}

static uint8_t i2c_hal_poll_not_busy(void *ctx)
{
    I2C_TypeDef *I2Cx = (I2C_TypeDef *)ctx;

    if (I2Cx == 0) {
        return 0U;
    }

    return (I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY) == RESET) ? 1U : 0U;
}

static hal_status_t i2c_hal_wait_event(I2C_TypeDef *I2Cx, uint32_t event, uint32_t timeout_us)
{
    i2c_hal_event_ctx_t ctx;

    ctx.instance = I2Cx;
    ctx.event = event;
    return hal_wait_flag_us(i2c_hal_poll_event, &ctx, timeout_us);
}

static hal_status_t i2c_hal_wait_not_busy(I2C_TypeDef *I2Cx, uint32_t timeout_us)
{
    return hal_wait_flag_us(i2c_hal_poll_not_busy, I2Cx, timeout_us);
}

static hal_status_t i2c_hal_peripheral_init(const i2c_hal_config_t *cfg, i2c_hal_runtime_t *runtime)
{
    I2C_InitTypeDef i2c;

    if ((cfg == 0) || (runtime == 0)) {
        return HAL_ERR_PARAM;
    }

    gpio_hal_apply_remap(cfg->remap);
    gpio_hal_config_pin(&cfg->scl);
    gpio_hal_config_pin(&cfg->sda);

    i2c_hal_clock_enable(cfg->instance);
    I2C_StructInit(&i2c);
    i2c.I2C_Mode = I2C_Mode_I2C;
    i2c.I2C_DutyCycle = I2C_DutyCycle_2;
    i2c.I2C_OwnAddress1 = 0x30;
    i2c.I2C_Ack = I2C_Ack_Enable;
    i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    i2c.I2C_ClockSpeed = cfg->speed_hz;
    I2C_Init(cfg->instance, &i2c);
    I2C_Cmd(cfg->instance, ENABLE);

    runtime->init = i2c;
    runtime->timeout_us = cfg->timeout_us;
    runtime->scl = cfg->scl;
    runtime->sda = cfg->sda;
    runtime->configured = 1U;

    return HAL_OK;
}

hal_status_t i2c_hal_init(const i2c_hal_config_t *cfg)
{
    i2c_hal_runtime_t *runtime;

    if ((cfg == 0) || (cfg->instance == 0)) {
        return HAL_ERR_PARAM;
    }

    runtime = i2c_hal_runtime_get(cfg->instance);
    if (runtime == 0) {
        return HAL_ERR_PARAM;
    }

    return i2c_hal_peripheral_init(cfg, runtime);
}

static hal_status_t i2c_hal_send_address(I2C_TypeDef *I2Cx, uint8_t address,
                                         uint8_t direction, uint32_t timeout_us)
{
    hal_status_t status;

    I2C_Send7bitAddress(I2Cx, address, direction);
    if (direction == I2C_Direction_Transmitter) {
        status = i2c_hal_wait_event(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, timeout_us);
    } else {
        status = i2c_hal_wait_event(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, timeout_us);
    }

    if (status != HAL_OK) {
        return HAL_ERR_NACK;
    }

    return HAL_OK;
}

static hal_status_t i2c_hal_send_byte(I2C_TypeDef *I2Cx, uint8_t data, uint32_t timeout_us)
{
    I2C_SendData(I2Cx, data);
    return i2c_hal_wait_event(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED, timeout_us);
}

hal_status_t i2c_hal_write(I2C_TypeDef *I2Cx, uint8_t address, uint8_t reg,
                           const uint8_t *data, uint16_t len)
{
    i2c_hal_runtime_t *runtime = i2c_hal_runtime_get(I2Cx);
    uint32_t timeout_us = I2C_HAL_DEFAULT_TIMEOUT_US;
    uint16_t i;
    hal_status_t status;

    if ((data == 0) && (len > 0U)) {
        return HAL_ERR_PARAM;
    }

    if ((runtime != 0) && (runtime->configured != 0U)) {
        timeout_us = runtime->timeout_us;
    }

    status = i2c_hal_wait_not_busy(I2Cx, timeout_us);
    if (status != HAL_OK) {
        return status;
    }

    I2C_GenerateSTART(I2Cx, ENABLE);
    status = i2c_hal_wait_event(I2Cx, I2C_EVENT_MASTER_MODE_SELECT, timeout_us);
    if (status != HAL_OK) {
        I2C_GenerateSTOP(I2Cx, ENABLE);
        return status;
    }

    status = i2c_hal_send_address(I2Cx, address, I2C_Direction_Transmitter, timeout_us);
    if (status != HAL_OK) {
        I2C_GenerateSTOP(I2Cx, ENABLE);
        return status;
    }

    status = i2c_hal_send_byte(I2Cx, reg, timeout_us);
    if (status != HAL_OK) {
        I2C_GenerateSTOP(I2Cx, ENABLE);
        return status;
    }

    for (i = 0; i < len; ++i) {
        status = i2c_hal_send_byte(I2Cx, data[i], timeout_us);
        if (status != HAL_OK) {
            I2C_GenerateSTOP(I2Cx, ENABLE);
            return status;
        }
    }

    I2C_GenerateSTOP(I2Cx, ENABLE);
    return HAL_OK;
}

hal_status_t i2c_hal_read(I2C_TypeDef *I2Cx, uint8_t address, uint8_t reg,
                          uint8_t *data, uint16_t len)
{
    i2c_hal_runtime_t *runtime = i2c_hal_runtime_get(I2Cx);
    uint32_t timeout_us = I2C_HAL_DEFAULT_TIMEOUT_US;
    uint16_t i;
    hal_status_t status;

    if ((data == 0) || (len == 0U)) {
        return HAL_ERR_PARAM;
    }

    if ((runtime != 0) && (runtime->configured != 0U)) {
        timeout_us = runtime->timeout_us;
    }

    status = i2c_hal_wait_not_busy(I2Cx, timeout_us);
    if (status != HAL_OK) {
        return status;
    }

    I2C_GenerateSTART(I2Cx, ENABLE);
    status = i2c_hal_wait_event(I2Cx, I2C_EVENT_MASTER_MODE_SELECT, timeout_us);
    if (status != HAL_OK) {
        I2C_GenerateSTOP(I2Cx, ENABLE);
        return status;
    }

    status = i2c_hal_send_address(I2Cx, address, I2C_Direction_Transmitter, timeout_us);
    if (status != HAL_OK) {
        I2C_GenerateSTOP(I2Cx, ENABLE);
        return status;
    }

    status = i2c_hal_send_byte(I2Cx, reg, timeout_us);
    if (status != HAL_OK) {
        I2C_GenerateSTOP(I2Cx, ENABLE);
        return status;
    }

    I2C_GenerateSTART(I2Cx, ENABLE);
    status = i2c_hal_wait_event(I2Cx, I2C_EVENT_MASTER_MODE_SELECT, timeout_us);
    if (status != HAL_OK) {
        I2C_GenerateSTOP(I2Cx, ENABLE);
        return status;
    }

    status = i2c_hal_send_address(I2Cx, address, I2C_Direction_Receiver, timeout_us);
    if (status != HAL_OK) {
        I2C_GenerateSTOP(I2Cx, ENABLE);
        return status;
    }

    if (len == 1U) {
        I2C_AcknowledgeConfig(I2Cx, DISABLE);
        status = i2c_hal_wait_event(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED, timeout_us);
        if (status != HAL_OK) {
            I2C_AcknowledgeConfig(I2Cx, ENABLE);
            I2C_GenerateSTOP(I2Cx, ENABLE);
            return status;
        }
        data[0] = I2C_ReceiveData(I2Cx);
        I2C_GenerateSTOP(I2Cx, ENABLE);
        I2C_AcknowledgeConfig(I2Cx, ENABLE);
        return HAL_OK;
    }

    for (i = 0; i < len; ++i) {
        if (i == (len - 1U)) {
            I2C_AcknowledgeConfig(I2Cx, DISABLE);
        }

        status = i2c_hal_wait_event(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED, timeout_us);
        if (status != HAL_OK) {
            I2C_AcknowledgeConfig(I2Cx, ENABLE);
            I2C_GenerateSTOP(I2Cx, ENABLE);
            return status;
        }

        data[i] = I2C_ReceiveData(I2Cx);
    }

    I2C_GenerateSTOP(I2Cx, ENABLE);
    I2C_AcknowledgeConfig(I2Cx, ENABLE);
    return HAL_OK;
}

hal_status_t i2c_hal_bus_recover(I2C_TypeDef *I2Cx, hal_pin_t scl, hal_pin_t sda)
{
    i2c_hal_runtime_t *runtime = i2c_hal_runtime_get(I2Cx);
    hal_pin_t scl_od = scl;
    hal_pin_t sda_od = sda;
    uint8_t i;

    if (I2Cx == 0) {
        return HAL_ERR_PARAM;
    }

    I2C_Cmd(I2Cx, DISABLE);
    I2C_SoftwareResetCmd(I2Cx, ENABLE);
    I2C_SoftwareResetCmd(I2Cx, DISABLE);

    scl_od.mode = GPIO_HAL_MODE_OUT_OD;
    sda_od.mode = GPIO_HAL_MODE_OUT_OD;
    gpio_hal_config_pin(&scl_od);
    gpio_hal_config_pin(&sda_od);
    gpio_hal_write(sda_od.port, sda_od.pin, 1U);
    gpio_hal_write(scl_od.port, scl_od.pin, 1U);

    for (i = 0; i < 9U; ++i) {
        gpio_hal_write(scl_od.port, scl_od.pin, 0U);
        hal_delay_us(5U);
        gpio_hal_write(scl_od.port, scl_od.pin, 1U);
        hal_delay_us(5U);
    }

    gpio_hal_write(sda_od.port, sda_od.pin, 0U);
    hal_delay_us(5U);
    gpio_hal_write(scl_od.port, scl_od.pin, 1U);
    hal_delay_us(5U);
    gpio_hal_write(sda_od.port, sda_od.pin, 1U);
    hal_delay_us(5U);

    if ((runtime != 0) && (runtime->configured != 0U)) {
        i2c_hal_config_t cfg;

        cfg.instance = I2Cx;
        cfg.speed_hz = runtime->init.I2C_ClockSpeed;
        cfg.scl = runtime->scl;
        cfg.sda = runtime->sda;
        cfg.remap = GPIO_HAL_REMAP_NONE;
        cfg.timeout_us = runtime->timeout_us;
        return i2c_hal_init(&cfg);
    }

    return HAL_OK;
}

#endif
