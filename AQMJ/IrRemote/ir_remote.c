#include "ir_remote.h"
#include "pin_config.h"
#include "main.h"

static volatile MotionCmd_t s_remote_cmd = CMD_STOP;
static volatile uint32_t s_last_remote_ms = 0U;
static volatile uint8_t s_ir_last_cmd = 0U;
static volatile uint32_t s_ir_edge_count = 0U;
static volatile uint16_t s_ir_last_edge_us = 0U;
static volatile uint16_t s_ir_dbg_lead_low = 0U;
static volatile uint16_t s_ir_dbg_lead_high = 0U;
static volatile uint8_t s_ir_dbg_fail_stage = 0U;
static volatile uint32_t s_ir_raw_data = 0U;
static volatile uint8_t s_ir_bit_idx = 0U;
static volatile uint8_t s_ir_lead_low_missed = 0U;

typedef enum
{
    IR_RX_IDLE = 0,
    IR_RX_WAIT_LEAD_RISE,
    IR_RX_WAIT_LEAD_FALL,
    IR_RX_WAIT_BIT_RISE,
    IR_RX_WAIT_BIT_FALL
} IrRxState_t;

static volatile IrRxState_t s_ir_state = IR_RX_IDLE;

/* HX1838 17-key remote, NEC protocol command bytes. */
#define IR_KEY_UP      0x52U
#define IR_KEY_DOWN    0x18U
#define IR_KEY_LEFT    0x5AU
#define IR_KEY_RIGHT   0x08U
#define IR_KEY_OK      0x1CU

static void IrDecoder_Reset(void)
{
    s_ir_state = IR_RX_IDLE;
    s_ir_raw_data = 0U;
    s_ir_bit_idx = 0U;
    s_ir_lead_low_missed = 0U;
}

static uint8_t InRangeU32(uint32_t v, uint32_t lo, uint32_t hi)
{
    return (v >= lo && v <= hi) ? 1U : 0U;
}

static uint8_t BitReverse8(uint8_t x)
{
    x = (uint8_t)(((x & 0xF0U) >> 4) | ((x & 0x0FU) << 4));
    x = (uint8_t)(((x & 0xCCU) >> 2) | ((x & 0x33U) << 2));
    x = (uint8_t)(((x & 0xAAU) >> 1) | ((x & 0x55U) << 1));
    return x;
}

static uint8_t IsKnownKey(uint8_t key)
{
    return (key == IR_KEY_UP || key == IR_KEY_DOWN || key == IR_KEY_LEFT ||
            key == IR_KEY_RIGHT || key == IR_KEY_OK) ? 1U : 0U;
}

static void IrRemote_ApplyKey(uint8_t key)
{
    s_ir_last_cmd = key;
    switch (key)
    {
    case IR_KEY_UP:
        s_remote_cmd = CMD_FORWARD;
        break;
    case IR_KEY_DOWN:
        s_remote_cmd = CMD_BACKWARD;
        break;
    case IR_KEY_LEFT:
        s_remote_cmd = CMD_LEFT;
        break;
    case IR_KEY_RIGHT:
        s_remote_cmd = CMD_RIGHT;
        break;
    case IR_KEY_OK:
        s_remote_cmd = CMD_STOP;
        break;
    default:
        return;
    }
    s_last_remote_ms = GetTickMs();
}

static void IrTimer_Init(void)
{
    TIM_TimeBaseInitTypeDef tim;
    uint32_t timer_clk;
    uint16_t psc;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    SystemCoreClockUpdate();
    timer_clk = SystemCoreClock / 2U;
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1)
    {
        timer_clk *= 2U;
    }
    psc = (uint16_t)((timer_clk / 1000000U) - 1U);

    TIM_DeInit(TIM2);
    tim.TIM_Prescaler = psc;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    tim.TIM_Period = 0xFFFFU;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_RepetitionCounter = 0U;
    TIM_TimeBaseInit(TIM2, &tim);
    TIM_Cmd(TIM2, ENABLE);
}

void IrRemote_Init(void)
{
    GPIO_InitTypeDef gpio;
    EXTI_InitTypeDef exti;
    NVIC_InitTypeDef nvic;

    IrTimer_Init();

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    gpio.GPIO_Pin = IR_REMOTE_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(IR_REMOTE_PORT, &gpio);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
    EXTI_ClearITPendingBit(EXTI_Line0);
    exti.EXTI_Line = EXTI_Line0;
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);

    nvic.NVIC_IRQChannel = EXTI0_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1U;
    nvic.NVIC_IRQChannelSubPriority = 1U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
}

void IrRemote_Poll(void)
{
    (void)0;
}

MotionCmd_t IrRemote_GetMotionCommand(void)
{
    if ((GetTickMs() - s_last_remote_ms) < 400U)
    {
        return s_remote_cmd;
    }
    return CMD_STOP;
}

void IrRemote_OnEdge(void)
{
    uint16_t now_us = (uint16_t)TIM2->CNT;
    uint16_t delta_us = (uint16_t)(now_us - s_ir_last_edge_us);
    uint8_t level_now = (GPIO_ReadInputDataBit(IR_REMOTE_PORT, IR_REMOTE_PIN) == Bit_SET) ? 1U : 0U;
    uint8_t addr;
    uint8_t addr_inv;
    uint8_t cmd;
    uint8_t cmd_inv;
    s_ir_last_edge_us = now_us;
    s_ir_edge_count++;

    if (delta_us > 15000U)
    {
        IrDecoder_Reset();
        s_ir_dbg_fail_stage = 0U;
    }

    if (level_now == 0U)
    {
        if (s_ir_state == IR_RX_WAIT_LEAD_FALL)
        {
            s_ir_dbg_lead_high = (uint16_t)delta_us;
            if (InRangeU32(delta_us, 1200U, 3200U) != 0U)
            {
                if (s_ir_last_cmd != 0U)
                {
                    IrRemote_ApplyKey(s_ir_last_cmd);
                }
                s_ir_dbg_fail_stage = 0U;
                IrDecoder_Reset();
                return;
            }
            if (InRangeU32(delta_us, 3200U, 6200U) == 0U)
            {
                s_ir_dbg_fail_stage = 5U;
                IrDecoder_Reset();
                return;
            }
            s_ir_raw_data = 0U;
            s_ir_bit_idx = 0U;
            s_ir_lead_low_missed = 0U;
            s_ir_state = IR_RX_WAIT_BIT_RISE;
            return;
        }

        if (s_ir_state == IR_RX_WAIT_BIT_FALL)
        {
            if (InRangeU32(delta_us, 100U, 1000U) != 0U)
            {
                /* NEC bit 0 */
            }
            else if (InRangeU32(delta_us, 1100U, 2500U) != 0U)
            {
                s_ir_raw_data |= ((uint32_t)1U << s_ir_bit_idx);
            }
            else
            {
                s_ir_dbg_fail_stage = 9U;
                IrDecoder_Reset();
                return;
            }

            s_ir_bit_idx++;
            if (s_ir_bit_idx >= 32U)
            {
                uint8_t candidate;
                addr = (uint8_t)(s_ir_raw_data & 0xFFU);
                addr_inv = (uint8_t)((s_ir_raw_data >> 8) & 0xFFU);
                cmd = (uint8_t)((s_ir_raw_data >> 16) & 0xFFU);
                cmd_inv = (uint8_t)((s_ir_raw_data >> 24) & 0xFFU);
                s_ir_last_cmd = cmd;

                candidate = cmd;
                if (IsKnownKey(candidate) != 0U)
                {
                    IrRemote_ApplyKey(candidate);
                    s_ir_dbg_fail_stage = 0U;
                    IrDecoder_Reset();
                    return;
                }

                candidate = BitReverse8(cmd);
                if (IsKnownKey(candidate) != 0U)
                {
                    IrRemote_ApplyKey(candidate);
                    s_ir_dbg_fail_stage = 0U;
                    IrDecoder_Reset();
                    return;
                }

                candidate = cmd_inv;
                if (IsKnownKey(candidate) != 0U)
                {
                    IrRemote_ApplyKey(candidate);
                    s_ir_dbg_fail_stage = 0U;
                    IrDecoder_Reset();
                    return;
                }

                candidate = BitReverse8(cmd_inv);
                if (IsKnownKey(candidate) != 0U)
                {
                    IrRemote_ApplyKey(candidate);
                    s_ir_dbg_fail_stage = 0U;
                    IrDecoder_Reset();
                    return;
                }

                if ((uint8_t)(addr + addr_inv) != 0xFFU)
                {
                    s_ir_dbg_fail_stage = 11U;
                    IrDecoder_Reset();
                    return;
                }
                if ((uint8_t)(cmd + cmd_inv) != 0xFFU)
                {
                    s_ir_dbg_fail_stage = 10U;
                    IrDecoder_Reset();
                    return;
                }

                s_ir_dbg_fail_stage = 13U;
                IrDecoder_Reset();
                return;
            }
            s_ir_state = IR_RX_WAIT_BIT_RISE;
            return;
        }

        if (s_ir_state == IR_RX_IDLE)
        {
            if (delta_us > 3000U)
            {
                s_ir_state = IR_RX_WAIT_LEAD_RISE;
                s_ir_dbg_fail_stage = 0U;
            }
            return;
        }

        s_ir_dbg_fail_stage = 1U;
        IrDecoder_Reset();
        return;
    }

    if (s_ir_state == IR_RX_WAIT_LEAD_RISE)
    {
        s_ir_dbg_lead_low = (uint16_t)delta_us;
        if (InRangeU32(delta_us, 6800U, 11200U) == 0U)
        {
            if (InRangeU32(delta_us, 200U, 1200U) != 0U)
            {
                s_ir_lead_low_missed = 1U;
                s_ir_state = IR_RX_WAIT_LEAD_FALL;
                return;
            }
            s_ir_dbg_fail_stage = 4U;
            IrDecoder_Reset();
            return;
        }
        s_ir_lead_low_missed = 0U;
        s_ir_state = IR_RX_WAIT_LEAD_FALL;
        return;
    }

    if (s_ir_state == IR_RX_WAIT_BIT_RISE)
    {
        if (InRangeU32(delta_us, 150U, 1200U) == 0U)
        {
            s_ir_dbg_fail_stage = 8U;
            IrDecoder_Reset();
            return;
        }
        s_ir_state = IR_RX_WAIT_BIT_FALL;
        return;
    }

    if (s_ir_state == IR_RX_IDLE)
    {
        return;
    }

    s_ir_dbg_fail_stage = 1U;
    IrDecoder_Reset();
}

uint8_t IrRemote_GetLastCode(void)
{
    return s_ir_last_cmd;
}

uint8_t IrRemote_GetPinLevel(void)
{
    return (GPIO_ReadInputDataBit(IR_REMOTE_PORT, IR_REMOTE_PIN) == Bit_SET) ? 1U : 0U;
}

uint32_t IrRemote_GetEdgeCount(void)
{
    return s_ir_edge_count;
}

uint16_t IrRemote_GetLeadLowUs(void)
{
    return s_ir_dbg_lead_low;
}

uint16_t IrRemote_GetLeadHighUs(void)
{
    return s_ir_dbg_lead_high;
}

uint8_t IrRemote_GetFailStage(void)
{
    return s_ir_dbg_fail_stage;
}
