/**
 * @file sensor_manager.cpp
 * @brief BME280 + analog light sensor driver.
 */
#include "sensor_manager.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_BME280.h>

static Adafruit_BME280 bme;

SensorManager::SensorManager() : _bme_ok(false) {
    memset(&_last, 0, sizeof(_last));
}

bool SensorManager::begin() {
    Wire.begin(BME280_SDA, BME280_SCL);

    _bme_ok = bme.begin(0x76, &Wire);
    if (!_bme_ok) {
        // Try alternate address
        _bme_ok = bme.begin(0x77, &Wire);
    }

    if (_bme_ok) {
        // Weather monitoring mode — low power
        bme.setSampling(
            Adafruit_BME280::MODE_FORCED,
            Adafruit_BME280::SAMPLING_X1,   // temperature
            Adafruit_BME280::SAMPLING_X1,   // pressure
            Adafruit_BME280::SAMPLING_X1,   // humidity
            Adafruit_BME280::FILTER_OFF,
            Adafruit_BME280::STANDBY_MS_1000
        );
        Serial.println("[SENSOR] BME280 initialised");
    } else {
        Serial.println("[SENSOR] BME280 not found!");
    }

    // Light sensor — analog input
    pinMode(LIGHT_SENSOR_PIN, INPUT);
    analogSetAttenuation(ADC_11db);  // Full 0-3.3V range

    return _bme_ok;
}

SensorData SensorManager::read() {
    SensorData data;
    data.timestamp = millis();
    data.valid = false;

    if (_bme_ok) {
        bme.takeForcedMeasurement();
        data.temperature = bme.readTemperature();
        data.humidity = bme.readHumidity();
        data.pressure = bme.readPressure() / 100.0f;  // Pa → hPa

        // Sanity check
        if (data.temperature > -40 && data.temperature < 85 &&
            data.humidity >= 0 && data.humidity <= 100 &&
            data.pressure > 300 && data.pressure < 1100) {
            data.valid = true;
        }
    } else {
        data.temperature = NAN;
        data.humidity = NAN;
        data.pressure = NAN;
    }

    // Light sensor: 12-bit ADC → normalised 0.0–1.0
    uint16_t raw = analogRead(LIGHT_SENSOR_PIN);
    data.light = (float)raw / 4095.0f;

    _last = data;
    return data;
}
