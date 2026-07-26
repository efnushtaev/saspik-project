#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <driver/spi_master.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DW1000_DEV_ID       0x00
#define DW1000_EU_ID        0x01
#define DW1000_PAN_ADDR     0x03
#define DW1000_SYS_CFG      0x04
#define DW1000_SYS_TIME     0x06
#define DW1000_TX_FCTRL     0x08
#define DW1000_TX_BUFFER    0x0C
#define DW1000_RX_BUFFER    0x09
#define DW1000_SYS_CTRL     0x0D
#define DW1000_SYS_MASK     0x0E
#define DW1000_SYS_STATUS   0x0F
#define DW1000_RX_FINFO     0x10
#define DW1000_RX_TIME      0x15
#define DW1000_TX_TIME      0x17
#define DW1000_CHAN_CTRL    0x1C
#define DW1000_PLL_CFG      0x1E
#define DW1000_PLL_LOCK     0x1F
#define DW1000_AGC_CTRL     0x23
#define DW1000_EXT_SYNC     0x25
#define DW1000_GPIO_CTRL    0x26
#define DW1000_DRX_CONF0    0x28
#define DW1000_DRX_CONF1    0x29
#define DW1000_DRX_CONF2    0x2A
#define DW1000_LDE_CFG1     0x2E
#define DW1000_LDE_CFG2     0x2F
#define DW1000_LDE_REPC     0x30
#define DW1000_PACF         0x34
#define DW1000_TX_POWER     0x1E

#define DW1000_EXPECTED_DEV_ID 0xDECA0130

#define DW1000_IRQ_CMD_RX_DATA     (1 << 10)
#define DW1000_IRQ_CMD_TX_DONE     (1 << 3)

typedef struct {
    spi_device_handle_t spi;
} dw1000_t;

esp_err_t dw1000_init(dw1000_t *dev);
esp_err_t dw1000_reset(dw1000_t *dev);

esp_err_t dw1000_write_raw(dw1000_t *dev, uint8_t reg, uint16_t sub,
                           const uint8_t *data, size_t len);
esp_err_t dw1000_read_raw(dw1000_t *dev, uint8_t reg, uint16_t sub,
                          uint8_t *data, size_t len);

esp_err_t dw1000_write8(dw1000_t *dev, uint8_t reg, uint16_t sub, uint8_t val);
uint8_t   dw1000_read8(dw1000_t *dev, uint8_t reg, uint16_t sub);
esp_err_t dw1000_write16(dw1000_t *dev, uint8_t reg, uint16_t sub, uint16_t val);
uint16_t  dw1000_read16(dw1000_t *dev, uint8_t reg, uint16_t sub);
esp_err_t dw1000_write32(dw1000_t *dev, uint8_t reg, uint16_t sub, uint32_t val);
uint32_t  dw1000_read32(dw1000_t *dev, uint8_t reg, uint16_t sub);

uint32_t  dw1000_read_dev_id(dw1000_t *dev);
uint64_t  dw1000_read_eui(dw1000_t *dev);
esp_err_t dw1000_write_eui(dw1000_t *dev, uint64_t eui);

esp_err_t dw1000_set_channel(dw1000_t *dev, uint8_t channel);
esp_err_t dw1000_set_data_rate(dw1000_t *dev, uint8_t rate);
esp_err_t dw1000_set_prf(dw1000_t *dev, uint8_t prf);
esp_err_t dw1000_set_tx_power(dw1000_t *dev, uint32_t power);

esp_err_t dw1000_send_frame(dw1000_t *dev, const uint8_t *data, size_t len);
esp_err_t dw1000_recv_frame(dw1000_t *dev, uint8_t *data, size_t *len,
                            uint32_t timeout_ms);
void      dw1000_set_rx_enable(dw1000_t *dev, bool en);

uint64_t  dw1000_read_tx_timestamp(dw1000_t *dev);
uint64_t  dw1000_read_rx_timestamp(dw1000_t *dev);
uint64_t  dw1000_read_sys_time(dw1000_t *dev);

void dw1000_set_wait4resp(dw1000_t *dev, bool en);

uint32_t dw1000_get_frame_length(dw1000_t *dev);

void dw1000_clear_irq(dw1000_t *dev, uint32_t bits);
uint32_t dw1000_read_irq(dw1000_t *dev);

void dw1000_enable_irq(dw1000_t *dev, uint32_t bits);

#ifdef __cplusplus
}
#endif
