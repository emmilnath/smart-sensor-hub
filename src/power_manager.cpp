/**
 * @file power_manager.cpp
 * @brief ESP32 deep sleep and battery management.
 */
#include "power_manager.h"
#include <esp_sleep.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <driver/adc.h>

#define BATTERY_ADC_PIN 35
#define VDIV_RATIO      2.0f   // Voltage divider ratio (e.g. 100k/100k)
#define ADC_VREF        3.3f

void PowerManager::deepSleep(uint64_t duration_us) {
    Serial.printf("[POWER] Entering deep sleep for %llu ms\n", duration_us / 1000);
    Serial.flush();

    // Disable peripherals
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    esp_bt_controller_disable();

    // Configure wakeup source
    esp_sleep_enable_timer_wakeup(duration_us);

    // Enter deep sleep
    esp_deep_sleep_start();
    // Execution stops here; resumes from setup() on wake
}

bool PowerManager::isWakeFromSleep() {
    return esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED;
}

const char* PowerManager::wakeReason() {
    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_TIMER:     return "timer";
        case ESP_SLEEP_WAKEUP_EXT0:      return "ext0";
        case ESP_SLEEP_WAKEUP_EXT1:      return "ext1";
        case ESP_SLEEP_WAKEUP_TOUCHPAD:  return "touchpad";
        case ESP_SLEEP_WAKEUP_ULP:       return "ULP";
        case ESP_SLEEP_WAKEUP_UNDEFINED: return "power-on";
        default: return "unknown";
    }
}

float PowerManager::readBatteryVoltage() {
    // Configure ADC
    analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
    uint16_t raw = analogRead(BATTERY_ADC_PIN);
    float voltage = (raw / 4095.0f) * ADC_VREF * VDIV_RATIO;
    return voltage;
}

uint8_t PowerManager::batteryPercent(float voltage) {
    // Simple linear mapping for single-cell LiPo (3.0V–4.2V)
    if (voltage >= 4.2f) return 100;
    if (voltage <= 3.0f) return 0;
    return (uint8_t)((voltage - 3.0f) / 1.2f * 100.0f);
}

void PowerManager::disableRadios() {
    WiFi.mode(WIFI_OFF);
    esp_bt_controller_disable();
}

void PowerManager::enableWiFi() {
    WiFi.mode(WIFI_STA);
}
