# 🌡️ Smart Sensor Hub

ESP32-based IoT sensor node with on-device ML inference and MQTT telemetry. Reads environmental data (temperature, humidity, pressure, ambient light), classifies conditions as normal/warning/critical using a TFLite Micro model, and publishes results over MQTT. Supports deep sleep for battery-powered deployments.

## Features

- **Multi-Sensor Fusion** — BME280 (temperature, humidity, pressure) + analog light sensor with validity checking and forced-measurement mode
- **On-Device ML** — TensorFlow Lite Micro inference with rule-based fallback when no model is loaded. Trained on synthetic sensor data with a tiny MLP (< 2 KB model)
- **MQTT Telemetry** — JSON-formatted sensor data and ML classification results published to configurable topics, with command subscription for remote control
- **Power Management** — ESP32 deep sleep with RTC timer wakeup, battery voltage monitoring via ADC, radio disable/enable for power savings
- **PlatformIO Build** — Single `platformio.ini` for the complete build, including all library dependencies

## Quick Start

### Build and flash
```bash
# Install PlatformIO CLI
pip install platformio

# Build
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

### Train ML model (optional)
```bash
cd model
pip install -r requirements.txt
python train_model.py --epochs 30
# Generates sensor_model.h — copy to include/ and rebuild firmware
```

## Project Structure

```
smart-sensor-hub/
├── src/
│   ├── main.cpp              # Application entry point
│   ├── sensor_manager.cpp    # BME280 + light sensor driver
│   ├── mqtt_client.cpp       # MQTT publish/subscribe
│   ├── ml_engine.cpp         # TFLite inference + rule-based fallback
│   └── power_manager.cpp     # Deep sleep, battery monitoring
├── include/
│   ├── config.h              # WiFi, MQTT, pin, timing configuration
│   ├── sensor_manager.h
│   ├── mqtt_client.h
│   ├── ml_engine.h
│   └── power_manager.h
├── model/
│   ├── train_model.py        # Synthetic data generation + TF training
│   ├── requirements.txt
│   └── sensor_model.h        # Generated C header (model weights)
├── platformio.ini
└── README.md
```

## Hardware

| Component | Connection |
|-----------|-----------|
| BME280 | I2C — SDA=GPIO21, SCL=GPIO22 |
| Light sensor | ADC — GPIO34 |
| Battery monitor | ADC — GPIO35 (via voltage divider) |
| WiFi | Built-in ESP32 radio |

## MQTT Topics

| Topic | Direction | Payload |
|-------|-----------|---------|
| `sensor/data` | Publish | `{"temp": 22.5, "hum": 48.3, "pres": 1013.2, "light": 0.65, "ts": 12345}` |
| `sensor/ml` | Publish | `{"class": "normal", "conf": 0.92, "ts": 12345}` |
| `sensor/cmd` | Subscribe | `reset`, `sleep`, `status` |

## Configuration

All settings are in `include/config.h`. Update WiFi credentials and MQTT broker address before building:

```c
#define WIFI_SSID       "YOUR_SSID"
#define WIFI_PASSWORD   "YOUR_PASSWORD"
#define MQTT_BROKER     "192.168.1.100"
```

## License

MIT
