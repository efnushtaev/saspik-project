#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <driver/spi_master.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DWM3000_DEV_ID       0x000000
#define DWM3000_EU_ID        0x000004
#define DWM3000_PAN_ADDR     0x00000C
#define DWM3000_SYS_CFG      0x000010
#define DWM3000_SYS_TIME     0x00001C
#define DWM3000_TX_FCTRL     0x000024
#define DWM3000_DX_TIME      0x00002C
#define DWM3000_RX_FWTO      0x000034
#define DWM3000_SYS_ENABLE   0x00003C
#define DWM3000_SYS_STATUS   0x000044
#define DWM3000_RX_FINFO     0x00004C
#define DWM3000_RX_TIME      0x000064
#define DWM3000_RX_TIME_RAW  0x000070
#define DWM3000_TX_TIME      0x000074
#define DWM3000_TX_ANTD      0x010004
#define DWM3000_TX_POWER     0x01000C
#define DWM3000_CHAN_CTRL    0x010014
#define DWM3000_TX_BUFFER    0x140000
#define DWM3000_RX_BUFFER    0x120000

#define DWM3000_STS_CFG0     0x020000
#define DWM3000_DGC_CFG      0x030018
#define DWM3000_DGC_CFG0     0x03001C
#define DWM3000_DGC_CFG1     0x030020
#define DWM3000_DGC_LUT0     0x030038
#define DWM3000_DGC_LUT1     0x03003C
#define DWM3000_DGC_LUT2     0x030040
#define DWM3000_DGC_LUT3     0x030044
#define DWM3000_DGC_LUT4     0x030048
#define DWM3000_DGC_LUT5     0x03004C
#define DWM3000_DGC_LUT6     0x030050
#define DWM3000_OTP_CFG      0x0B0008
#define DWM3000_DTUNE0       0x060000
#define DWM3000_DTUNE3       0x06000C
#define DWM3000_RX_CAL_CFG   0x04000C
#define DWM3000_RX_CAL_RESI  0x040014
#define DWM3000_RX_CAL_RESQ  0x04001C
#define DWM3000_RX_CAL_STS   0x040020
#define DWM3000_TX_CTRL_LO   0x070018
#define DWM3000_TX_CTRL_HI   0x07001C
#define DWM3000_LDO_CTRL     0x070048
#define DWM3000_LDO_RLOAD    0x070050
#define DWM3000_PLL_CFG      0x090000
#define DWM3000_PLL_CAL      0x090008
#define DWM3000_XTAL         0x090014
#define DWM3000_CIA_CONF     0x0E0000
#define DWM3000_CLK_CTRL     0x110004
#define DWM3000_SEQ_CTRL     0x110008

#define DWM3000_EXPECTED_DEV_ID 0xDECA0311

#define DWM3000_IRQ_CMD_RX_DATA     (1 << 14)
#define DWM3000_IRQ_CMD_TX_DONE     (1 << 7)

#define DWM3000_PRF_16M   1
#define DWM3000_PRF_64M   2
#define DWM3000_BR_850K   0
#define DWM3000_BR_6M8    1

typedef struct {
    spi_device_handle_t spi;
    uint8_t channel;
    uint8_t prf;
    uint8_t data_rate;
} dwm3000_t;

esp_err_t dwm3000_init(dwm3000_t *dev);
esp_err_t dwm3000_reset(dwm3000_t *dev);
esp_err_t dwm3000_configure(dwm3000_t *dev);

esp_err_t dwm3000_write_raw(dwm3000_t *dev, uint32_t reg, uint16_t sub,
                           const uint8_t *data, size_t len);
esp_err_t dwm3000_read_raw(dwm3000_t *dev, uint32_t reg, uint16_t sub,
                          uint8_t *data, size_t len);

esp_err_t dwm3000_write8(dwm3000_t *dev, uint32_t reg, uint16_t sub, uint8_t val);
uint8_t   dwm3000_read8(dwm3000_t *dev, uint32_t reg, uint16_t sub);
esp_err_t dwm3000_write16(dwm3000_t *dev, uint32_t reg, uint16_t sub, uint16_t val);
uint16_t  dwm3000_read16(dwm3000_t *dev, uint32_t reg, uint16_t sub);
esp_err_t dwm3000_write32(dwm3000_t *dev, uint32_t reg, uint16_t sub, uint32_t val);
uint32_t  dwm3000_read32(dwm3000_t *dev, uint32_t reg, uint16_t sub);

uint32_t  dwm3000_read_dev_id(dwm3000_t *dev);
uint64_t  dwm3000_read_eui(dwm3000_t *dev);
esp_err_t dwm3000_write_eui(dwm3000_t *dev, uint64_t eui);

esp_err_t dwm3000_set_channel(dwm3000_t *dev, uint8_t channel);
esp_err_t dwm3000_set_data_rate(dwm3000_t *dev, uint8_t rate);
esp_err_t dwm3000_set_prf(dwm3000_t *dev, uint8_t prf);
esp_err_t dwm3000_set_tx_power(dwm3000_t *dev, uint32_t power);

esp_err_t dwm3000_send_frame(dwm3000_t *dev, const uint8_t *data, size_t len);
esp_err_t dwm3000_recv_frame(dwm3000_t *dev, uint8_t *data, size_t *len,
                            uint32_t timeout_ms);
void      dwm3000_set_rx_enable(dwm3000_t *dev, bool en);

uint64_t  dwm3000_read_tx_timestamp(dwm3000_t *dev);
uint64_t  dwm3000_read_rx_timestamp(dwm3000_t *dev);
uint64_t  dwm3000_read_sys_time(dwm3000_t *dev);

void dwm3000_set_wait4resp(dwm3000_t *dev, bool en);

uint32_t dwm3000_get_frame_length(dwm3000_t *dev);

void dwm3000_clear_irq(dwm3000_t *dev, uint32_t bits);
uint32_t dwm3000_read_irq(dwm3000_t *dev);

void dwm3000_enable_irq(dwm3000_t *dev, uint32_t bits);

#ifdef __cplusplus
}
#endif