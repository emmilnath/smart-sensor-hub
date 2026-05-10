/**
 * @file sensor_manager.h
 * @brief Unified sensor reading interface — BME280 + analog light sensor.
 */
#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>

struct SensorData {
    float temperature;    // °C
    float humidity;       // %RH
    float pressure;       // hPa
    float light;          // 0.0 – 1.0 (normalised ADC)
    uint32_t timestamp;   // millis()
    bool valid;
};

class SensorManager {
public:
    SensorManager();

    /**
     * Initialise I2C and sensor hardware. Returns false if BME280 not found.
     */
    bool begin();

    /**
     * Read all sensors. Populates the SensorData struct.
     */
    SensorData read();

    /**
     * Get the most recent reading without re-reading hardware.
     */
    SensorData lastReading() const { return _last; }

private:
    SensorData _last;
    bool _bme_ok;
};

#endif /* SENSOR_MANAGER_H */
