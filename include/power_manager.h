/**
 * @file power_manager.h
 * @brief Deep sleep and power management for battery operation.
 */
#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>

class PowerManager {
public:
    /**
     * Enter deep sleep for the specified duration.
     * Wakes via RTC timer. WiFi and BT are shut down before sleep.
     */
    static void deepSleep(uint64_t duration_us);

    /**
     * Check if this boot is a wake from deep sleep.
     */
    static bool isWakeFromSleep();

    /**
     * Get the wake reason as a human-readable string.
     */
    static const char* wakeReason();

    /**
     * Read battery voltage via ADC (assumes voltage divider on GPIO35).
     * Returns voltage in volts.
     */
    static float readBatteryVoltage();

    /**
     * Estimate battery percentage from voltage (simple linear for LiPo).
     */
    static uint8_t batteryPercent(float voltage);

    /**
     * Disable WiFi and Bluetooth radios to save power.
     */
    static void disableRadios();

    /**
     * Re-enable WiFi (BT stays off unless explicitly started).
     */
    static void enableWiFi();
};

#endif /* POWER_MANAGER_H */
