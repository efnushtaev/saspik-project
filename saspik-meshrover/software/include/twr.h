#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "dw1000.h"

#define TWR_FRAME_TYPE_POLL    0x01
#define TWR_FRAME_TYPE_RESP    0x02
#define TWR_FRAME_TYPE_FINAL   0x03

#define TWR_PAYLOAD_POLL_LEN     2
#define TWR_PAYLOAD_RESP_LEN     2
#define TWR_PAYLOAD_FINAL_LEN    11

#define TWR_DST_ADDR_FINAL_OFF   3
#define TWR_RX_TS_OFF            7

typedef struct {
    uint8_t seq;
    uint64_t t1;
    uint64_t t2;
    uint64_t t3;
    uint64_t t4;
    float range_m;
    bool ready;
} twr_t;

void twr_init(twr_t *twr);
esp_err_t twr_do_ranging(twr_t *twr, dw1000_t *dev, int role);

static inline uint64_t ts_to_ps(uint64_t ts)
{
    return ts * 15;
}

static inline float ts_to_mm(uint64_t ts)
{
    return (float)(ts * 15) / 1000.0f * 0.299792458f;
}

static inline float ts_to_m(uint64_t ts)
{
    return (float)(ts * 15) / 1000000.0f * 0.299792458f;
}
