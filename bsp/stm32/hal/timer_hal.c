/**
 * @file timer_hal.c
 * @brief Timer HAL: microsecond delay (TIM4), PWM setup, and update IRQ dispatch.
 */

#include "timer_hal.h"
#include "hal_common.h"
#include "irq_event.h"
#include "stm32f1_hal_map.h"
#include "misc.h"
#include "stm32f10x_rcc.h"

/** @brief Non-zero after TIM4 delay timer has been initialized once. */
static uint8_t timer_hal_delay_ready;

/**
 * @brief Enables RCC clock for TIM1 (APB2) or TIM2–4 (APB1).
 * @param TIMx Timer instance.
 */
static void timer_hal_clock_enable(TIM_TypeDef *TIMx)
{
    if (TIMx == TIM1) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    } else if (TIMx == TIM2) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    } else if (TIMx == TIM3) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    } else if (TIMx == TIM4) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    }
}

/**
 * @brief Returns NVIC IRQ number for timer update events.
 * @param TIMx Timer instance.
 * @return IRQn_Type for the instance update interrupt.
 */
static IRQn_Type timer_hal_irqn(TIM_TypeDef *TIMx)
{
    if (TIMx == TIM1) {
        return TIM1_UP_IRQn;
    }
    if (TIMx == TIM2) {
        return TIM2_IRQn;
    }
    if (TIMx == TIM3) {
        return TIM3_IRQn;
    }
    return TIM4_IRQn;
}

/**
 * @brief Maps timer instance to irq_event update source.
 * @param TIMx Timer instance.
 * @return irq_event_source_t for timer update.
 */
static irq_event_source_t timer_hal_update_source(TIM_TypeDef *TIMx)
{
    if (TIMx == TIM1) {
        return IRQ_EVENT_SOURCE_TIM1_UPDATE;
    }
    if (TIMx == TIM2) {
        return IRQ_EVENT_SOURCE_TIM2_UPDATE;
    }
    if (TIMx == TIM3) {
        return IRQ_EVENT_SOURCE_TIM3_UPDATE;
    }
    return IRQ_EVENT_SOURCE_TIM4_UPDATE;
}

/**
 * @brief One-time init of TIM4 as 1 MHz free-running counter for timer_hal_delay_us().
 */
static void timer_hal_delay_timer_init(void)
{
    TIM_TimeBaseInitTypeDef timer;

    if (timer_hal_delay_ready != 0U) {
        return;
    }

    timer_hal_clock_enable(TIM4);
    TIM_TimeBaseStructInit(&timer);
    timer.TIM_Prescaler = 72U - 1U;
    timer.TIM_Period = 0xFFFFU;
    timer.TIM_CounterMode = TIM_CounterMode_Up;
    timer.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM4, &timer);
    TIM_Cmd(TIM4, ENABLE);
    timer_hal_delay_ready = 1U;
}

/**
 * @brief Configures a timer for 1 MHz tick rate with update period @p period_us.
 * @param TIMx Timer instance (TIM1–TIM4).
 * @param period_us Auto-reload value in microseconds (ARR = period_us - 1).
 */
void timer_hal_init_us(hal_timer_id_t instance, uint16_t period_us)
{
    TIM_TypeDef *TIMx = stm32f1_timer(instance);
    TIM_TimeBaseInitTypeDef timer;

    if (TIMx == 0) {
        return;
    }

    timer_hal_clock_enable(TIMx);
    TIM_TimeBaseStructInit(&timer);
    timer.TIM_Prescaler = 72U - 1U;
    timer.TIM_Period = period_us - 1U;
    timer.TIM_CounterMode = TIM_CounterMode_Up;
    timer.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIMx, &timer);
    TIM_Cmd(TIMx, ENABLE);
}

/**
 * @brief Busy-waits for @p us microseconds using TIM4 as a free-running counter.
 * @param us Delay in microseconds.
 */
void timer_hal_delay_us(uint32_t us)
{
    uint16_t start;

    timer_hal_delay_timer_init();
    start = (uint16_t)TIM_GetCounter(TIM4);

    while (us > 0U) {
        uint16_t chunk = (us > 60000U) ? 60000U : (uint16_t)us;
        start = (uint16_t)TIM_GetCounter(TIM4);
        while ((uint16_t)(TIM_GetCounter(TIM4) - start) < chunk) {
        }
        us -= chunk;
    }
}

/**
 * @brief Configures one timer channel for PWM1 output mode.
 * @param TIMx Timer instance.
 * @param channel Channel number (1–4).
 * @param compare Initial compare (pulse) value.
 */
static void timer_hal_configure_pwm_channel(TIM_TypeDef *TIMx, uint8_t channel, uint16_t compare)
{
    TIM_OCInitTypeDef oc;

    TIM_OCStructInit(&oc);
    oc.TIM_OCMode = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse = compare;
    oc.TIM_OCPolarity = TIM_OCPolarity_High;

    switch (channel) {
    case 1U:
        TIM_OC1Init(TIMx, &oc);
        TIM_OC1PreloadConfig(TIMx, TIM_OCPreload_Enable);
        break;
    case 2U:
        TIM_OC2Init(TIMx, &oc);
        TIM_OC2PreloadConfig(TIMx, TIM_OCPreload_Enable);
        break;
    case 3U:
        TIM_OC3Init(TIMx, &oc);
        TIM_OC3PreloadConfig(TIMx, TIM_OCPreload_Enable);
        break;
    case 4U:
        TIM_OC4Init(TIMx, &oc);
        TIM_OC4PreloadConfig(TIMx, TIM_OCPreload_Enable);
        break;
    default:
        break;
    }
}

/**
 * @brief Initializes timer timebase for PWM with preload enabled.
 * @param TIMx Timer instance.
 * @param period Auto-reload value (ARR).
 * @param prescaler Prescaler minus one (PSC register value).
 */
static void timer_hal_pwm_timebase_init(TIM_TypeDef *TIMx, uint16_t period, uint16_t prescaler)
{
    TIM_TimeBaseInitTypeDef timer;

    timer_hal_clock_enable(TIMx);
    TIM_TimeBaseStructInit(&timer);
    timer.TIM_Prescaler = prescaler;
    timer.TIM_Period = period;
    timer.TIM_CounterMode = TIM_CounterMode_Up;
    timer.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIMx, &timer);
    TIM_ARRPreloadConfig(TIMx, ENABLE);
    TIM_Cmd(TIMx, ENABLE);
}

/**
 * @brief Initializes PWM on @p cfg->channel with GPIO and timebase from @p cfg.
 * @param cfg PWM configuration (instance and pin must be valid).
 * @return HAL_OK or HAL_ERR_PARAM.
 */
hal_status_t timer_hal_pwm_init(const timer_hal_pwm_config_t *cfg)
{
    TIM_TypeDef *TIMx;

    if (cfg == 0) {
        return HAL_ERR_PARAM;
    }

    TIMx = stm32f1_timer(cfg->instance);
    if (TIMx == 0) {
        return HAL_ERR_PARAM;
    }

    gpio_hal_apply_remap(cfg->remap);
    gpio_hal_config_pin(&cfg->pin);
    timer_hal_pwm_timebase_init(TIMx, cfg->period, cfg->prescaler);
    timer_hal_configure_pwm_channel(TIMx, cfg->channel, 0U);
    return HAL_OK;
}

/**
 * @brief Sets the compare register for PWM channel 1–4.
 * @param TIMx Timer instance.
 * @param channel Channel number (1–4).
 * @param compare Compare value (pulse width).
 */
void timer_hal_pwm_set_compare(hal_timer_id_t instance, uint8_t channel, uint16_t compare)
{
    TIM_TypeDef *TIMx = stm32f1_timer(instance);

    if (TIMx == 0) {
        return;
    }

    switch (channel) {
    case 1U:
        TIM_SetCompare1(TIMx, compare);
        break;
    case 2U:
        TIM_SetCompare2(TIMx, compare);
        break;
    case 3U:
        TIM_SetCompare3(TIMx, compare);
        break;
    case 4U:
        TIM_SetCompare4(TIMx, compare);
        break;
    default:
        break;
    }
}

/**
 * @brief Enables timer update interrupt and NVIC for @p TIMx.
 * @param TIMx Timer instance.
 */
void timer_hal_enable_update_irq(hal_timer_id_t instance)
{
    TIM_TypeDef *TIMx = stm32f1_timer(instance);
    NVIC_InitTypeDef nvic;

    if (TIMx == 0) {
        return;
    }

    nvic.NVIC_IRQChannel = timer_hal_irqn(TIMx);
    nvic.NVIC_IRQChannelPreemptionPriority = 2U;
    nvic.NVIC_IRQChannelSubPriority = 1U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    TIM_ITConfig(TIMx, TIM_IT_Update, ENABLE);
}

/**
 * @brief Timer update IRQ handler; posts irq_event and clears update flag.
 * @param TIMx Timer instance.
 */
void timer_hal_irq_handler(hal_timer_id_t instance)
{
    TIM_TypeDef *TIMx = stm32f1_timer(instance);

    if (TIMx == 0) {
        return;
    }

    if (TIM_GetITStatus(TIMx, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIMx, TIM_IT_Update);
        irq_event_post_from_isr(timer_hal_update_source(TIMx), 0U);
    }
}
