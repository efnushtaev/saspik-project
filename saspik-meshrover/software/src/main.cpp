#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <esp_timer.h>

#include "dw1000.h"
#include "twr.h"
#include "config.h"
#include "pins.h"

static const char *TAG = "meshrover";

static dw1000_t dw_dev;
static twr_t twr;

static void print_hex(const char *label, const uint8_t *data, size_t len)
{
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "MeshRover UWB Node starting...");

    nvs_flash_init();

    esp_err_t err = dw1000_init(&dw_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "DW1000 init failed: %s", esp_err_to_name(err));
        return;
    }

    uint64_t eui = dw1000_read_eui(&dw_dev);
    ESP_LOGI(TAG, "EUI: %016llX", (unsigned long long)eui);

    dw1000_set_channel(&dw_dev, 5);
    dw1000_set_prf(&dw_dev, 1);
    dw1000_set_data_rate(&dw_dev, 0);

    ESP_LOGI(TAG, "DW1000 configured. Waiting for ranging...");

    twr_init(&twr);

    int role = TWR_ROLE_TAG;
    ESP_LOGI(TAG, "Role: %s", role == TWR_ROLE_TAG ? "TAG" : "ANCHOR");

    int64_t last_print = esp_timer_get_time();

    while (1) {
        err = twr_do_ranging(&twr, &dw_dev, role);
        if (err == ESP_OK && twr.ready) {
            printf("RANGE: %.3f m\n", twr.range_m);
            twr.ready = false;
        }

        int64_t now = esp_timer_get_time();
        if (now - last_print > 5000000) {
            ESP_LOGI(TAG, "Heartbeat: seq=%u, err=%s",
                     twr.seq, esp_err_to_name(err));
            last_print = now;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
