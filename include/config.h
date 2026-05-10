/**
 * @file config.h
 * @brief Centralised configuration for Smart Sensor Hub.
 */
#ifndef CONFIG_H
#define CONFIG_H

/* ── WiFi ──────────────────────────────────────────── */
#define WIFI_SSID       "YOUR_SSID"
#define WIFI_PASSWORD   "YOUR_PASSWORD"
#define WIFI_TIMEOUT_MS 15000

/* ── MQTT ──────────────────────────────────────────── */
#define MQTT_BROKER     "192.168.1.100"
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "smart-sensor-hub"
#define MQTT_USER       ""
#define MQTT_PASS       ""
#define MQTT_TOPIC_DATA "sensor/data"
#define MQTT_TOPIC_CMD  "sensor/cmd"
#define MQTT_TOPIC_ML   "sensor/ml"

/* ── Sensor Pins ───────────────────────────────────── */
#define BME280_SDA      21
#define BME280_SCL      22
#define LIGHT_SENSOR_PIN 34   /* ADC1_CH6 */

/* ── Timing ────────────────────────────────────────── */
#define SENSOR_READ_INTERVAL_MS  1000
#define MQTT_PUBLISH_INTERVAL_MS 5000
#define ML_INFERENCE_INTERVAL_MS 5000

/* ── Deep Sleep ────────────────────────────────────── */
#define DEEP_SLEEP_ENABLED       false
#define DEEP_SLEEP_DURATION_US   (60ULL * 1000000ULL)  /* 60 seconds */

/* ── ML Model ──────────────────────────────────────── */
#define ML_ARENA_SIZE   8192
#define ML_INPUT_SIZE   4      /* temp, humidity, pressure, light */
#define ML_OUTPUT_SIZE  3      /* normal, warning, critical */

#endif /* CONFIG_H */
