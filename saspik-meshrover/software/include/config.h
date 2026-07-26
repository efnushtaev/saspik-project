#pragma once

#define UWB_TAG "uwb"

#define UWB_SPI_CLOCK_HZ    20000000
#define UWB_SPI_HOST        SPI2_HOST

#define UWB_ANTENNA_DELAY   16400

#define UWB_FRAME_TIMEOUT_MS 100

#define TWR_ROLE_TAG  0
#define TWR_ROLE_ANCHOR 1

#ifndef CONFIG_UWB_ROLE
#define CONFIG_UWB_ROLE TWR_ROLE_TAG
#endif
