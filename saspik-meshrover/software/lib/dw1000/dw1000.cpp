#include "dw1000.h"
#include "pins.h"
#include "config.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <string.h>

static const char *TAG = UWB_TAG;

static void _delay_us(uint32_t us)
{
    esp_rom_delay_us(us);
}

esp_err_t dw1000_init(dw1000_t *dev)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_SPI_MOSI,
        .miso_io_num = PIN_SPI_MISO,
        .sclk_io_num = PIN_SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    spi_device_interface_config_t dev_cfg = {
        .mode = 0,
        .clock_speed_hz = UWB_SPI_CLOCK_HZ,
        .spics_io_num = PIN_SPI_CS,
        .queue_size = 3,
    };

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

    err = dw1000_reset(dev);
    if (err != ESP_OK) {
        return err;
    }

    uint32_t dev_id = dw1000_read_dev_id(dev);
    ESP_LOGI(TAG, "DW1000 DEV_ID: 0x%08lX", dev_id);
    if (dev_id != DW1000_EXPECTED_DEV_ID) {
        ESP_LOGE(TAG, "unexpected device ID: 0x%08lX", dev_id);
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "DW1000 detected OK");

    return ESP_OK;
}

esp_err_t dw1000_reset(dw1000_t *dev)
{
    gpio_set_level(PIN_DW_RST, 0);
    _delay_us(10);
    gpio_set_level(PIN_DW_RST, 1);
    _delay_us(5000);

    dw1000_write8(dev, DW1000_SYS_CFG, 0x00, 0x00);
    dw1000_set_rx_enable(dev, false);

    uint32_t status = dw1000_read32(dev, DW1000_SYS_STATUS, 0x00);
    dw1000_write32(dev, DW1000_SYS_STATUS, 0x00, status);

    return ESP_OK;
}

static void _spi_tx(dw1000_t *dev, const uint8_t *data, size_t len)
{
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_transmit(dev->spi, &t);
}

static void _spi_rx(dw1000_t *dev, const uint8_t *tx, uint8_t *rx, size_t len)
{
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    spi_device_transmit(dev->spi, &t);
}

static void _auto_sub(dw1000_t *dev, uint8_t reg, uint16_t sub,
                      const uint8_t *data, uint8_t *out, size_t len, bool write)
{
    if (len == 0) return;

    uint8_t hdr = (reg << 1) | (write ? 1 : 0);
    uint8_t sub_hdr[3];
    int sub_len = 0;

    if (sub < 0x80) {
        sub_hdr[0] = sub & 0x7F;
        sub_len = 1;
    } else if (sub < 0x10000) {
        sub_hdr[0] = (sub >> 8) & 0x7F;
        sub_hdr[1] = sub & 0xFF;
        sub_len = 2;
    } else {
        sub_hdr[0] = 0x80 | ((sub >> 16) & 0x0F);
        sub_hdr[1] = (sub >> 8) & 0xFF;
        sub_hdr[2] = sub & 0xFF;
        sub_len = 3;
    }

    size_t total = 1 + sub_len + len;
    uint8_t *buf = (uint8_t *)malloc(total);
    if (!buf) return;

    buf[0] = hdr;
    memcpy(buf + 1, sub_hdr, sub_len);

    if (write) {
        memcpy(buf + 1 + sub_len, data, len);
        _spi_tx(dev, buf, total);
    } else {
        memset(buf + 1 + sub_len, 0, len);
        _spi_rx(dev, buf, buf, total);
        memcpy(out, buf + 1 + sub_len, len);
    }

    free(buf);
}

esp_err_t dw1000_write_raw(dw1000_t *dev, uint8_t reg, uint16_t sub,
                           const uint8_t *data, size_t len)
{
    _auto_sub(dev, reg, sub, data, NULL, len, true);
    return ESP_OK;
}

esp_err_t dw1000_read_raw(dw1000_t *dev, uint8_t reg, uint16_t sub,
                          uint8_t *data, size_t len)
{
    _auto_sub(dev, reg, sub, NULL, data, len, false);
    return ESP_OK;
}

esp_err_t dw1000_write8(dw1000_t *dev, uint8_t reg, uint16_t sub, uint8_t val)
{
    return dw1000_write_raw(dev, reg, sub, &val, 1);
}

uint8_t dw1000_read8(dw1000_t *dev, uint8_t reg, uint16_t sub)
{
    uint8_t val = 0;
    dw1000_read_raw(dev, reg, sub, &val, 1);
    return val;
}

esp_err_t dw1000_write16(dw1000_t *dev, uint8_t reg, uint16_t sub, uint16_t val)
{
    uint8_t buf[2] = { (uint8_t)(val & 0xFF), (uint8_t)(val >> 8) };
    return dw1000_write_raw(dev, reg, sub, buf, 2);
}

uint16_t dw1000_read16(dw1000_t *dev, uint8_t reg, uint16_t sub)
{
    uint8_t buf[2] = {0, 0};
    dw1000_read_raw(dev, reg, sub, buf, 2);
    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

esp_err_t dw1000_write32(dw1000_t *dev, uint8_t reg, uint16_t sub, uint32_t val)
{
    uint8_t buf[4] = {
        (uint8_t)(val & 0xFF),
        (uint8_t)((val >> 8) & 0xFF),
        (uint8_t)((val >> 16) & 0xFF),
        (uint8_t)((val >> 24) & 0xFF)
    };
    return dw1000_write_raw(dev, reg, sub, buf, 4);
}

uint32_t dw1000_read32(dw1000_t *dev, uint8_t reg, uint16_t sub)
{
    uint8_t buf[4] = {0, 0, 0, 0};
    dw1000_read_raw(dev, reg, sub, buf, 4);
    return (uint32_t)buf[0] | ((uint32_t)buf[1] << 8)
         | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
}

uint32_t dw1000_read_dev_id(dw1000_t *dev)
{
    return dw1000_read32(dev, DW1000_DEV_ID, 0x00);
}

uint64_t dw1000_read_eui(dw1000_t *dev)
{
    uint8_t buf[8] = {0};
    dw1000_read_raw(dev, DW1000_EU_ID, 0x00, buf, 8);
    uint64_t eui = 0;
    for (int i = 0; i < 8; i++) {
        eui |= (uint64_t)buf[i] << (i * 8);
    }
    return eui;
}

esp_err_t dw1000_write_eui(dw1000_t *dev, uint64_t eui)
{
    uint8_t buf[8];
    for (int i = 0; i < 8; i++) {
        buf[i] = (eui >> (i * 8)) & 0xFF;
    }
    return dw1000_write_raw(dev, DW1000_EU_ID, 0x00, buf, 8);
}

esp_err_t dw1000_set_channel(dw1000_t *dev, uint8_t channel)
{
    if (channel < 1 || channel > 7) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t ch = channel;
    uint8_t band = (channel <= 4) ? 0 : 1;
    dw1000_write8(dev, DW1000_CHAN_CTRL, 0x00, (band << 3) | (ch & 0x07));
    return ESP_OK;
}

esp_err_t dw1000_set_data_rate(dw1000_t *dev, uint8_t rate)
{
    uint8_t drx = dw1000_read8(dev, DW1000_DRX_CONF1, 0x00);
    drx = (drx & ~0xC0) | ((rate & 0x03) << 6);
    dw1000_write8(dev, DW1000_DRX_CONF1, 0x00, drx);
    return ESP_OK;
}

esp_err_t dw1000_set_prf(dw1000_t *dev, uint8_t prf)
{
    uint8_t chan = dw1000_read8(dev, DW1000_CHAN_CTRL, 0x00);
    chan = (chan & ~0x20) | ((prf & 0x01) << 5);
    dw1000_write8(dev, DW1000_CHAN_CTRL, 0x00, chan);
    return ESP_OK;
}

esp_err_t dw1000_set_tx_power(dw1000_t *dev, uint32_t power)
{
    return dw1000_write32(dev, 0x1E, 0x00, power);
}

esp_err_t dw1000_send_frame(dw1000_t *dev, const uint8_t *data, size_t len)
{
    if (len < 1 || len > 1023) {
        return ESP_ERR_INVALID_ARG;
    }

    dw1000_write_raw(dev, DW1000_TX_BUFFER, 0x00, data, len);

    dw1000_write16(dev, DW1000_TX_FCTRL, 0x00, (uint16_t)len & 0x03FF);

    dw1000_clear_irq(dev, DW1000_IRQ_CMD_TX_DONE);
    dw1000_write8(dev, DW1000_SYS_CTRL, 0x00, 0x02);

    return ESP_OK;
}

void dw1000_set_rx_enable(dw1000_t *dev, bool en)
{
    if (en) {
        dw1000_write8(dev, DW1000_SYS_CTRL, 0x00, 0x01);
    } else {
        dw1000_write8(dev, DW1000_SYS_CTRL, 0x00, 0x00);
    }
}

void dw1000_set_wait4resp(dw1000_t *dev, bool en)
{
    if (en) {
        dw1000_write8(dev, DW1000_SYS_CTRL, 0x00, 0x09);
    }
}

esp_err_t dw1000_recv_frame(dw1000_t *dev, uint8_t *data, size_t *len,
                            uint32_t timeout_ms)
{
    uint32_t frame_len = dw1000_get_frame_length(dev);
    if (frame_len == 0 || frame_len > *len) {
        *len = 0;
        return ESP_ERR_INVALID_SIZE;
    }

    *len = frame_len;
    dw1000_read_raw(dev, DW1000_RX_BUFFER, 0x00, data, frame_len);
    return ESP_OK;
}

uint32_t dw1000_get_frame_length(dw1000_t *dev)
{
    return dw1000_read32(dev, DW1000_RX_FINFO, 0x00) & 0x03FF;
}

uint64_t dw1000_read_tx_timestamp(dw1000_t *dev)
{
    uint8_t buf[5] = {0};
    dw1000_read_raw(dev, DW1000_TX_TIME, 0x00, buf, 5);
    uint64_t ts = 0;
    for (int i = 0; i < 5; i++) {
        ts |= (uint64_t)buf[i] << (i * 8);
    }
    return ts;
}

uint64_t dw1000_read_rx_timestamp(dw1000_t *dev)
{
    uint8_t buf[5] = {0};
    dw1000_read_raw(dev, DW1000_RX_TIME, 0x00, buf, 5);
    uint64_t ts = 0;
    for (int i = 0; i < 5; i++) {
        ts |= (uint64_t)buf[i] << (i * 8);
    }
    return ts;
}

uint64_t dw1000_read_sys_time(dw1000_t *dev)
{
    uint8_t buf[5] = {0};
    dw1000_read_raw(dev, DW1000_SYS_TIME, 0x00, buf, 5);
    uint64_t ts = 0;
    for (int i = 0; i < 5; i++) {
        ts |= (uint64_t)buf[i] << (i * 8);
    }
    return ts;
}

void dw1000_clear_irq(dw1000_t *dev, uint32_t bits)
{
    dw1000_write32(dev, DW1000_SYS_STATUS, 0x00, bits);
}

uint32_t dw1000_read_irq(dw1000_t *dev)
{
    return dw1000_read32(dev, DW1000_SYS_STATUS, 0x00);
}

void dw1000_enable_irq(dw1000_t *dev, uint32_t bits)
{
    dw1000_write32(dev, DW1000_SYS_MASK, 0x00, bits);
}
