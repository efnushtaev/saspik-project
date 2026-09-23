#ifndef CONFIG_H
#define CONFIG_H

// --- Класс устройства ---
#define DEVICE_CLASS "saspik.s.en.b001"

/**
 * WiFi settings
 */
#define WIFI_SSID "TP-Link_B354"
#define WIFI_PASS "57339016"
/**
 * ESP-NOW receiver MAC address
 */
#define ESP_NOW_RECEIVER_MAC {0xd4, 0xe9, 0xf4, 0xf3, 0x97, 0x38}
/**
 * Pins
 */
#define DHT22_DATA_D_PIN 4
#define FLOAT_SENSOR_D_PIN 13  // Пин для поплавкового датчика (геркон)
/**
 * Samples
 */
#define SAMPLES 10      // Количество образцов
#define SAMPLE_DELAY 50 // Задержка между замерами (мс)
/**
 * Sleep
 */
#define SLEEP_SECONDS 2 // Интервал сна
/**
 * Stabilization delay
 */
#define STAB_DELAY 500 // Интервал сна

#endif