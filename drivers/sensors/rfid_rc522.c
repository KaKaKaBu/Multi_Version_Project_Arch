#include "rfid_if.h"
#include "spi_hal.h"
#include "gpio_hal.h"
#include "hal_common.h"
#include "board_config.h"
#include "driver_core.h"

#if HAL_SPI_USE_HW

#include <string.h>

#define RC522_MAXRLEN 18U

#define RC522_PCD_IDLE 0x00U
#define RC522_PCD_AUTHENT 0x0EU
#define RC522_PCD_TRANSCEIVE 0x0CU
#define RC522_PCD_CALCCRC 0x03U

#define RC522_PICC_REQALL 0x52U
#define RC522_PICC_ANTICOLL1 0x93U

#define RC522_MI_OK 0x26U
#define RC522_MI_ERR 0xBBU

#define RC522_CommandReg 0x01U
#define RC522_ComIEnReg 0x02U
#define RC522_ComIrqReg 0x04U
#define RC522_DivIrqReg 0x05U
#define RC522_ErrorReg 0x06U
#define RC522_Status2Reg 0x08U
#define RC522_FIFODataReg 0x09U
#define RC522_FIFOLevelReg 0x0AU
#define RC522_ControlReg 0x0CU
#define RC522_BitFramingReg 0x0DU
#define RC522_CollReg 0x0EU
#define RC522_ModeReg 0x11U
#define RC522_TxControlReg 0x14U
#define RC522_TxAutoReg 0x15U
#define RC522_RxSelReg 0x17U
#define RC522_RFCfgReg 0x26U
#define RC522_TModeReg 0x2AU
#define RC522_TPrescalerReg 0x2BU
#define RC522_TReloadRegH 0x2CU
#define RC522_TReloadRegL 0x2DU
#define RC522_CRCResultRegM 0x22U
#define RC522_CRCResultRegL 0x21U

static uint8_t rc522_uid[4];
static uint8_t rc522_card_present;

static void rc522_cs_enable(void)
{
    gpio_hal_write(board_rc522_cs_pin.port, board_rc522_cs_pin.pin, 0U);
}

static void rc522_cs_disable(void)
{
    gpio_hal_write(board_rc522_cs_pin.port, board_rc522_cs_pin.pin, 1U);
}

static void rc522_rst_enable(void)
{
    gpio_hal_write(board_rc522_rst_pin.port, board_rc522_rst_pin.pin, 0U);
}

static void rc522_rst_disable(void)
{
    gpio_hal_write(board_rc522_rst_pin.port, board_rc522_rst_pin.pin, 1U);
}

static uint8_t rc522_spi_transfer(uint8_t byte)
{
    return spi_hal_transfer_byte(BOARD_RC522_SPI_BUS_ID, byte);
}

static uint8_t rc522_read_raw(uint8_t address)
{
    uint8_t value;
    uint8_t addr = (uint8_t)(((address << 1U) & 0x7EU) | 0x80U);

    rc522_cs_enable();
    (void)rc522_spi_transfer(addr);
    value = rc522_spi_transfer(0x00U);
    rc522_cs_disable();
    return value;
}

static void rc522_write_raw(uint8_t address, uint8_t value)
{
    uint8_t addr = (uint8_t)((address << 1U) & 0x7EU);

    rc522_cs_enable();
    (void)rc522_spi_transfer(addr);
    (void)rc522_spi_transfer(value);
    rc522_cs_disable();
}

static void rc522_set_bit_mask(uint8_t reg, uint8_t mask)
{
    uint8_t temp = rc522_read_raw(reg);

    rc522_write_raw(reg, (uint8_t)(temp | mask));
}

static void rc522_clear_bit_mask(uint8_t reg, uint8_t mask)
{
    uint8_t temp = rc522_read_raw(reg);

    rc522_write_raw(reg, (uint8_t)(temp & (uint8_t)(~mask)));
}

static void rc522_antenna_on(void)
{
    uint8_t temp = rc522_read_raw(RC522_TxControlReg);

    if ((temp & 0x03U) == 0U) {
        rc522_set_bit_mask(RC522_TxControlReg, 0x03U);
    }
}

static void rc522_reset(void)
{
    rc522_rst_disable();
    hal_delay_us(1U);
    rc522_rst_enable();
    hal_delay_us(1U);
    rc522_rst_disable();
    hal_delay_us(1U);

    rc522_write_raw(RC522_CommandReg, 0x0FU);
    while ((rc522_read_raw(RC522_CommandReg) & 0x10U) != 0U) {
    }

    hal_delay_us(1U);
    rc522_write_raw(RC522_ModeReg, 0x3DU);
    rc522_write_raw(RC522_TReloadRegL, 30U);
    rc522_write_raw(RC522_TReloadRegH, 0U);
    rc522_write_raw(RC522_TModeReg, 0x8DU);
    rc522_write_raw(RC522_TPrescalerReg, 0x3EU);
    rc522_write_raw(RC522_TxAutoReg, 0x40U);
}

static void rc522_config_iso_type_a(void)
{
    rc522_clear_bit_mask(RC522_Status2Reg, 0x08U);
    rc522_write_raw(RC522_ModeReg, 0x3DU);
    rc522_write_raw(RC522_RxSelReg, 0x86U);
    rc522_write_raw(RC522_RFCfgReg, 0x7FU);
    rc522_write_raw(RC522_TReloadRegL, 30U);
    rc522_write_raw(RC522_TReloadRegH, 0U);
    rc522_write_raw(RC522_TModeReg, 0x8DU);
    rc522_write_raw(RC522_TPrescalerReg, 0x3EU);
    hal_delay_us(2U);
    rc522_antenna_on();
}

static char rc522_com_mf522(uint8_t command, const uint8_t *in_data, uint8_t in_len,
                            uint8_t *out_data, uint32_t *out_len_bit)
{
    char status = (char)RC522_MI_ERR;
    uint8_t irq_en = 0x00U;
    uint8_t wait_for = 0x00U;
    uint8_t irq;
    uint8_t last_bits;
    uint8_t fifo_len;
    uint32_t timeout = 1000U;
    uint32_t i;

    if (command == RC522_PCD_AUTHENT) {
        irq_en = 0x12U;
        wait_for = 0x10U;
    } else if (command == RC522_PCD_TRANSCEIVE) {
        irq_en = 0x77U;
        wait_for = 0x30U;
    }

    rc522_write_raw(RC522_ComIEnReg, (uint8_t)(irq_en | 0x80U));
    rc522_clear_bit_mask(RC522_ComIrqReg, 0x80U);
    rc522_write_raw(RC522_CommandReg, RC522_PCD_IDLE);
    rc522_set_bit_mask(RC522_FIFOLevelReg, 0x80U);

    for (i = 0U; i < in_len; ++i) {
        rc522_write_raw(RC522_FIFODataReg, in_data[i]);
    }

    rc522_write_raw(RC522_CommandReg, command);
    if (command == RC522_PCD_TRANSCEIVE) {
        rc522_set_bit_mask(RC522_BitFramingReg, 0x80U);
    }

    do {
        irq = rc522_read_raw(RC522_ComIrqReg);
        --timeout;
    } while ((timeout != 0U) && ((irq & 0x01U) == 0U) && ((irq & wait_for) == 0U));

    rc522_clear_bit_mask(RC522_BitFramingReg, 0x80U);

    if (timeout != 0U) {
        if ((rc522_read_raw(RC522_ErrorReg) & 0x1BU) == 0U) {
            status = (char)RC522_MI_OK;
            if ((irq & irq_en & 0x01U) != 0U) {
                status = (char)RC522_MI_ERR;
            }
            if (command == RC522_PCD_TRANSCEIVE) {
                fifo_len = rc522_read_raw(RC522_FIFOLevelReg);
                last_bits = (uint8_t)(rc522_read_raw(RC522_ControlReg) & 0x07U);
                if (last_bits != 0U) {
                    *out_len_bit = (uint32_t)((fifo_len - 1U) * 8U + last_bits);
                } else {
                    *out_len_bit = (uint32_t)fifo_len * 8U;
                }
                if (fifo_len == 0U) {
                    fifo_len = 1U;
                }
                if (fifo_len > RC522_MAXRLEN) {
                    fifo_len = RC522_MAXRLEN;
                }
                for (i = 0U; i < fifo_len; ++i) {
                    out_data[i] = rc522_read_raw(RC522_FIFODataReg);
                }
            }
        }
    }

    rc522_set_bit_mask(RC522_ControlReg, 0x80U);
    rc522_write_raw(RC522_CommandReg, RC522_PCD_IDLE);
    return status;
}

static char rc522_request(uint8_t req_code, uint8_t *tag_type)
{
    char status;
    uint8_t buf[RC522_MAXRLEN];
    uint32_t len;

    rc522_clear_bit_mask(RC522_Status2Reg, 0x08U);
    rc522_write_raw(RC522_BitFramingReg, 0x07U);
    rc522_set_bit_mask(RC522_TxControlReg, 0x03U);
    buf[0] = req_code;
    status = rc522_com_mf522(RC522_PCD_TRANSCEIVE, buf, 1U, buf, &len);
    if ((status == (char)RC522_MI_OK) && (len == 0x10U)) {
        tag_type[0] = buf[0];
        tag_type[1] = buf[1];
    } else {
        status = (char)RC522_MI_ERR;
    }
    return status;
}

static char rc522_anticoll(uint8_t *snr)
{
    char status;
    uint8_t i;
    uint8_t check = 0U;
    uint8_t buf[RC522_MAXRLEN];
    uint32_t len;

    rc522_clear_bit_mask(RC522_Status2Reg, 0x08U);
    rc522_write_raw(RC522_BitFramingReg, 0x00U);
    rc522_clear_bit_mask(RC522_CollReg, 0x80U);
    buf[0] = 0x93U;
    buf[1] = 0x20U;
    status = rc522_com_mf522(RC522_PCD_TRANSCEIVE, buf, 2U, buf, &len);
    if (status == (char)RC522_MI_OK) {
        for (i = 0U; i < 4U; ++i) {
            snr[i] = buf[i];
            check ^= buf[i];
        }
        if (check != buf[i]) {
            status = (char)RC522_MI_ERR;
        }
    }
    rc522_set_bit_mask(RC522_CollReg, 0x80U);
    return status;
}

static void rc522_calculate_crc(const uint8_t *indata, uint8_t len, uint8_t *outdata)
{
    uint8_t n;

    rc522_clear_bit_mask(RC522_DivIrqReg, 0x04U);
    rc522_write_raw(RC522_CommandReg, RC522_PCD_IDLE);
    rc522_set_bit_mask(RC522_FIFOLevelReg, 0x80U);
    for (n = 0U; n < len; ++n) {
        rc522_write_raw(RC522_FIFODataReg, indata[n]);
    }
    rc522_write_raw(RC522_CommandReg, RC522_PCD_CALCCRC);
    n = 0xFFU;
    do {
        n = rc522_read_raw(RC522_DivIrqReg);
    } while ((n & 0x04U) == 0U);
    outdata[0] = rc522_read_raw(RC522_CRCResultRegL);
    outdata[1] = rc522_read_raw(RC522_CRCResultRegM);
}

static char rc522_select(uint8_t *snr)
{
    char status;
    uint8_t i;
    uint8_t buf[RC522_MAXRLEN];
    uint32_t len;

    buf[0] = RC522_PICC_ANTICOLL1;
    buf[1] = 0x70U;
    buf[6] = 0U;
    for (i = 0U; i < 4U; ++i) {
        buf[(uint8_t)(i + 2U)] = snr[i];
        buf[6] ^= snr[i];
    }
    rc522_calculate_crc(buf, 7U, &buf[7]);
    rc522_clear_bit_mask(RC522_Status2Reg, 0x08U);
    status = rc522_com_mf522(RC522_PCD_TRANSCEIVE, buf, 9U, buf, &len);
    if ((status != (char)RC522_MI_OK) || (len != 0x18U)) {
        status = (char)RC522_MI_ERR;
    }
    return status;
}

static void rc522_init(void)
{
    spi_hal_config_t cfg;

    rc522_card_present = 0U;
    memset(rc522_uid, 0, sizeof(rc522_uid));

    gpio_hal_config_pin(&board_rc522_cs_pin);
    gpio_hal_config_pin(&board_rc522_rst_pin);
    rc522_cs_disable();
    rc522_rst_disable();

    cfg.instance = BOARD_HW_SPI1_INSTANCE;
    cfg.sck = board_hw_spi1_sck;
    cfg.mosi = board_hw_spi1_mosi;
    cfg.miso = board_hw_spi1_miso;
    cfg.remap = BOARD_HW_SPI1_REMAP;
    cfg.prescaler = BOARD_HW_SPI1_PRESCALER;
    cfg.cpol = BOARD_HW_SPI1_CPOL;
    cfg.cpha = BOARD_HW_SPI1_CPHA;
    cfg.timeout_us = SPI_HAL_DEFAULT_TIMEOUT_US;
#if HAL_SPI_ENABLE_DMA
    cfg.tx_dma_channel = BOARD_HW_SPI1_TX_DMA;
    cfg.rx_dma_channel = BOARD_HW_SPI1_RX_DMA;
#endif
    (void)spi_hal_init(BOARD_RC522_SPI_BUS_ID, &cfg);

    rc522_reset();
    rc522_config_iso_type_a();
}

static unsigned char rc522_poll_card(void)
{
    uint8_t tag_type[2];
    char status;

    status = rc522_request(RC522_PICC_REQALL, tag_type);
    if (status != (char)RC522_MI_OK) {
        rc522_card_present = 0U;
        return 0U;
    }

    status = rc522_anticoll(rc522_uid);
    if (status != (char)RC522_MI_OK) {
        rc522_card_present = 0U;
        return 0U;
    }

    status = rc522_select(rc522_uid);
    if (status != (char)RC522_MI_OK) {
        rc522_card_present = 0U;
        return 0U;
    }

    rc522_card_present = 1U;
    return 1U;
}

static void rc522_get_uid(unsigned char *uid, unsigned char max_len, unsigned char *uid_len)
{
    unsigned char copy_len = 4U;

    if (uid_len != 0) {
        *uid_len = 0U;
    }

    if ((uid == 0) || (max_len == 0U) || (rc522_card_present == 0U)) {
        return;
    }

    if (copy_len > max_len) {
        copy_len = max_len;
    }

    memcpy(uid, rc522_uid, copy_len);
    if (uid_len != 0) {
        *uid_len = copy_len;
    }
}

static void rc522_clear_card(void)
{
    rc522_card_present = 0U;
    memset(rc522_uid, 0, sizeof(rc522_uid));
}

static const rfid_driver_t rc522_drv = {
    "rc522",
    rc522_init,
    rc522_poll_card,
    rc522_get_uid,
    rc522_clear_card
};

REGISTER_DRIVER(RFID, rc522_drv);

#endif
