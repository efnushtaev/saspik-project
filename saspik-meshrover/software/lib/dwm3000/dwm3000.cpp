#include "dwm3000.h"
#include "pins.h"
#include "config.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <string.h>

static const char *TAG = UWB_TAG;

#define _SPI_RD 0x0000u
#define _SPI_WR 0x8000u
#define _SPI_FAC   0x01u
#define _SPI_EAMRW 0x40u

#define _CMD_TXRXOFF  0x0
#define _CMD_TX       0x1
#define _CMD_RX       0x2
#define _CMD_CLR_IRQS 0x12

#define _SYS_STATUS_CP_LOCK  0x2
#define _SYS_STATUS_RCINIT   0x0100
#define _TX_FCTRL_TXFLEN_MASK  0x3FF
#define _TX_FCTRL_TR         0x800
#define _CHAN_CTRL_RX_PCODE      0x1F00u
#define _CHAN_CTRL_TX_PCODE      0xF8u
#define _CHAN_CTRL_SFD_TYPE      0x6u
#define _CHAN_CTRL_RF_CHAN       0x1u
#define _TX_FCTRL_TXBR           0x400u
#define _TX_FCTRL_TXPSR          0xF000u
#define _DTUNE0_PRE_PAC_SYM      0x3u
#define _DGC_CFG_THR_64          0x7E00u
#define _OTP_CFG_OPS_ID          0x1800u
#define _OTP_CFG_OPS_KICK        0x400u

#define _DWT_OPSET_SHORT  0x1000
#define _DWT_PLEN_128     0x05
#define _DWT_PAC8         0x0
#define _DWT_SFDTOC_DEF   129
#define _DWT_PD_THRESH_DEFAULT 0xAF5F584CUL
#define _DGC_CFG_THR_64_VAL    0x32
#define _DWT_DGC_CFG0          0x10000240UL
#define _DWT_DGC_CFG1          0x1b6da489UL
#define _RF_TXCTRL_CH5         0x1C071134UL
#define _RF_TXCTRL_LO_B2       0x0E
#define _RF_PLL_CFG_CH5        0x1F3C
#define _RF_PLL_CFG_LD         0x81
#define _LDO_RLOAD_VAL_B1      0x14
#define _XTAL_TRIM_DEFAULT     0x2E
#define _CLK_CTRL_AUTO         0x0200
#define _SEQ_CTRL_AINIT2IDLE   0x0100
#define _ERR_RX_CAL_FAIL       0x1FFFFFFF
#define _STS_CFG0_LEN64        0x07

static const uint32_t _ch5_dgc_lut[7] = {
    0x1C0FD, 0x1C43E, 0x1C6BE, 0x1C77E,
    0x1CF36, 0x1CFB5, 0x1CFF5
};

static void _delay_us(uint32_t us)
{
    esp_rom_delay_us(us);
}

static void _delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static void _reg_xfer(dwm3000_t *dev, uint32_t reg, uint16_t sub,
                      uint8_t *buffer, size_t len, bool write)
{
    uint8_t header[2];
    uint8_t cnt = 1;

    uint16_t reg_file   = 0x1F & ((reg + sub) >> 16);
    uint16_t reg_offset = 0x7F &  (reg + sub);

    uint16_t addr = (uint16_t)((reg_file << 9) | (reg_offset << 2));
    uint16_t mode = write ? _SPI_WR : _SPI_RD;

    header[0] = (uint8_t)((mode | addr) >> 8);
    header[1] = (uint8_t)(addr | (mode & 0x03));

    if (len == 0) {
        header[0] = (uint8_t)(0x80 | (reg << 1) | _SPI_FAC);
        cnt = 1;
    } else if (reg_offset == 0) {
        cnt = 1;
    } else {
        header[0] |= _SPI_EAMRW;
        cnt = 2;
    }

    uint8_t on_stack[16];
    uint8_t *buf = NULL;
    if (len <= sizeof(on_stack)) {
        buf = on_stack;
    } else {
        buf = (uint8_t *)malloc(cnt + len);
        if (!buf) return;
    }

    memcpy(buf, header, cnt);
    if (write) {
        if (len) memcpy(buf + cnt, buffer, len);
        spi_transaction_t t = {};
        t.length = (cnt + len) * 8;
        t.tx_buffer = buf;
        spi_device_transmit(dev->spi, &t);
    } else {
        memset(buf + cnt, 0, len);
        spi_transaction_t t = {};
        t.length = (cnt + len) * 8;
        t.tx_buffer = buf;
        t.rx_buffer = buf;
        spi_device_transmit(dev->spi, &t);
        memcpy(buffer, buf + cnt, len);
    }

    if (buf != on_stack) free(buf);
}

static void _fast_cmd(dwm3000_t *dev, uint8_t cmd)
{
    _reg_xfer(dev, cmd, 0, NULL, 0, true);
}

esp_err_t dwm3000_write_raw(dwm3000_t *dev, uint32_t reg, uint16_t sub,
                           const uint8_t *data, size_t len)
{
    if (len == 0) return ESP_ERR_INVALID_ARG;
    _reg_xfer(dev, reg, sub, (uint8_t *)data, len, true);
    return ESP_OK;
}

esp_err_t dwm3000_read_raw(dwm3000_t *dev, uint32_t reg, uint16_t sub,
                          uint8_t *data, size_t len)
{
    if (len == 0) return ESP_ERR_INVALID_ARG;
    _reg_xfer(dev, reg, sub, data, len, false);
    return ESP_OK;
}

esp_err_t dwm3000_write8(dwm3000_t *dev, uint32_t reg, uint16_t sub, uint8_t val)
{
    return dwm3000_write_raw(dev, reg, sub, &val, 1);
}

uint8_t dwm3000_read8(dwm3000_t *dev, uint32_t reg, uint16_t sub)
{
    uint8_t val = 0;
    dwm3000_read_raw(dev, reg, sub, &val, 1);
    return val;
}

esp_err_t dwm3000_write16(dwm3000_t *dev, uint32_t reg, uint16_t sub, uint16_t val)
{
    uint8_t buf[2] = { (uint8_t)(val & 0xFF), (uint8_t)(val >> 8) };
    return dwm3000_write_raw(dev, reg, sub, buf, 2);
}

uint16_t dwm3000_read16(dwm3000_t *dev, uint32_t reg, uint16_t sub)
{
    uint8_t buf[2] = {0, 0};
    dwm3000_read_raw(dev, reg, sub, buf, 2);
    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

esp_err_t dwm3000_write32(dwm3000_t *dev, uint32_t reg, uint16_t sub, uint32_t val)
{
    uint8_t buf[4] = {
        (uint8_t)(val & 0xFF),
        (uint8_t)((val >> 8) & 0xFF),
        (uint8_t)((val >> 16) & 0xFF),
        (uint8_t)((val >> 24) & 0xFF)
    };
    return dwm3000_write_raw(dev, reg, sub, buf, 4);
}

uint32_t dwm3000_read32(dwm3000_t *dev, uint32_t reg, uint16_t sub)
{
    uint8_t buf[4] = {0, 0, 0, 0};
    dwm3000_read_raw(dev, reg, sub, buf, 4);
    return (uint32_t)buf[0] | ((uint32_t)buf[1] << 8)
         | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
}

uint32_t dwm3000_read_dev_id(dwm3000_t *dev)
{
    return dwm3000_read32(dev, DWM3000_DEV_ID, 0x00);
}

uint64_t dwm3000_read_eui(dwm3000_t *dev)
{
    uint8_t buf[8] = {0};
    dwm3000_read_raw(dev, DWM3000_EU_ID, 0x00, buf, 8);
    uint64_t eui = 0;
    for (int i = 0; i < 8; i++) {
        eui |= (uint64_t)buf[i] << (i * 8);
    }
    return eui;
}

esp_err_t dwm3000_write_eui(dwm3000_t *dev, uint64_t eui)
{
    uint8_t buf[8];
    for (int i = 0; i < 8; i++) {
        buf[i] = (eui >> (i * 8)) & 0xFF;
    }
    return dwm3000_write_raw(dev, DWM3000_EU_ID, 0x00, buf, 8);
}

esp_err_t dwm3000_set_channel(dwm3000_t *dev, uint8_t channel)
{
    if (channel < 1 || channel > 9) {
        return ESP_ERR_INVALID_ARG;
    }
    dev->channel = channel;
    return ESP_OK;
}

esp_err_t dwm3000_set_data_rate(dwm3000_t *dev, uint8_t rate)
{
    if (rate > 1) {
        return ESP_ERR_INVALID_ARG;
    }
    dev->data_rate = rate;
    return ESP_OK;
}

esp_err_t dwm3000_set_prf(dwm3000_t *dev, uint8_t prf)
{
    if (prf != DWM3000_PRF_16M && prf != DWM3000_PRF_64M) {
        return ESP_ERR_INVALID_ARG;
    }
    dev->prf = prf;
    return ESP_OK;
}

esp_err_t dwm3000_set_tx_power(dwm3000_t *dev, uint32_t power)
{
    return dwm3000_write32(dev, DWM3000_TX_POWER, 0x00, power);
}

static bool _wait_idle_rc(dwm3000_t *dev)
{
    for (int i = 0; i < 100; i++) {
        uint16_t st = dwm3000_read16(dev, DWM3000_SYS_STATUS, 0x02);
        if (st & _SYS_STATUS_RCINIT) return true;
        _delay_ms(1);
    }
    return false;
}

static bool _pgf_cal(dwm3000_t *dev)
{
    uint16_t ldo = dwm3000_read16(dev, DWM3000_LDO_CTRL, 0x00);
    dwm3000_write16(dev, DWM3000_LDO_CTRL, 0x00,
                    ldo | 0x100 | 0x004 | 0x001);

    dwm3000_write32(dev, DWM3000_RX_CAL_CFG, 0x00, 0x20001);
    dwm3000_write8(dev, DWM3000_RX_CAL_CFG, 0x00,
                   dwm3000_read8(dev, DWM3000_RX_CAL_CFG, 0x00) | 0x10);

    for (int i = 0; i < 3; i++) {
        _delay_us(20);
        if (dwm3000_read8(dev, DWM3000_RX_CAL_STS, 0x00) == 1) break;
    }

    dwm3000_write8(dev, DWM3000_RX_CAL_CFG, 0x00, 0x00);
    dwm3000_write8(dev, DWM3000_RX_CAL_STS, 0x00, 0x01);
    dwm3000_write8(dev, DWM3000_RX_CAL_CFG, 0x02, 0x01);
    dwm3000_write16(dev, DWM3000_LDO_CTRL, 0x00, ldo);

    uint32_t resi = dwm3000_read32(dev, DWM3000_RX_CAL_RESI, 0x00);
    uint32_t resq = dwm3000_read32(dev, DWM3000_RX_CAL_RESQ, 0x00);
    if (resi == _ERR_RX_CAL_FAIL || resq == _ERR_RX_CAL_FAIL) {
        ESP_LOGW(TAG, "PGF cal failed resi=0x%08lX resq=0x%08lX",
                 (unsigned long)resi, (unsigned long)resq);
        return false;
    }
    return true;
}

esp_err_t dwm3000_configure(dwm3000_t *dev)
{
    if (dev->channel != 5) {
        ESP_LOGE(TAG, "channel %u not supported by this port", dev->channel);
        return ESP_ERR_NOT_SUPPORTED;
    }

    uint32_t otp = dwm3000_read32(dev, DWM3000_OTP_CFG, 0x00);
    otp = (otp & ~((uint32_t)_OTP_CFG_OPS_ID)) | (_DWT_OPSET_SHORT | _OTP_CFG_OPS_KICK);
    dwm3000_write32(dev, DWM3000_OTP_CFG, 0x00, otp);

    uint32_t temp = dwm3000_read32(dev, DWM3000_CHAN_CTRL, 0x00);
    temp &= ~((uint32_t)(_CHAN_CTRL_RX_PCODE | _CHAN_CTRL_TX_PCODE
                         | _CHAN_CTRL_SFD_TYPE | _CHAN_CTRL_RF_CHAN));
    temp |= (9u << 8) | (9u << 3) | (1u << 1);
    dwm3000_write32(dev, DWM3000_CHAN_CTRL, 0x00, temp);

    temp = dwm3000_read32(dev, DWM3000_TX_FCTRL, 0x00);
    temp &= ~((uint32_t)(_TX_FCTRL_TXBR | _TX_FCTRL_TXPSR));
    temp |= ((uint32_t)dev->data_rate << 10) | ((uint32_t)_DWT_PLEN_128 << 12);
    dwm3000_write32(dev, DWM3000_TX_FCTRL, 0x00, temp);

    uint8_t dtune0 = dwm3000_read8(dev, DWM3000_DTUNE0, 0x00);
    dtune0 = (dtune0 & ~_DTUNE0_PRE_PAC_SYM) | _DWT_PAC8;
    dwm3000_write8(dev, DWM3000_DTUNE0, 0x00, dtune0);
    dwm3000_write16(dev, DWM3000_DTUNE0, 0x02, _DWT_SFDTOC_DEF);

    dwm3000_write8(dev, DWM3000_STS_CFG0, 0x00, _STS_CFG0_LEN64);
    dwm3000_write32(dev, DWM3000_DTUNE3, 0x00, _DWT_PD_THRESH_DEFAULT);

    dwm3000_write32(dev, DWM3000_TX_CTRL_HI, 0x00, _RF_TXCTRL_CH5);
    dwm3000_write16(dev, DWM3000_PLL_CFG, 0x00, _RF_PLL_CFG_CH5);
    dwm3000_write8(dev, DWM3000_LDO_RLOAD, 0x01, _LDO_RLOAD_VAL_B1);
    dwm3000_write8(dev, DWM3000_TX_CTRL_LO, 0x02, _RF_TXCTRL_LO_B2);
    dwm3000_write8(dev, DWM3000_PLL_CAL, 0x00, _RF_PLL_CFG_LD);

    dwm3000_write32(dev, DWM3000_SYS_STATUS, 0x00, _SYS_STATUS_CP_LOCK);
    dwm3000_write16(dev, DWM3000_CLK_CTRL, 0x00, _CLK_CTRL_AUTO);
    dwm3000_write8(dev, DWM3000_SEQ_CTRL, 0x01,
                   dwm3000_read8(dev, DWM3000_SEQ_CTRL, 0x01)
                   | (_SEQ_CTRL_AINIT2IDLE >> 8));

    bool locked = false;
    for (int i = 0; i < 6; i++) {
        _delay_us(20);
        if (dwm3000_read8(dev, DWM3000_SYS_STATUS, 0x00) & _SYS_STATUS_CP_LOCK) {
            locked = true;
            break;
        }
    }
    if (!locked) {
        ESP_LOGE(TAG, "PLL lock failed");
        return ESP_ERR_TIMEOUT;
    }
    ESP_LOGI(TAG, "PLL locked");

    for (int i = 0; i < 7; i++) {
        dwm3000_write32(dev, DWM3000_DGC_LUT0 + i * 4, 0x00, _ch5_dgc_lut[i]);
    }
    dwm3000_write32(dev, DWM3000_DGC_CFG0, 0x00, _DWT_DGC_CFG0);
    dwm3000_write32(dev, DWM3000_DGC_CFG1, 0x00, _DWT_DGC_CFG1);

    temp = dwm3000_read16(dev, DWM3000_DGC_CFG, 0x00);
    temp &= ~_DGC_CFG_THR_64;
    temp |= _DGC_CFG_THR_64_VAL << 9;
    dwm3000_write16(dev, DWM3000_DGC_CFG, 0x00, temp);

    _pgf_cal(dev);

    dwm3000_write16(dev, DWM3000_TX_ANTD, 0x00, UWB_ANTENNA_DELAY);
    dwm3000_write16(dev, DWM3000_CIA_CONF, 0x00, UWB_ANTENNA_DELAY);
    dwm3000_write32(dev, DWM3000_TX_POWER, 0x00, 0x1F3F7F7F);

    dwm3000_clear_irq(dev, 0xFFFFFFFF);
    return ESP_OK;
}

esp_err_t dwm3000_init(dwm3000_t *dev)
{
    dev->channel = 5;
    dev->prf = DWM3000_PRF_64M;
    dev->data_rate = DWM3000_BR_850K;

    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num = PIN_SPI_MOSI;
    bus_cfg.miso_io_num = PIN_SPI_MISO;
    bus_cfg.sclk_io_num = PIN_SPI_SCLK;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = 4096;

    spi_device_interface_config_t dev_cfg = {};
    dev_cfg.mode = 0;
    dev_cfg.clock_speed_hz = UWB_SPI_CLOCK_HZ;
    dev_cfg.spics_io_num = PIN_SPI_CS;
    dev_cfg.queue_size = 3;

    esp_err_t err = spi_bus_initialize(UWB_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(err));
        return err;
    }

    err = spi_bus_add_device(UWB_SPI_HOST, &dev_cfg, &dev->spi);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(err));
        return err;
    }

    gpio_set_direction(PIN_DW_IRQ, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_DW_IRQ, GPIO_PULLUP_ONLY);
    gpio_set_direction(PIN_DW_RST, GPIO_MODE_OUTPUT);

    err = dwm3000_reset(dev);
    if (err != ESP_OK) {
        return err;
    }

    uint32_t dev_id = dwm3000_read_dev_id(dev);
    ESP_LOGI(TAG, "DWM3000 DEV_ID: 0x%08lX", dev_id);
    if (dev_id != DWM3000_EXPECTED_DEV_ID && dev_id != 0xDECA0312) {
        ESP_LOGE(TAG, "unexpected device ID: 0x%08lX", dev_id);
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "DWM3000 detected OK");

    dwm3000_write8(dev, DWM3000_XTAL, 0x00, _XTAL_TRIM_DEFAULT);

    return ESP_OK;
}

esp_err_t dwm3000_reset(dwm3000_t *dev)
{
    gpio_set_level(PIN_DW_RST, 0);
    _delay_us(10);
    gpio_set_level(PIN_DW_RST, 1);
    _delay_ms(5);

    if (!_wait_idle_rc(dev)) {
        ESP_LOGE(TAG, "IDLE_RC timeout after reset");
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

esp_err_t dwm3000_send_frame(dwm3000_t *dev, const uint8_t *data, size_t len)
{
    if (len < 1 || len > 125) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buf[128];
    memcpy(buf, data, len);
    memset(buf + len, 0, 2);

    dwm3000_write_raw(dev, DWM3000_TX_BUFFER, 0x00, buf, len + 2);

    uint32_t reg32 = dwm3000_read32(dev, DWM3000_TX_FCTRL, 0x00);
    reg32 &= ~((uint32_t)(_TX_FCTRL_TXFLEN_MASK | _TX_FCTRL_TR));
    reg32 |= (len + 2) | (1u << 11);
    dwm3000_write32(dev, DWM3000_TX_FCTRL, 0x00, reg32);

    dwm3000_clear_irq(dev, DWM3000_IRQ_CMD_TX_DONE);
    _fast_cmd(dev, _CMD_TX);

    return ESP_OK;
}

void dwm3000_set_rx_enable(dwm3000_t *dev, bool en)
{
    if (en) {
        _fast_cmd(dev, _CMD_RX);
    } else {
        _fast_cmd(dev, _CMD_TXRXOFF);
    }
}

void dwm3000_set_wait4resp(dwm3000_t *dev, bool en)
{
    (void)dev;
    (void)en;
}

esp_err_t dwm3000_recv_frame(dwm3000_t *dev, uint8_t *data, size_t *len,
                            uint32_t timeout_ms)
{
    uint32_t frame_len = dwm3000_get_frame_length(dev);
    if (frame_len == 0 || frame_len > *len) {
        *len = 0;
        return ESP_ERR_INVALID_SIZE;
    }

    *len = frame_len;
    dwm3000_read_raw(dev, DWM3000_RX_BUFFER, 0x00, data, frame_len);
    return ESP_OK;
}

uint32_t dwm3000_get_frame_length(dwm3000_t *dev)
{
    return dwm3000_read32(dev, DWM3000_RX_FINFO, 0x00) & 0x03FF;
}

uint64_t dwm3000_read_tx_timestamp(dwm3000_t *dev)
{
    uint8_t buf[5] = {0};
    dwm3000_read_raw(dev, DWM3000_TX_TIME, 0x00, buf, 5);
    uint64_t ts = 0;
    for (int i = 0; i < 5; i++) {
        ts |= (uint64_t)buf[i] << (i * 8);
    }
    return ts;
}

uint64_t dwm3000_read_rx_timestamp(dwm3000_t *dev)
{
    uint8_t buf[5] = {0};
    dwm3000_read_raw(dev, DWM3000_RX_TIME, 0x00, buf, 5);
    uint64_t ts = 0;
    for (int i = 0; i < 5; i++) {
        ts |= (uint64_t)buf[i] << (i * 8);
    }
    return ts;
}

uint64_t dwm3000_read_sys_time(dwm3000_t *dev)
{
    uint8_t buf[5] = {0};
    dwm3000_read_raw(dev, DWM3000_SYS_TIME, 0x00, buf, 5);
    uint64_t ts = 0;
    for (int i = 0; i < 5; i++) {
        ts |= (uint64_t)buf[i] << (i * 8);
    }
    return ts;
}

void dwm3000_clear_irq(dwm3000_t *dev, uint32_t bits)
{
    if (bits == 0xFFFFFFFF) {
        _fast_cmd(dev, _CMD_CLR_IRQS);
        return;
    }
    dwm3000_write32(dev, DWM3000_SYS_STATUS, 0x00, bits);
}

uint32_t dwm3000_read_irq(dwm3000_t *dev)
{
    return dwm3000_read32(dev, DWM3000_SYS_STATUS, 0x00);
}

void dwm3000_enable_irq(dwm3000_t *dev, uint32_t bits)
{
    dwm3000_write32(dev, DWM3000_SYS_ENABLE, 0x00, bits);
}