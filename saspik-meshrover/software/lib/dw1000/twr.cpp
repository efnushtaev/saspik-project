#include "twr.h"
#include "config.h"

#include <esp_log.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "twr";

void twr_init(twr_t *twr)
{
    memset(twr, 0, sizeof(*twr));
}

static void _make_poll(uint8_t *frame, uint8_t seq)
{
    frame[0] = TWR_FRAME_TYPE_POLL;
    frame[1] = seq;
}

static void _make_resp(uint8_t *frame, uint8_t seq)
{
    frame[0] = TWR_FRAME_TYPE_RESP;
    frame[1] = seq;
}

static void _make_final(uint8_t *frame, uint8_t seq,
                        uint64_t t1, uint64_t t4)
{
    frame[0] = TWR_FRAME_TYPE_FINAL;
    frame[1] = seq;
    for (int i = 0; i < 5; i++) {
        frame[2 + i] = (t1 >> (i * 8)) & 0xFF;
        frame[7 + i] = (t4 >> (i * 8)) & 0xFF;
    }
}

static int _parse_poll(const uint8_t *frame, size_t len)
{
    if (len < TWR_PAYLOAD_POLL_LEN) return -1;
    if (frame[0] != TWR_FRAME_TYPE_POLL) return -1;
    return frame[1];
}

static int _parse_resp(const uint8_t *frame, size_t len)
{
    if (len < TWR_PAYLOAD_RESP_LEN) return -1;
    if (frame[0] != TWR_FRAME_TYPE_RESP) return -1;
    return frame[1];
}

static void _parse_final(const uint8_t *frame, size_t len,
                         uint64_t *t1, uint64_t *t4)
{
    if (len < TWR_PAYLOAD_FINAL_LEN) return;
    if (frame[0] != TWR_FRAME_TYPE_FINAL) return;
    *t1 = 0;
    *t4 = 0;
    for (int i = 0; i < 5; i++) {
        *t1 |= (uint64_t)frame[2 + i] << (i * 8);
        *t4 |= (uint64_t)frame[7 + i] << (i * 8);
    }
}

static void _wait_for_irq(dw1000_t *dev, uint32_t mask, uint32_t timeout_ms)
{
    uint32_t elapsed = 0;
    while (elapsed < timeout_ms) {
        uint32_t status = dw1000_read_irq(dev);
        if (status & mask) {
            dw1000_clear_irq(dev, mask);
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
        elapsed++;
    }
    ESP_LOGW(TAG, "IRQ timeout after %lu ms", (unsigned long)timeout_ms);
}

esp_err_t twr_do_ranging(twr_t *twr, dw1000_t *dev, int role)
{
    uint8_t frame[128];
    size_t frame_len;
    uint32_t status;
    uint32_t serr;

    if (role == TWR_ROLE_TAG) {
        dw1000_clear_irq(dev, 0xFFFFFFFF);
        dw1000_enable_irq(dev, DW1000_IRQ_CMD_TX_DONE | DW1000_IRQ_CMD_RX_DATA);

        _make_poll(frame, twr->seq);
        dw1000_send_frame(dev, frame, TWR_PAYLOAD_POLL_LEN);
        _wait_for_irq(dev, DW1000_IRQ_CMD_TX_DONE, 100);
        if (!(dw1000_read_irq(dev) & DW1000_IRQ_CMD_TX_DONE) == 0) {
            twr->t1 = dw1000_read_tx_timestamp(dev);
        }

        dw1000_set_rx_enable(dev, true);
        _wait_for_irq(dev, DW1000_IRQ_CMD_RX_DATA, UWB_FRAME_TIMEOUT_MS + 500);
        status = dw1000_read_irq(dev);
        if (status & DW1000_IRQ_CMD_RX_DATA) {
            twr->t4 = dw1000_read_rx_timestamp(dev);
            twr->t2 = twr->t4;
        } else {
            serr = dw1000_read32(dev, DW1000_SYS_STATUS, 0x00);
            ESP_LOGW(TAG, "TAG: no response (status=0x%08lX)", (unsigned long)serr);
            dw1000_set_rx_enable(dev, false);
            dw1000_clear_irq(dev, 0xFFFFFFFF);
            vTaskDelay(pdMS_TO_TICKS(10));
            return ESP_ERR_TIMEOUT;
        }

    } else {
        dw1000_set_rx_enable(dev, true);
        _wait_for_irq(dev, DW1000_IRQ_CMD_RX_DATA, UWB_FRAME_TIMEOUT_MS + 500);
        status = dw1000_read_irq(dev);
        if (!(status & DW1000_IRQ_CMD_RX_DATA)) {
            dw1000_clear_irq(dev, 0xFFFFFFFF);
            return ESP_ERR_TIMEOUT;
        }

        frame_len = sizeof(frame);
        dw1000_recv_frame(dev, frame, &frame_len, 0);

        if (frame[0] == TWR_FRAME_TYPE_POLL) {
            twr->t2 = dw1000_read_rx_timestamp(dev);
            dw1000_clear_irq(dev, 0xFFFFFFFF);

            _make_resp(frame, twr->seq);
            dw1000_send_frame(dev, frame, TWR_PAYLOAD_RESP_LEN);
            _wait_for_irq(dev, DW1000_IRQ_CMD_TX_DONE, 100);
            twr->t3 = dw1000_read_tx_timestamp(dev);

            dw1000_set_rx_enable(dev, true);
            _wait_for_irq(dev, DW1000_IRQ_CMD_RX_DATA, UWB_FRAME_TIMEOUT_MS + 500);
            status = dw1000_read_irq(dev);
            if (status & DW1000_IRQ_CMD_RX_DATA) {
                frame_len = sizeof(frame);
                dw1000_recv_frame(dev, frame, &frame_len, 0);
                if (frame[0] == TWR_FRAME_TYPE_FINAL) {
                    _parse_final(frame, frame_len, &twr->t1, &twr->t4);
                    twr->t2 = dw1000_read_rx_timestamp(dev);
                    dw1000_clear_irq(dev, 0xFFFFFFFF);
                }
            }

            if (twr->t1 && twr->t4) {
                uint64_t t_round = twr->t4 - twr->t1;
                uint64_t t_reply = twr->t3 - twr->t2;
                if (t_round > t_reply) {
                    uint64_t tof = (t_round - t_reply) / 2;
                    float range_ps = (float)(tof * 15);
                    float range = range_ps / 1000000.0f * 0.299792458f;
                    twr->range_m = range;
                    twr->ready = true;
                }
            }

        } else if (frame[0] == TWR_FRAME_TYPE_RESP) {
            twr->t4 = dw1000_read_rx_timestamp(dev);
            dw1000_clear_irq(dev, 0xFFFFFFFF);

            _make_final(frame, twr->seq, twr->t1, twr->t4);
            dw1000_send_frame(dev, frame, TWR_PAYLOAD_FINAL_LEN);
            _wait_for_irq(dev, DW1000_IRQ_CMD_TX_DONE, 100);

            dw1000_set_rx_enable(dev, false);
            dw1000_clear_irq(dev, 0xFFFFFFFF);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    twr->seq++;
    return ESP_OK;
}
