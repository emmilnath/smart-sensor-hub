/**
 * @file main.cpp
 * @brief Smart Sensor Hub — main application.
 *
 * Reads environmental sensors (BME280 + light), runs on-device ML inference
 * for anomaly classification, and publishes results over MQTT.
 * Supports deep sleep for battery-powered operation.
 */
#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "sensor_manager.h"
#include "mqtt_client.h"
#include "ml_engine.h"
#include "power_manager.h"

/* ── Global instances ───────────────────────────────── */
static SensorManager sensors;
static MQTTClient    mqtt;
static MLEngine      ml;

static uint32_t last_sensor_ms  = 0;
static uint32_t last_publish_ms = 0;
static uint32_t last_ml_ms      = 0;

/* ── WiFi connection ────────────────────────────────── */
static bool connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return true;

    Serial.printf("[WIFI] Connecting to %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_TIMEOUT_MS) {
            Serial.println(" FAILED");
            return false;
        }
        delay(250);
        Serial.print(".");
    }

    Serial.printf(" OK (%s)\n", WiFi.localIP().toString().c_str());
    return true;
}

/* ── Command handler ────────────────────────────────── */
static void onCommand(const char* topic, const char* payload) {
    Serial.printf("[CMD] %s: %s\n", topic, payload);

    if (strcmp(payload, "reset") == 0) {
        ESP.restart();
    } else if (strcmp(payload, "sleep") == 0) {
        PowerManager::deepSleep(DEEP_SLEEP_DURATION_US);
    } else if (strcmp(payload, "status") == 0) {
        SensorData data = sensors.lastReading();
        mqtt.publishSensorData(data);
    }
}

/* ── Setup ──────────────────────────────────────────── */
void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n=== Smart Sensor Hub ===");
    Serial.printf("Wake reason: %s\n", PowerManager::wakeReason());

    // Battery check
    float vbat = PowerManager::readBatteryVoltage();
    uint8_t pct = PowerManager::batteryPercent(vbat);
    Serial.printf("Battery: %.2fV (%d%%)\n", vbat, pct);

    // Init sensors
    if (!sensors.begin()) {
        Serial.println("[WARN] BME280 not detected — temperature/humidity/pressure unavailable");
    }

    // Init ML engine (pass nullptr for now — uses rule-based fallback)
    ml.begin(nullptr, 0);

    // Connect WiFi
    if (!connectWiFi()) {
        Serial.println("[WARN] WiFi failed — operating in offline mode");
    }

    // Init MQTT
    mqtt.begin(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS);
    mqtt.onCommand(onCommand);
    mqtt.connect();

    Serial.println("[INIT] Setup complete\n");
}

/* ── Main loop ──────────────────────────────────────── */
void loop() {
    mqtt.loop();

    uint32_t now = millis();

    // ── Read sensors ───────────────────────────────
    if (now - last_sensor_ms >= SENSOR_READ_INTERVAL_MS) {
        last_sensor_ms = now;
        SensorData data = sensors.read();

        if (data.valid) {
            Serial.printf("[DATA] T=%.1f°C  H=%.1f%%  P=%.0fhPa  L=%.2f\n",
                          data.temperature, data.humidity,
                          data.pressure, data.light);
        }
    }

    // ── Publish telemetry ──────────────────────────
    if (now - last_publish_ms >= MQTT_PUBLISH_INTERVAL_MS) {
        last_publish_ms = now;
        SensorData data = sensors.lastReading();
        if (data.valid && mqtt.connected()) {
            mqtt.publishSensorData(data);
        }
    }

    // ── ML inference ───────────────────────────────
    if (now - last_ml_ms >= ML_INFERENCE_INTERVAL_MS) {
        last_ml_ms = now;
        SensorData data = sensors.lastReading();
        MLResult result = ml.classify(data);

        if (result.valid) {
            const char* label = MLEngine::stateToString(result.state);
            Serial.printf("[ML] %s (%.1f%%, %uμs)\n",
                          label, result.confidence * 100, result.inference_us);

            if (mqtt.connected()) {
                mqtt.publishMLResult(label, result.confidence);
            }
        }
    }

    // ── Deep sleep check ───────────────────────────
    #if DEEP_SLEEP_ENABLED
    if (now > 30000) {  // Stay awake for at least 30s
        PowerManager::deepSleep(DEEP_SLEEP_DURATION_US);
    }
    #endif
}
